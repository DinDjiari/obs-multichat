#include "twitch-client.hpp"

#include <QtWebSockets/QWebSocket>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QHash>
#include <QRegularExpression>

namespace {

// Parse an IRCv3 line into tags, prefix, command, params and trailing text.
struct IrcLine {
	QHash<QString, QString> tags;
	QString prefix;
	QString command;
	QStringList params;
	QString trailing;
};

QString unescapeTag(const QString &v)
{
	QString out;
	for (int i = 0; i < v.size(); ++i) {
		if (v[i] == '\\' && i + 1 < v.size()) {
			const QChar n = v[++i];
			if (n == 's')
				out += ' ';
			else if (n == ':')
				out += ';';
			else if (n == 'r')
				out += '\r';
			else if (n == 'n')
				out += '\n';
			else if (n == '\\')
				out += '\\';
			else
				out += n;
		} else {
			out += v[i];
		}
	}
	return out;
}

IrcLine parseIrc(const QString &raw)
{
	IrcLine l;
	QString s = raw;

	if (s.startsWith('@')) {
		const int sp = s.indexOf(' ');
		const QString tagStr = s.mid(1, sp - 1);
		s = s.mid(sp + 1);
		const QStringList pairs = tagStr.split(';', Qt::SkipEmptyParts);
		for (const QString &p : pairs) {
			const int eq = p.indexOf('=');
			if (eq < 0)
				l.tags.insert(p, QString());
			else
				l.tags.insert(p.left(eq),
					      unescapeTag(p.mid(eq + 1)));
		}
	}

	if (s.startsWith(':')) {
		const int sp = s.indexOf(' ');
		l.prefix = s.mid(1, sp - 1);
		s = s.mid(sp + 1);
	}

	const int colon = s.indexOf(QStringLiteral(" :"));
	QString head = s;
	if (colon >= 0) {
		head = s.left(colon);
		l.trailing = s.mid(colon + 2);
	}

	const QStringList parts = head.split(' ', Qt::SkipEmptyParts);
	if (!parts.isEmpty()) {
		l.command = parts.first();
		for (int i = 1; i < parts.size(); ++i)
			l.params << parts[i];
	}
	return l;
}

QString nickFromPrefix(const QString &prefix)
{
	const int bang = prefix.indexOf('!');
	return bang > 0 ? prefix.left(bang) : prefix;
}

QString subPlanName(const QString &plan)
{
	if (plan == QStringLiteral("Prime"))
		return QStringLiteral("Prime");
	if (plan == QStringLiteral("1000"))
		return QStringLiteral("Tier 1");
	if (plan == QStringLiteral("2000"))
		return QStringLiteral("Tier 2");
	if (plan == QStringLiteral("3000"))
		return QStringLiteral("Tier 3");
	return plan;
}

} // namespace

TwitchClient::TwitchClient(QObject *parent)
	: QObject(parent), m_net(new QNetworkAccessManager(this))
{
}

TwitchClient::~TwitchClient()
{
	disconnectAll();
}

void TwitchClient::setCredentials(const QString &clientId,
				  const QString &accessToken,
				  const QString &channel)
{
	m_clientId = clientId;
	m_token = accessToken;
	m_channel = channel.trimmed().toLower();
	if (m_channel.startsWith('#'))
		m_channel = m_channel.mid(1);
}

void TwitchClient::disconnectAll()
{
	m_wantConnected = false;
	if (m_irc) {
		m_irc->close();
		m_irc->deleteLater();
		m_irc = nullptr;
	}
	if (m_eventsub) {
		m_eventsub->close();
		m_eventsub->deleteLater();
		m_eventsub = nullptr;
	}
	m_chatConnected = false;
	emit statusChanged(QStringLiteral("Disconnected"), false);
}

void TwitchClient::connectChat()
{
	if (m_token.isEmpty() || m_channel.isEmpty()) {
		emit statusChanged(QStringLiteral("Missing token or channel"),
				   false);
		return;
	}

	m_wantConnected = true;
	emit statusChanged(QStringLiteral("Connecting..."), false);

	if (m_irc) {
		m_irc->deleteLater();
	}
	m_irc = new QWebSocket();
	m_irc->setParent(this);

	connect(m_irc, &QWebSocket::connected, this,
		&TwitchClient::onIrcConnected);
	connect(m_irc, &QWebSocket::textMessageReceived, this,
		&TwitchClient::onIrcText);
	connect(m_irc, &QWebSocket::disconnected, this,
		&TwitchClient::onIrcClosed);

	m_irc->open(QUrl(QStringLiteral("wss://irc-ws.chat.twitch.tv:443")));

	// EventSub for follows runs in parallel.
	fetchBroadcasterId();
}

