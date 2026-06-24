#include "multichat-core.hpp"

#include "chat-tab.hpp"
#include "alerts-tab.hpp"
#include "settings-tab.hpp"
#include "settings-dialog.hpp"
#include "setup-wizard.hpp"
#include "twitch-client.hpp"
#include "youtube-client.hpp"
#include "loopback-auth.hpp"
#include "oauth-endpoints.hpp"
#include "token-store.hpp"
#include "theme.hpp"

#include <QMessageBox>
#include <QInputDialog>
#include <QLineEdit>
#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QDir>
#include <QSaveFile>
#include <QTimer>

#ifndef MULTICHAT_TWITCH_CLIENT_ID
#define MULTICHAT_TWITCH_CLIENT_ID ""
#endif
#ifndef MULTICHAT_YOUTUBE_CLIENT_ID
#define MULTICHAT_YOUTUBE_CLIENT_ID ""
#endif
#ifndef MULTICHAT_YOUTUBE_CLIENT_SECRET
#define MULTICHAT_YOUTUBE_CLIENT_SECRET ""
#endif

namespace {

// Read a KEY=VALUE entry from ~/.config/obs-multichat/credentials.env at
// runtime, so credentials work without a rebuild and regardless of whether the
// build baked them in.
QString credFileValue(const QString &key)
{
	QFile f(ConfigManager::configDir() +
		QStringLiteral("/credentials.env"));
	if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
		return {};
	while (!f.atEnd()) {
		const QString line = QString::fromUtf8(f.readLine()).trimmed();
		if (line.isEmpty() || line.startsWith('#'))
			continue;
		const int eq = line.indexOf('=');
		if (eq <= 0)
			continue;
		if (line.left(eq).trimmed() == key) {
			QString v = line.mid(eq + 1).trimmed();
			if (v.startsWith('"') && v.endsWith('"') && v.size() >= 2)
				v = v.mid(1, v.size() - 2);
			return v;
		}
	}
	return {};
}

QString builtinTwitchClientId()
{
	const QString rt = credFileValue(QStringLiteral("TWITCH_CLIENT_ID"));
	if (!rt.isEmpty())
		return rt;
	return QString::fromUtf8(MULTICHAT_TWITCH_CLIENT_ID).trimmed();
}

QString builtinYouTubeClientId()
{
	const QString rt = credFileValue(QStringLiteral("YOUTUBE_CLIENT_ID"));
	if (!rt.isEmpty())
		return rt;
	return QString::fromUtf8(MULTICHAT_YOUTUBE_CLIENT_ID).trimmed();
}

QString builtinYouTubeClientSecret()
{
	const QString rt =
		credFileValue(QStringLiteral("YOUTUBE_CLIENT_SECRET"));
	if (!rt.isEmpty())
		return rt;
	return QString::fromUtf8(MULTICHAT_YOUTUBE_CLIENT_SECRET).trimmed();
}

OAuthEndpoints twitchEndpoints(const QString &clientId)
{
	OAuthEndpoints ep;
	ep.authUrl = QStringLiteral("https://id.twitch.tv/oauth2/authorize");
	ep.tokenUrl = QStringLiteral("https://id.twitch.tv/oauth2/token");
	ep.clientId = clientId;
	ep.scope = QStringLiteral(
		"chat:read chat:edit bits:read channel:read:subscriptions "
		"moderator:read:followers");
	ep.usePkce = false;   // Twitch does not support PKCE
	ep.implicit = true;   // token in URL fragment, no client secret needed
	ep.loopbackHost = QStringLiteral("localhost");
	ep.loopbackPort = 3000; // must match the registered redirect URI
	return ep;
}

OAuthEndpoints youtubeEndpoints(const QString &clientId,
				const QString &clientSecret)
{
	OAuthEndpoints ep;
	ep.authUrl =
		QStringLiteral("https://accounts.google.com/o/oauth2/v2/auth");
	ep.tokenUrl = QStringLiteral("https://oauth2.googleapis.com/token");
	ep.clientId = clientId;
	ep.clientSecret = clientSecret;
	ep.scope = QStringLiteral(
		"https://www.googleapis.com/auth/youtube.readonly");
	ep.usePkce = true;
	ep.loopbackHost = QStringLiteral("127.0.0.1");
	ep.loopbackPort = 0; // Google allows any loopback port
	return ep;
}

} // namespace

