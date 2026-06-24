#include "loopback-auth.hpp"

#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDesktopServices>
#include <QCryptographicHash>
#include <QRandomGenerator>

namespace {

QString randomString(int len)
{
	static const char chars[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~";
	const int n = int(sizeof(chars)) - 1;
	QString s;
	s.reserve(len);
	for (int i = 0; i < len; ++i)
		s.append(QChar(chars[QRandomGenerator::global()->bounded(n)]));
	return s;
}

QString pkceChallenge(const QString &verifier)
{
	const QByteArray digest = QCryptographicHash::hash(
		verifier.toUtf8(), QCryptographicHash::Sha256);
	return QString::fromLatin1(digest.toBase64(
		QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

// "GET /path?query HTTP/1.1" -> "/path?query"
QString requestTarget(const QString &requestLine)
{
	const int sp1 = requestLine.indexOf(' ');
	const int sp2 = requestLine.indexOf(' ', sp1 + 1);
	if (sp1 < 0 || sp2 < 0)
		return {};
	return requestLine.mid(sp1 + 1, sp2 - sp1 - 1);
}

QString queryParam(const QString &target, const QString &key)
{
	const int q = target.indexOf('?');
	if (q < 0)
		return {};
	QUrlQuery query(target.mid(q + 1));
	return query.queryItemValue(key, QUrl::FullyDecoded);
}

void writeHttp(QTcpSocket *sock, const QByteArray &body)
{
	QByteArray resp = "HTTP/1.1 200 OK\r\n"
			  "Content-Type: text/html; charset=utf-8\r\n";
	resp += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
	resp += "Connection: close\r\n\r\n";
	resp += body;
	sock->write(resp);
	sock->flush();
	sock->disconnectFromHost();
	sock->deleteLater();
}

QByteArray pageHtml(const QString &heading)
{
	return QStringLiteral(
		       "<html><head><meta charset='utf-8'></head>"
		       "<body style='font-family:sans-serif;background:#1e1e1e;"
		       "color:#ddd;text-align:center;padding-top:80px'>"
		       "<h2>%1</h2><p>You can close this tab and return to "
		       "OBS.</p></body></html>")
		.arg(heading)
		.toUtf8();
}

// Page served on the implicit redirect: reads the access token from the URL
// fragment (which the browser never sends to the server) and posts it back to
// the local listener.
QByteArray bridgeHtml()
{
	return QByteArrayLiteral(
		"<html><head><meta charset='utf-8'></head>"
		"<body style='font-family:sans-serif;background:#1e1e1e;"
		"color:#ddd;text-align:center;padding-top:80px'>"
		"<h2>Finishing login...</h2>"
		"<script>"
		"var h=location.hash?location.hash.substring(1):'';"
		"fetch('/token?'+h).then(function(){"
		"document.body.innerHTML="
		"\"<h2>Login complete</h2><p>You can close this tab \"+"
		"\"and return to OBS.</p>\";});"
		"</script></body></html>");
}

} // namespace

LoopbackAuth::LoopbackAuth(QObject *parent) : QObject(parent)
{
	m_net = new QNetworkAccessManager(this);
}

LoopbackAuth::~LoopbackAuth()
{
	cancel();
}

QString LoopbackAuth::redirectUri() const
{
	return m_redirectUri;
}

void LoopbackAuth::cancel()
{
	if (m_server) {
		m_server->close();
		m_server->deleteLater();
		m_server = nullptr;
	}
	m_port = 0;
}

void LoopbackAuth::fail(const QString &error)
{
	cancel();
	emit failed(error);
}

QString LoopbackAuth::buildAuthUrl()
{
	QUrl url(m_ep.authUrl);
	QUrlQuery q;
	q.addQueryItem(QStringLiteral("client_id"), m_ep.clientId);
	q.addQueryItem(QStringLiteral("redirect_uri"), redirectUri());
	q.addQueryItem(QStringLiteral("response_type"),
		       m_ep.implicit ? QStringLiteral("token")
				     : QStringLiteral("code"));
	q.addQueryItem(QStringLiteral("scope"), m_ep.scope);
	if (m_ep.usePkce) {
		q.addQueryItem(QStringLiteral("code_challenge"),
			       pkceChallenge(m_codeVerifier));
		q.addQueryItem(QStringLiteral("code_challenge_method"),
			       QStringLiteral("S256"));
	}
	if (!m_ep.implicit) {
		q.addQueryItem(QStringLiteral("access_type"),
			       QStringLiteral("offline"));
		q.addQueryItem(QStringLiteral("prompt"),
			       QStringLiteral("consent"));
	}
	q.addQueryItem(QStringLiteral("state"), m_state);
	url.setQuery(q);
	return url.toString(QUrl::FullyEncoded);
}

void LoopbackAuth::start(const OAuthEndpoints &ep)
{
	cancel();
	m_ep = ep;
	m_codeVerifier = randomString(64);
	m_state = randomString(24);

	bool bound = false;
	const quint16 wantPort = quint16(m_ep.loopbackPort);
	m_server = new QTcpServer(this);
	if (m_server->listen(QHostAddress::LocalHost, wantPort)) {
		bound = true;
		m_port = m_server->serverPort();
		connect(m_server, &QTcpServer::newConnection, this,
			&LoopbackAuth::onNewConnection);
	} else {
		m_server->deleteLater();
		m_server = nullptr;
		if (wantPort != 0) {
			// Fixed port (Twitch): the URL is still valid; let the
			// user finish manually by pasting the redirect.
			m_port = wantPort;
		} else {
			emit failed(tr("Could not open a local listener for "
				       "login."));
			return;
		}
	}

	m_redirectUri = QStringLiteral("http://%1:%2")
				.arg(m_ep.loopbackHost)
				.arg(m_port);

	const QString authUrl = buildAuthUrl();
	const bool opened = QDesktopServices::openUrl(QUrl(authUrl));

	if (bound && opened)
		return; // normal path: wait for the redirect

	// Either the browser would not open, or the port was busy.
	emit manualFallback(authUrl, !bound);
}

void LoopbackAuth::completeFromRedirect(const QString &pastedUrl)
{
	const QUrl u(pastedUrl.trimmed());

	if (m_ep.implicit) {
		QUrlQuery frag(u.fragment());
		const QString token =
			frag.queryItemValue(QStringLiteral("access_token"),
					    QUrl::FullyDecoded);
		const QString state =
			frag.queryItemValue(QStringLiteral("state"),
					    QUrl::FullyDecoded);
		if (token.isEmpty()) {
			emit failed(tr("No access token found in the pasted "
				       "address."));
			return;
		}
		if (!m_state.isEmpty() && !state.isEmpty() &&
		    state != m_state) {
			emit failed(tr("State mismatch during login."));
			return;
		}
		cancel();
		emit succeeded(token, QString(), 0);
		return;
	}

	QUrlQuery q(u.query());
	const QString code =
		q.queryItemValue(QStringLiteral("code"), QUrl::FullyDecoded);
	if (code.isEmpty()) {
		emit failed(tr("No authorization code found in the pasted "
			       "address."));
		return;
	}
	cancel();
	exchangeCode(code);
}

void LoopbackAuth::onNewConnection()
{
	QTcpSocket *sock = m_server ? m_server->nextPendingConnection()
				    : nullptr;
	if (!sock)
		return;

	connect(sock, &QTcpSocket::readyRead, this, [this, sock]() {
		const QString line =
			QString::fromUtf8(sock->readLine()).trimmed();
		const QString target = requestTarget(line);
		const QString path = target.section('?', 0, 0);

		if (m_ep.implicit) {
			if (!path.startsWith(QStringLiteral("/token"))) {
				// Redirect landing: hand back the JS bridge and
				// keep listening for its /token request.
				writeHttp(sock, bridgeHtml());
				return;
			}
			const QString token =
				queryParam(target,
					   QStringLiteral("access_token"));
			const QString state =
				queryParam(target, QStringLiteral("state"));
			const QString err =
				queryParam(target, QStringLiteral("error"));

			if (token.isEmpty() && err.isEmpty()) {
				// Stray request (e.g. empty fragment); ignore.
				writeHttp(sock, pageHtml(tr("Waiting...")));
				return;
			}
			writeHttp(sock,
				  pageHtml(err.isEmpty() && !token.isEmpty()
						   ? tr("Login complete")
						   : tr("Login failed")));
			cancel();
			if (!err.isEmpty()) {
				emit failed(err);
				return;
			}
			if (!m_state.isEmpty() && state != m_state) {
				emit failed(tr("State mismatch during login."));
				return;
			}
			emit succeeded(token, QString(), 0);
			return;
		}

		// Authorization code flow (Google).
		const QString code = queryParam(target, QStringLiteral("code"));
		const QString state = queryParam(target, QStringLiteral("state"));
		const QString err = queryParam(target, QStringLiteral("error"));

		writeHttp(sock, pageHtml(code.isEmpty() || !err.isEmpty()
						 ? tr("Login failed")
						 : tr("Login complete")));
		cancel();

		if (!err.isEmpty()) {
			emit failed(err);
			return;
		}
		if (code.isEmpty()) {
			emit failed(tr("No authorization code received."));
			return;
		}
		if (!m_state.isEmpty() && state != m_state) {
			emit failed(tr("State mismatch during login."));
			return;
		}
		exchangeCode(code);
	});
}

void LoopbackAuth::exchangeCode(const QString &code)
{
	QNetworkRequest req((QUrl(m_ep.tokenUrl)));
	req.setHeader(QNetworkRequest::ContentTypeHeader,
		      QStringLiteral("application/x-www-form-urlencoded"));

	QUrlQuery body;
	body.addQueryItem(QStringLiteral("client_id"), m_ep.clientId);
	if (!m_ep.clientSecret.isEmpty())
		body.addQueryItem(QStringLiteral("client_secret"),
				  m_ep.clientSecret);
	body.addQueryItem(QStringLiteral("code"), code);
	if (m_ep.usePkce)
		body.addQueryItem(QStringLiteral("code_verifier"),
				  m_codeVerifier);
	body.addQueryItem(QStringLiteral("grant_type"),
			  QStringLiteral("authorization_code"));
	body.addQueryItem(QStringLiteral("redirect_uri"), redirectUri());

	QNetworkReply *reply = m_net->post(
		req, body.toString(QUrl::FullyEncoded).toUtf8());
	connect(reply, &QNetworkReply::finished, this, [this, reply]() {
		reply->deleteLater();
		const QByteArray data = reply->readAll();
		const QJsonObject o = QJsonDocument::fromJson(data).object();

		const QString access =
			o.value(QStringLiteral("access_token")).toString();
		if (access.isEmpty()) {
			QString e =
				o.value(QStringLiteral("error_description"))
					.toString();
			if (e.isEmpty())
				e = o.value(QStringLiteral("error")).toString();
			if (e.isEmpty()) {
				const int http =
					reply->attribute(
						QNetworkRequest::
							HttpStatusCodeAttribute)
						.toInt();
				e = tr("Token request failed (HTTP %1).")
					    .arg(http);
			}
			emit failed(e);
			return;
		}
		const QString refresh =
			o.value(QStringLiteral("refresh_token")).toString();
		const int expires =
			o.value(QStringLiteral("expires_in")).toInt(0);
		emit succeeded(access, refresh, expires);
	});
}

void LoopbackAuth::refresh(const OAuthEndpoints &ep,
			   const QString &refreshToken)
{
	m_ep = ep;

	QNetworkRequest req((QUrl(m_ep.tokenUrl)));
	req.setHeader(QNetworkRequest::ContentTypeHeader,
		      QStringLiteral("application/x-www-form-urlencoded"));

	QUrlQuery body;
	body.addQueryItem(QStringLiteral("client_id"), m_ep.clientId);
	if (!m_ep.clientSecret.isEmpty())
		body.addQueryItem(QStringLiteral("client_secret"),
				  m_ep.clientSecret);
	body.addQueryItem(QStringLiteral("refresh_token"), refreshToken);
	body.addQueryItem(QStringLiteral("grant_type"),
			  QStringLiteral("refresh_token"));

	QNetworkReply *reply = m_net->post(
		req, body.toString(QUrl::FullyEncoded).toUtf8());
	connect(reply, &QNetworkReply::finished, this, [this, reply]() {
		reply->deleteLater();
		const QByteArray data = reply->readAll();
		const QJsonObject o = QJsonDocument::fromJson(data).object();

		const QString access =
			o.value(QStringLiteral("access_token")).toString();
		if (access.isEmpty()) {
			QString e =
				o.value(QStringLiteral("error_description"))
					.toString();
			if (e.isEmpty())
				e = o.value(QStringLiteral("error")).toString();
			if (e.isEmpty()) {
				const int http =
					reply->attribute(
						QNetworkRequest::
							HttpStatusCodeAttribute)
						.toInt();
				e = tr("Token refresh failed (HTTP %1).")
					    .arg(http);
			}
			emit failed(e);
			return;
		}
		const QString refresh =
			o.value(QStringLiteral("refresh_token")).toString();
		const int expires =
			o.value(QStringLiteral("expires_in")).toInt(0);
		emit succeeded(access, refresh, expires);
	});
}
