#pragma once

#include <QString>

// Endpoint + credential description for one provider's OAuth flow.
struct OAuthEndpoints {
	QString authUrl;       // authorization endpoint
	QString tokenUrl;      // token endpoint
	QString clientId;
	QString clientSecret;  // required for Twitch; used by Google too
	QString scope;         // space separated scope list

	bool usePkce = false;  // Google: true, Twitch: false (unsupported)
	bool implicit = false; // Twitch: true (token in fragment, no secret)

	// Loopback redirect target. Google accepts any port on 127.0.0.1;
	// Twitch requires an exactly registered URI, so a fixed port + the
	// "localhost" host is used there.
	QString loopbackHost = QStringLiteral("127.0.0.1");
	int loopbackPort = 0;  // 0 = pick a free ephemeral port
};
