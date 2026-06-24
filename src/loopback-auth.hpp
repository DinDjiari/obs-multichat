#pragma once

#include <QObject>
#include <QString>

#include "oauth-endpoints.hpp"

class QTcpServer;
class QNetworkAccessManager;

// OAuth 2.0 login via a transient loopback listener (RFC 8252). Clicking login
// opens the system browser; after the user approves, the provider redirects to
// http://<host>:<port>, captured here to complete the flow.
//
// Supports the authorization code flow (+PKCE, Google) and the implicit flow
// (Twitch, token in the URL fragment, no client secret). Listens on both IPv4
// and IPv6 loopback so "localhost" works regardless of how it resolves. If the
// automatic path fails or stalls, manualFallback() lets the user finish by
// pasting the redirect URL.
class LoopbackAuth : public QObject {
	Q_OBJECT
public:
	explicit LoopbackAuth(QObject *parent = nullptr);
	~LoopbackAuth() override;

	void start(const OAuthEndpoints &ep);
	void refresh(const OAuthEndpoints &ep, const QString &refreshToken);
	void completeFromRedirect(const QString &pastedUrl);
	void cancel();

signals:
	void succeeded(const QString &accessToken, const QString &refreshToken,
		       int expiresIn);
	void failed(const QString &error);
	void manualFallback(const QString &authUrl, bool needPaste);

private:
	void onNewConnection();
	void exchangeCode(const QString &code);
	void fail(const QString &error);
	QString redirectUri() const;
	QString buildAuthUrl();

	OAuthEndpoints m_ep;
	QTcpServer *m_server = nullptr;
	QNetworkAccessManager *m_net = nullptr;

	QString m_codeVerifier;
	QString m_state;
	QString m_redirectUri;
	int m_port = 0;
};