MultichatCore::MultichatCore(QWidget *mainWindow)
	: QObject(mainWindow), m_main(mainWindow)
{
	m_config.load();

	m_twitch = new TwitchClient(this);
	m_youtube = new YouTubeClient(this);
	m_twitchAuth = new LoopbackAuth(this);
	m_youtubeAuth = new LoopbackAuth(this);

	// Client wiring.
	connect(m_twitch, &TwitchClient::chatMessage, this,
		&MultichatCore::onChatMessage);
	connect(m_twitch, &TwitchClient::alert, this, &MultichatCore::onAlert);
	connect(m_twitch, &TwitchClient::statusChanged, this,
		[this](const QString &s, bool c) {
			if (m_settings)
				m_settings->tab()->setTwitchStatus(s, c);
		});
	connect(m_twitch, &TwitchClient::authError, this, [this]() {
		if (m_settings)
			m_settings->tab()->setTwitchStatus(
				tr("Auth error - re-login"), false);
	});

	connect(m_youtube, &YouTubeClient::chatMessage, this,
		&MultichatCore::onChatMessage);
	connect(m_youtube, &YouTubeClient::alert, this,
		&MultichatCore::onAlert);
	connect(m_youtube, &YouTubeClient::statusChanged, this,
		[this](const QString &s, bool c) {
			if (m_settings)
				m_settings->tab()->setYouTubeStatus(s, c);
		});
	connect(m_youtube, &YouTubeClient::authError, this, [this]() {
		if (m_settings)
			m_settings->tab()->setYouTubeStatus(
				tr("Auth error - re-login"), false);
	});

	// Twitch OAuth (loopback, auth code + client secret).
	connect(m_twitchAuth, &LoopbackAuth::succeeded, this,
		[this](const QString &access, const QString &refresh, int) {
			TokenStore::set(TokenKeys::TwitchAccess, access);
			if (!refresh.isEmpty())
				TokenStore::set(TokenKeys::TwitchRefresh,
						refresh);
			refreshLoginState();
			connectTwitch();
		});
	connect(m_twitchAuth, &LoopbackAuth::failed, this,
		[this](const QString &err) {
			if (m_settings)
				m_settings->tab()->setTwitchStatus(err, false);
		});
	connect(m_twitchAuth, &LoopbackAuth::manualFallback, this,
		[this](const QString &url, bool needPaste) {
			showManualLogin(m_twitchAuth, url, needPaste,
					QStringLiteral("Twitch"));
		});

	// YouTube OAuth (loopback, auth code + PKCE).
	connect(m_youtubeAuth, &LoopbackAuth::succeeded, this,
		[this](const QString &access, const QString &refresh, int) {
			TokenStore::set(TokenKeys::YouTubeAccess, access);
			if (!refresh.isEmpty())
				TokenStore::set(TokenKeys::YouTubeRefresh,
						refresh);
			refreshLoginState();
			connectYouTube();
		});
	connect(m_youtubeAuth, &LoopbackAuth::failed, this,
		[this](const QString &err) {
			if (m_settings)
				m_settings->tab()->setYouTubeStatus(err, false);
		});
	connect(m_youtubeAuth, &LoopbackAuth::manualFallback, this,
		[this](const QString &url, bool needPaste) {
			showManualLogin(m_youtubeAuth, url, needPaste,
					QStringLiteral("YouTube"));
		});

	startupConnect();
	maybeAutoSetup();
}

MultichatCore::~MultichatCore()
{
	if (m_settings)
		m_settings->deleteLater();
}

QWidget *MultichatCore::createChatWidget()
{
	if (!m_chatTab) {
		m_chatTab = new ChatTab(m_main);
		m_chatTab->setStyleSheet(mc::darkStyle());
		connect(m_chatTab, &ChatTab::settingsRequested, this,
			&MultichatCore::openSettings);
		applyDisplaySettings();
	}
	return m_chatTab;
}