void TwitchClient::sendIrc(const QString &line)
{
	if (m_irc && m_irc->state() == QAbstractSocket::ConnectedState)
		m_irc->sendTextMessage(line + QStringLiteral("\r\n"));
}

void TwitchClient::onIrcConnected()
{
	sendIrc(QStringLiteral(
		"CAP REQ :twitch.tv/tags twitch.tv/commands twitch.tv/membership"));
	sendIrc(QStringLiteral("PASS oauth:") + m_token);
	// Login name is not strictly validated for reading; use a placeholder
	// derived from the channel; Twitch accepts any nick with a valid token.
	sendIrc(QStringLiteral("NICK ") + m_channel);
	sendIrc(QStringLiteral("JOIN #") + m_channel);

	m_chatConnected = true;
	emit statusChanged(QStringLiteral("Connected"), true);
}

void TwitchClient::onIrcClosed()
{
	m_chatConnected = false;
	emit statusChanged(QStringLiteral("Disconnected"), false);
}

void TwitchClient::onIrcText(const QString &frame)
{
	// A frame can contain multiple lines.
	const QStringList lines = frame.split(QRegularExpression("\r?\n"),
					      Qt::SkipEmptyParts);
	for (const QString &line : lines)
		handleIrcLine(line);
}

void TwitchClient::handleIrcLine(const QString &raw)
{
	const IrcLine l = parseIrc(raw);

	if (l.command == QStringLiteral("PING")) {
		sendIrc(QStringLiteral("PONG :") + l.trailing);
		return;
	}

	if (l.command == QStringLiteral("NOTICE")) {
		// Login authentication failed.
		if (l.trailing.contains(QStringLiteral("authentication"),
					Qt::CaseInsensitive) ||
		    l.trailing.contains(QStringLiteral("Login"),
					Qt::CaseInsensitive)) {
			emit authError();
		}
		return;
	}

	if (l.command == QStringLiteral("PRIVMSG")) {
		ChatMessage m;
		m.platform = Platform::Twitch;
		m.author = l.tags.value(QStringLiteral("display-name"),
					nickFromPrefix(l.prefix));
		m.message = l.trailing;
		const QString color =
			l.tags.value(QStringLiteral("color"));
		if (!color.isEmpty())
			m.authorColor = QColor(color);
		m.isModerator =
			l.tags.value(QStringLiteral("mod")) ==
			QStringLiteral("1");
		m.isMember =
			l.tags.value(QStringLiteral("subscriber")) ==
			QStringLiteral("1");
		emit chatMessage(m);

		// Cheer / bits.
		const int bits =
			l.tags.value(QStringLiteral("bits")).toInt();
		if (bits > 0) {
			Alert a;
			a.type = AlertType::TwitchBits;
			a.platform = Platform::Twitch;
			a.user = m.author;
			a.amount = bits;
			a.message = l.trailing;
			emit alert(a);
		}
		return;
	}

	if (l.command == QStringLiteral("USERNOTICE")) {
		const QString msgId =
			l.tags.value(QStringLiteral("msg-id"));
		const QString login = l.tags.value(
			QStringLiteral("login"),
			l.tags.value(QStringLiteral("display-name")));

		Alert a;
		a.platform = Platform::Twitch;
		a.user = l.tags.value(QStringLiteral("display-name"), login);
		a.message = l.trailing;

		if (msgId == QStringLiteral("sub") ||
		    msgId == QStringLiteral("resub")) {
			a.type = AlertType::TwitchSub;
			a.detail = subPlanName(l.tags.value(
				QStringLiteral("msg-param-sub-plan")));
			a.amount = l.tags
					   .value(QStringLiteral(
						   "msg-param-cumulative-months"))
					   .toInt();
			emit alert(a);
		} else if (msgId == QStringLiteral("subgift") ||
			   msgId == QStringLiteral("anonsubgift")) {
			a.type = AlertType::TwitchGiftSub;
			a.detail = subPlanName(l.tags.value(
				QStringLiteral("msg-param-sub-plan")));
			a.message = l.tags.value(QStringLiteral(
				"msg-param-recipient-display-name"));
			a.amount = 1;
			emit alert(a);
		} else if (msgId == QStringLiteral("submysterygift")) {
			a.type = AlertType::TwitchGiftSub;
			a.detail = subPlanName(l.tags.value(
				QStringLiteral("msg-param-sub-plan")));
			bool ok = false;
			const int count =
				l.tags.value(QStringLiteral(
					      "msg-param-mass-gift-count"))
					.toInt(&ok);
			a.amount = ok ? count : 1;
			emit alert(a);
		} else if (msgId == QStringLiteral("raid")) {
			a.type = AlertType::TwitchRaid;
			a.user = l.tags.value(
				QStringLiteral("msg-param-displayName"),
				a.user);
			a.amount =
				l.tags.value(QStringLiteral(
						     "msg-param-viewerCount"))
					.toInt();
			emit alert(a);
		}
		return;
	}
}

