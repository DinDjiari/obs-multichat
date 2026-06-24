#include "youtube-client.hpp"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

YouTubeClient::YouTubeClient(QObject *parent)
	: QObject(parent),
	  m_net(new QNetworkAccessManager(this)),
	  m_findTimer(new QTimer(this)),
	  m_pollTimer(new QTimer(this))
{
	m_findTimer->setInterval(15000);
	connect(m_findTimer, &QTimer::timeout, this,
		&YouTubeClient::findActiveBroadcast);

	m_pollTimer->setSingleShot(true);
	connect(m_pollTimer, &QTimer::timeout, this,
		&YouTubeClient::pollMessages);
}

void YouTubeClient::setCredentials(const QString &accessToken)
{
	m_token = accessToken;
}

void YouTubeClient::disconnectAll()
{
	m_wantConnected = false;
	m_findTimer->stop();
	m_pollTimer->stop();
	m_liveChatId.clear();
	m_pageToken.clear();
	emit statusChanged(QStringLiteral("Disconnected"), false);
}

void YouTubeClient::connectChat()
{
	if (m_token.isEmpty()) {
		emit statusChanged(QStringLiteral("Missing token"), false);
		return;
	}
	m_wantConnected = true;
	m_primingPoll = true;
	m_pageToken.clear();
	emit statusChanged(QStringLiteral("Searching live broadcast..."),
			   false);
	findActiveBroadcast();
	m_findTimer->start();
}

void YouTubeClient::findActiveBroadcast()
{
	if (!m_wantConnected)
		return;

	QUrl url(QStringLiteral(
		"https://www.googleapis.com/youtube/v3/liveBroadcasts"));
	QUrlQuery q;
	q.addQueryItem(QStringLiteral("part"),
		       QStringLiteral("snippet,status"));
	q.addQueryItem(QStringLiteral("mine"), QStringLiteral("true"));
	q.addQueryItem(QStringLiteral("maxResults"), QStringLiteral("50"));
	url.setQuery(q);

	QNetworkRequest req(url);
	req.setRawHeader("Authorization",
			 QByteArray("Bearer ") + m_token.toUtf8());

	QNetworkReply *reply = m_net->get(req);
	connect(reply, &QNetworkReply::finished, this, [this, reply]() {
		reply->deleteLater();
		if (!m_wantConnected)
			return;

		if (reply->error() == QNetworkReply::AuthenticationRequiredError) {
			emit authError();
			return;
		}

		const QByteArray data = reply->readAll();
		const QJsonObject o = QJsonDocument::fromJson(data).object();

		// Surface API errors (quota, scope, live streaming not enabled)
		// instead of silently retrying forever.
		const QJsonObject err = o.value(QStringLiteral("error"))
						.toObject();
		if (!err.isEmpty()) {
			const int code = err.value(QStringLiteral("code"))
						 .toInt();
			QString reason;
			const QJsonArray errs =
				err.value(QStringLiteral("errors")).toArray();
			if (!errs.isEmpty())
				reason = errs.first()
						 .toObject()
						 .value(QStringLiteral("reason"))
						 .toString();
			if (reason.isEmpty())
				reason = err.value(QStringLiteral("message"))
						 .toString();
			emit statusChanged(
				tr("YouTube API error %1: %2")
					.arg(code)
					.arg(reason),
				false);
			return;
		}
		if (reply->error() != QNetworkReply::NoError) {
			const int http =
				reply->attribute(
					QNetworkRequest::HttpStatusCodeAttribute)
					.toInt();
			emit statusChanged(
				tr("Searching... (HTTP %1)").arg(http), false);
			return;
		}

		const QJsonArray items =
			o.value(QStringLiteral("items")).toArray();

		QString liveChatId;
		bool sawChatlessLive = false;
		for (const QJsonValue &v : items) {
			const QJsonObject b = v.toObject();
			const QString ls =
				b.value(QStringLiteral("status"))
					.toObject()
					.value(QStringLiteral("lifeCycleStatus"))
					.toString();
			if (ls != QStringLiteral("live") &&
			    ls != QStringLiteral("liveStarting"))
				continue;

			const QString cid =
				b.value(QStringLiteral("snippet"))
					.toObject()
					.value(QStringLiteral("liveChatId"))
					.toString();
			if (cid.isEmpty()) {
				sawChatlessLive = true;
				continue;
			}
			liveChatId = cid;
			if (ls == QStringLiteral("live"))
				break; // prefer a fully live broadcast
		}

		if (liveChatId.isEmpty()) {
			if (sawChatlessLive)
				emit statusChanged(
					tr("Live, but chat is disabled for "
					   "this stream."),
					false);
			else
				emit statusChanged(
					tr("Searching live broadcast... "
					   "(none live yet)"),
					false);
			return; // keep retrying via m_findTimer
		}

		m_liveChatId = liveChatId;
		m_findTimer->stop();
		emit statusChanged(QStringLiteral("Connected"), true);
		pollMessages();
	});
}