QWidget *MultichatCore::createAlertsWidget()
{
	if (!m_alertsTab) {
		m_alertsTab = new AlertsTab(m_main);
		m_alertsTab->setStyleSheet(mc::darkStyle());
	}
	return m_alertsTab;
}

void MultichatCore::ensureSettingsDialog()
{
	if (m_settings)
		return;
	m_settings = new SettingsDialog(m_main);
	wireSettings();
}

void MultichatCore::wireSettings()
{
	SettingsTab *t = m_settings->tab();
	connect(t, &SettingsTab::saveRequested, this,
		&MultichatCore::onSaveSettings);
	connect(t, &SettingsTab::setupRequested, this,
		&MultichatCore::openSetupWizard);
	connect(t, &SettingsTab::loginTwitchRequested, this,
		&MultichatCore::onLoginTwitch);
	connect(t, &SettingsTab::logoutTwitchRequested, this,
		&MultichatCore::onLogoutTwitch);
	connect(t, &SettingsTab::loginYouTubeRequested, this,
		&MultichatCore::onLoginYouTube);
	connect(t, &SettingsTab::logoutYouTubeRequested, this,
		&MultichatCore::onLogoutYouTube);

	const QString ytSecret =
		TokenStore::get(TokenKeys::YouTubeClientSecret);
	t->loadFrom(m_config, ytSecret);
	if (!builtinTwitchClientId().isEmpty())
		t->setTwitchManaged(true);
	if (!builtinYouTubeClientId().isEmpty())
		t->setYouTubeManaged(true);
	refreshLoginState();
}

QString MultichatCore::effectiveTwitchClientId() const
{
	const QString builtin = builtinTwitchClientId();
	return builtin.isEmpty() ? m_config.twitchClientId : builtin;
}

QString MultichatCore::effectiveYouTubeClientId() const
{
	const QString builtin = builtinYouTubeClientId();
	return builtin.isEmpty() ? m_config.youtubeClientId : builtin;
}

QString MultichatCore::effectiveYouTubeClientSecret() const
{
	const QString builtin = builtinYouTubeClientSecret();
	return builtin.isEmpty()
		       ? TokenStore::get(TokenKeys::YouTubeClientSecret)
		       : builtin;
}

void MultichatCore::applyConfigFromTab()
{
	if (!m_settings)
		return;
	QString ytSecret;
	m_settings->tab()->applyTo(m_config, ytSecret);
	m_config.save();
	if (ytSecret.isEmpty())
		TokenStore::clear(TokenKeys::YouTubeClientSecret);
	else
		TokenStore::set(TokenKeys::YouTubeClientSecret, ytSecret);
	applyDisplaySettings();
}

void MultichatCore::openSettings()
{
	ensureSettingsDialog();
	m_settings->show();
	m_settings->raise();
	m_settings->activateWindow();
}

void MultichatCore::writeCredentialsFile(const QString &tw, const QString &yt,
					 const QString &ytSecret)
{
	QDir().mkpath(ConfigManager::configDir());
	const QString path =
		ConfigManager::configDir() + QStringLiteral("/credentials.env");
	QSaveFile f(path);
	if (!f.open(QIODevice::WriteOnly))
		return;
	const QString out =
		QStringLiteral("TWITCH_CLIENT_ID=%1\n"
			       "YOUTUBE_CLIENT_ID=%2\n"
			       "YOUTUBE_CLIENT_SECRET=%3\n")
			.arg(tw.trimmed(), yt.trimmed(), ytSecret.trimmed());
	f.write(out.toUtf8());
	f.commit();
	QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner);
}