// ---- EventSub (channel.follow) -------------------------------------------

void TwitchClient::fetchBroadcasterId()
{
	if (m_clientId.isEmpty() || m_token.isEmpty())
		return;

	QNetworkRequest req(
		QUrl(QStringLiteral("https://api.twitch.tv/helix/users")));
	req.setRawHeader("Client-Id", m_clientId.toUtf8());
	req.setRawHeader("Authorization",
			 QByteArray("Bearer ") + m_token.toUtf8());

	QNetworkReply *reply = m_net->get(req);
	connect(reply, &QNetworkReply::finished, this, [this, reply]() {
		reply->deleteLater();
		const QJsonObject o =
			QJsonDocument::fromJson(reply->readAll()).object();
		const QJsonArray data =
			o.value(QStringLiteral("data")).toArray();
		if (data.isEmpty()) {
			if (reply->error() != QNetworkReply::NoError)
				emit authError();
			return;
		}
		m_broadcasterId = data.first()
					  .toObject()
					  .value(QStringLiteral("id"))
					  .toString();
		connectEventSub();
	});
}

void TwitchClient::connectEventSub()
{
	if (m_broadcasterId.isEmpty())
		return;

	if (m_eventsub)
		m_eventsub->deleteLater();
	m_eventsub = new QWebSocket();
	m_eventsub->setParent(this);

	connect(m_eventsub, &QWebSocket::textMessageReceived, this,
		&TwitchClient::onEventSubText);

	m_eventsub->open(
		QUrl(QStringLiteral("wss://eventsub.wss.twitch.tv/ws")));
}

void TwitchClient::onEventSubText(const QString &frame)
{
	const QJsonObject root = QJsonDocument::fromJson(frame.toUtf8()).object();
	const QJsonObject meta =
		root.value(QStringLiteral("metadata")).toObject();
	const QString type =
		meta.value(QStringLiteral("message_type")).toString();
	const QJsonObject payload =
		root.value(QStringLiteral("payload")).toObject();

	if (type == QStringLiteral("session_welcome")) {
		const QString sessionId =
			payload.value(QStringLiteral("session"))
				.toObject()
				.value(QStringLiteral("id"))
				.toString();
		createFollowSubscription(sessionId);
	} else if (type == QStringLiteral("session_reconnect")) {
		const QString url =
			payload.value(QStringLiteral("session"))
				.toObject()
				.value(QStringLiteral("reconnect_url"))
				.toString();
		if (!url.isEmpty() && m_eventsub)
			m_eventsub->open(QUrl(url));
	} else if (type == QStringLiteral("notification")) {
		const QString subType =
			meta.value(QStringLiteral("subscription_type"))
				.toString();
		if (subType == QStringLiteral("channel.follow")) {
			const QJsonObject event =
				payload.value(QStringLiteral("event"))
					.toObject();
			Alert a;
			a.type = AlertType::TwitchFollow;
			a.platform = Platform::Twitch;
			a.user = event.value(QStringLiteral("user_name"))
					 .toString();
			emit alert(a);
		}
	}
}

void TwitchClient::createFollowSubscription(const QString &sessionId)
{
	if (sessionId.isEmpty() || m_broadcasterId.isEmpty())
		return;

	QJsonObject condition;
	condition.insert(QStringLiteral("broadcaster_user_id"),
			 m_broadcasterId);
	condition.insert(QStringLiteral("moderator_user_id"),
			 m_broadcasterId);

	QJsonObject transport;
	transport.insert(QStringLiteral("method"),
			 QStringLiteral("websocket"));
	transport.insert(QStringLiteral("session_id"), sessionId);

	QJsonObject body;
	body.insert(QStringLiteral("type"),
		    QStringLiteral("channel.follow"));
	body.insert(QStringLiteral("version"), QStringLiteral("2"));
	body.insert(QStringLiteral("condition"), condition);
	body.insert(QStringLiteral("transport"), transport);

	QNetworkRequest req(QUrl(QStringLiteral(
		"https://api.twitch.tv/helix/eventsub/subscriptions")));
	req.setRawHeader("Client-Id", m_clientId.toUtf8());
	req.setRawHeader("Authorization",
			 QByteArray("Bearer ") + m_token.toUtf8());
	req.setHeader(QNetworkRequest::ContentTypeHeader,
		      QStringLiteral("application/json"));

	QNetworkReply *reply = m_net->post(
		req, QJsonDocument(body).toJson(QJsonDocument::Compact));
	connect(reply, &QNetworkReply::finished, reply,
		&QNetworkReply::deleteLater);
}