void YouTubeClient::pollMessages()
{
	if (!m_wantConnected || m_liveChatId.isEmpty())
		return;

	QUrl url(QStringLiteral(
		"https://www.googleapis.com/youtube/v3/liveChatMessages"));
	QUrlQuery q;
	q.addQueryItem(QStringLiteral("liveChatId"), m_liveChatId);
	q.addQueryItem(QStringLiteral("part"),
		       QStringLiteral("snippet,authorDetails"));
	if (!m_pageToken.isEmpty())
		q.addQueryItem(QStringLiteral("pageToken"), m_pageToken);
	url.setQuery(q);

	QNetworkRequest req(url);
	req.setRawHeader("Authorization",
			 QByteArray("Bearer ") + m_token.toUtf8());

	QNetworkReply *reply = m_net->get(req);
	connect(reply, &QNetworkReply::finished, this, [this, reply]() {
		reply->deleteLater();
		if (!m_wantConnected)
			return;

		if (reply->error() == QNetworkReply::AuthenticationRequiredError) {
			emit authError();
			return;
		}
		if (reply->error() != QNetworkReply::NoError) {
			const int http =
				reply->attribute(
					QNetworkRequest::HttpStatusCodeAttribute)
					.toInt();
			// Don't drop the connection on the first hiccup. Right
			// after going live the chat endpoint can briefly return
			// 403/404/5xx; retry the same chat several times before
			// giving up and searching again (prevents flapping).
			if (++m_pollFailures < 6) {
				emit statusChanged(
					tr("Reconnecting (HTTP %1)...")
						.arg(http),
					true);
				m_pollTimer->start(3000);
				return;
			}
			// Repeated failures: the stream likely ended.
			m_pollFailures = 0;
			m_liveChatId.clear();
			emit statusChanged(
				QStringLiteral("Searching live broadcast..."),
				false);
			m_findTimer->start();
			return;
		}

		m_pollFailures = 0;

		const QJsonObject o =
			QJsonDocument::fromJson(reply->readAll()).object();

		m_pageToken =
			o.value(QStringLiteral("nextPageToken")).toString();

		// The first batch is historical backlog; show only new ones.
		const QJsonArray items =
			o.value(QStringLiteral("items")).toArray();
		if (!m_primingPoll)
			handleItems(items);
		m_primingPoll = false;

		int interval =
			o.value(QStringLiteral("pollingIntervalMillis"))
				.toInt(5000);
		if (interval < 2000)
			interval = 2000;
		m_pollTimer->start(interval);
	});
}

void YouTubeClient::handleItems(const QJsonArray &items)
{
	for (const QJsonValue &v : items) {
		const QJsonObject item = v.toObject();
		const QJsonObject snippet =
			item.value(QStringLiteral("snippet")).toObject();
		const QJsonObject author =
			item.value(QStringLiteral("authorDetails")).toObject();

		const QString type =
			snippet.value(QStringLiteral("type")).toString();
		const QString name =
			author.value(QStringLiteral("displayName")).toString();

		if (type == QStringLiteral("textMessageEvent")) {
			ChatMessage m;
			m.platform = Platform::YouTube;
			m.author = name;
			m.message =
				snippet.value(QStringLiteral(
						      "textMessageDetails"))
					.toObject()
					.value(QStringLiteral("messageText"))
					.toString();
			m.isModerator =
				author.value(QStringLiteral("isChatModerator"))
					.toBool();
			m.isMember =
				author.value(QStringLiteral("isChatSponsor"))
					.toBool();
			emit chatMessage(m);
		} else if (type == QStringLiteral("superChatEvent")) {
			const QJsonObject d =
				snippet.value(QStringLiteral(
						      "superChatDetails"))
					.toObject();
			Alert a;
			a.type = AlertType::YouTubeSuperChat;
			a.platform = Platform::YouTube;
			a.user = name;
			a.message =
				d.value(QStringLiteral("userComment"))
					.toString();
			a.detail = d.value(QStringLiteral(
						   "amountDisplayString"))
					   .toString();
			a.money = d.value(QStringLiteral("amountMicros"))
					  .toString()
					  .toLongLong() /
				  1000000.0;
			a.currency =
				d.value(QStringLiteral("currency")).toString();
			emit alert(a);
		} else if (type == QStringLiteral("superStickerEvent")) {
			const QJsonObject d =
				snippet.value(QStringLiteral(
						      "superStickerDetails"))
					.toObject();
			Alert a;
			a.type = AlertType::YouTubeSuperSticker;
			a.platform = Platform::YouTube;
			a.user = name;
			a.detail = d.value(QStringLiteral(
						   "amountDisplayString"))
					   .toString();
			a.currency =
				d.value(QStringLiteral("currency")).toString();
			a.money = d.value(QStringLiteral("amountMicros"))
					  .toString()
					  .toLongLong() /
				  1000000.0;
			a.message =
				d.value(QStringLiteral("superStickerMetadata"))
					.toObject()
					.value(QStringLiteral("altText"))
					.toString();
			emit alert(a);
		} else if (type == QStringLiteral("newSponsorEvent")) {
			const QJsonObject d =
				snippet.value(QStringLiteral(
						      "newSponsorDetails"))
					.toObject();
			Alert a;
			a.type = AlertType::YouTubeMembership;
			a.platform = Platform::YouTube;
			a.user = name;
			a.detail = d.value(QStringLiteral("memberLevelName"))
					   .toString();
			emit alert(a);
		} else if (type ==
			   QStringLiteral("memberMilestoneChatEvent")) {
			const QJsonObject d =
				snippet.value(QStringLiteral(
						      "memberMilestoneChatDetails"))
					.toObject();
			Alert a;
			a.type = AlertType::YouTubeMembership;
			a.platform = Platform::YouTube;
			a.user = name;
			a.amount = d.value(QStringLiteral("memberMonth"))
					   .toInt();
			a.detail = d.value(QStringLiteral("memberLevelName"))
					   .toString();
			a.message =
				d.value(QStringLiteral("userComment"))
					.toString();
			emit alert(a);
		} else if (type ==
				   QStringLiteral("membershipGiftingEvent") ||
			   type == QStringLiteral(
					   "giftMembershipReceivedEvent")) {
			Alert a;
			a.type = AlertType::YouTubeMembership;
			a.platform = Platform::YouTube;
			a.user = name;
			a.detail = QStringLiteral("Gift membership");
			emit alert(a);
		}
	}
}