void MultichatCore::openSetupWizard()
{
	// Mark that the wizard has been seen so it won't auto-open again.
	QDir().mkpath(ConfigManager::configDir());
	QFile marker(ConfigManager::configDir() +
		     QStringLiteral("/.setup_seen"));
	if (marker.open(QIODevice::WriteOnly))
		marker.close();

	SetupWizard w(m_main);
	w.setValues(credFileValue(QStringLiteral("TWITCH_CLIENT_ID")),
		    m_config.twitchChannel,
		    credFileValue(QStringLiteral("YOUTUBE_CLIENT_ID")),
		    credFileValue(QStringLiteral("YOUTUBE_CLIENT_SECRET")));
	if (w.exec() != QDialog::Accepted)
		return;

	writeCredentialsFile(w.twitchClientId(), w.youtubeClientId(),
			     w.youtubeClientSecret());
	m_config.twitchChannel = w.twitchChannel();
	m_config.save();

	// Reflect new state in the settings window if it exists.
	if (m_settings) {
		m_settings->tab()->setTwitchManaged(
			!builtinTwitchClientId().isEmpty());
		m_settings->tab()->setYouTubeManaged(
			!builtinYouTubeClientId().isEmpty());
		m_settings->tab()->loadFrom(m_config,
					    effectiveYouTubeClientSecret());
	}

	QMessageBox::information(
		m_main, tr("Multichat"),
		tr("Setup saved. Open Multichat Settings and click Login to "
		   "connect."));
}

void MultichatCore::maybeAutoSetup()
{
	// Auto-open the wizard once if nothing is configured yet.
	if (!builtinTwitchClientId().isEmpty() ||
	    !builtinYouTubeClientId().isEmpty())
		return; // already have credentials
	if (QFile::exists(ConfigManager::configDir() +
			  QStringLiteral("/.setup_seen")))
		return; // user already saw the wizard
	QTimer::singleShot(1500, this, [this]() { openSetupWizard(); });
}

void MultichatCore::showManualLogin(LoopbackAuth *auth, const QString &authUrl,
				    bool needPaste, const QString &platform)
{
	QApplication::clipboard()->setText(authUrl);

	if (!needPaste) {
		// The local listener is running; the user only needs to open
		// the URL. The redirect will then be captured automatically.
		QMessageBox::information(
			m_settings, tr("%1 login").arg(platform),
			tr("The browser could not be opened automatically.\n\n"
			   "The login link has been copied to your clipboard. "
			   "Open it in your browser and approve access - the "
			   "login then completes by itself."));
		return;
	}

	// No listener (port busy): the user must paste the redirect URL back.
	QMessageBox::information(
		m_settings, tr("%1 login (manual)").arg(platform),
		tr("Automatic login is unavailable (local port 3000 is in "
		   "use).\n\n"
		   "1. The login link was copied to your clipboard - open it "
		   "in your browser.\n"
		   "2. Approve access. The browser will land on a "
		   "'localhost' page that may show an error - that is fine.\n"
		   "3. Copy the FULL address from the browser's address bar "
		   "and paste it in the next dialog."));

	bool ok = false;
	const QString pasted = QInputDialog::getText(
		m_settings, tr("%1 login").arg(platform),
		tr("Paste the full localhost address:"), QLineEdit::Normal,
		QString(), &ok);
	if (ok && !pasted.trimmed().isEmpty())
		auth->completeFromRedirect(pasted);
}

void MultichatCore::refreshLoginState()
{
	if (!m_settings)
		return;
	m_settings->tab()->setTwitchLoggedIn(
		!TokenStore::get(TokenKeys::TwitchRefresh).isEmpty() ||
		!TokenStore::get(TokenKeys::TwitchAccess).isEmpty());
	m_settings->tab()->setYouTubeLoggedIn(
		!TokenStore::get(TokenKeys::YouTubeRefresh).isEmpty() ||
		!TokenStore::get(TokenKeys::YouTubeAccess).isEmpty());
}

void MultichatCore::applyDisplaySettings()
{
	if (!m_chatTab)
		return;
	m_chatTab->setShowBadges(m_config.showBadges);
	m_chatTab->setShowTimestamps(m_config.showTimestamps);
	m_chatTab->setFontSize(m_config.fontSize);
}

void MultichatCore::startupConnect()
{
	// Twitch uses the implicit flow (no refresh token); reuse the stored
	// access token until it expires, then the user re-logs in.
	if (!TokenStore::get(TokenKeys::TwitchAccess).isEmpty())
		connectTwitch();

	const QString ytRefresh = TokenStore::get(TokenKeys::YouTubeRefresh);
	if (!effectiveYouTubeClientId().isEmpty() && !ytRefresh.isEmpty()) {
		m_youtubeAuth->refresh(
			youtubeEndpoints(effectiveYouTubeClientId(),
					 effectiveYouTubeClientSecret()),
			ytRefresh);
	} else if (!TokenStore::get(TokenKeys::YouTubeAccess).isEmpty()) {
		connectYouTube();
	}
}

void MultichatCore::connectTwitch()
{
	const QString token = TokenStore::get(TokenKeys::TwitchAccess);
	const QString cid = effectiveTwitchClientId();
	if (token.isEmpty() || cid.isEmpty() ||
	    m_config.twitchChannel.isEmpty())
		return;
	m_twitch->setCredentials(cid, token, m_config.twitchChannel);
	m_twitch->connectChat();
}

void MultichatCore::connectYouTube()
{
	const QString token = TokenStore::get(TokenKeys::YouTubeAccess);
	if (token.isEmpty())
		return;
	m_youtube->setCredentials(token);
	m_youtube->connectChat();
}

void MultichatCore::onSaveSettings()
{
	if (!m_settings)
		return;
	applyConfigFromTab();
	QMessageBox::information(m_settings, tr("Multichat"),
				 tr("Settings saved."));
}

void MultichatCore::onLoginTwitch()
{
	applyConfigFromTab(); // use whatever is currently typed in
	const QString cid = effectiveTwitchClientId();
	if (cid.isEmpty()) {
		QMessageBox::warning(
			m_settings, tr("Multichat"),
			tr("Enter a Twitch Client ID first."));
		return;
	}
	m_twitchAuth->start(twitchEndpoints(cid));
}

void MultichatCore::onLogoutTwitch()
{
	m_twitch->disconnectAll();
	TokenStore::clear(TokenKeys::TwitchAccess);
	TokenStore::clear(TokenKeys::TwitchRefresh);
	refreshLoginState();
	if (m_settings)
		m_settings->tab()->setTwitchStatus(tr("Logged out"), false);
}

void MultichatCore::onLoginYouTube()
{
	applyConfigFromTab(); // use whatever is currently typed in
	const QString cid = effectiveYouTubeClientId();
	if (cid.isEmpty()) {
		QMessageBox::warning(
			m_settings, tr("Multichat"),
			tr("Enter a YouTube Client ID first."));
		return;
	}
	m_youtubeAuth->start(
		youtubeEndpoints(cid, effectiveYouTubeClientSecret()));
}

void MultichatCore::onLogoutYouTube()
{
	m_youtube->disconnectAll();
	TokenStore::clear(TokenKeys::YouTubeAccess);
	TokenStore::clear(TokenKeys::YouTubeRefresh);
	refreshLoginState();
	if (m_settings)
		m_settings->tab()->setYouTubeStatus(tr("Logged out"), false);
}

void MultichatCore::onChatMessage(const ChatMessage &m)
{
	if (m_chatTab)
		m_chatTab->addMessage(m);
}

void MultichatCore::onAlert(const Alert &a)
{
	bool enabled = true;
	switch (a.type) {
	case AlertType::TwitchFollow:
		enabled = m_config.alertFollow;
		break;
	case AlertType::TwitchSub:
		enabled = m_config.alertSub;
		break;
	case AlertType::TwitchGiftSub:
		enabled = m_config.alertGift;
		break;
	case AlertType::TwitchBits:
		enabled = m_config.alertBits;
		break;
	case AlertType::TwitchRaid:
		enabled = m_config.alertRaid;
		break;
	case AlertType::YouTubeMembership:
		enabled = m_config.alertMembership;
		break;
	case AlertType::YouTubeSuperChat:
		enabled = m_config.alertSuperChat;
		break;
	case AlertType::YouTubeSuperSticker:
		enabled = m_config.alertSuperSticker;
		break;
	}
	if (enabled && m_alertsTab)
		m_alertsTab->addAlert(a);
}
