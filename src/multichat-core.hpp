#pragma once

#include <QObject>
#include <QPointer>
#include "config-manager.hpp"
#include "chat-types.hpp"

class QWidget;
class ChatTab;
class AlertsTab;
class SettingsDialog;
class TwitchClient;
class YouTubeClient;
class LoopbackAuth;

// Owns the platform clients, OAuth flows and configuration, and produces the
// widgets hosted by OBS: the chat dock, the alerts dock and the settings
// window (opened from the Tools menu).
class MultichatCore : public QObject {
	Q_OBJECT
public:
	explicit MultichatCore(QWidget *mainWindow);
	~MultichatCore() override;

	QWidget *createChatWidget();
	QWidget *createAlertsWidget();
	void openSettings();
	void openSetupWizard();

private slots:
	void onSaveSettings();
	void onLoginTwitch();
	void onLogoutTwitch();
	void onLoginYouTube();
	void onLogoutYouTube();

	void onChatMessage(const ChatMessage &m);
	void onAlert(const Alert &a);

private:
	void ensureSettingsDialog();
	void wireSettings();
	void applyConfigFromTab();
	void writeCredentialsFile(const QString &tw, const QString &yt,
				  const QString &ytSecret);
	void maybeAutoSetup();
	QString effectiveTwitchClientId() const;
	QString effectiveYouTubeClientId() const;
	QString effectiveYouTubeClientSecret() const;
	void showManualLogin(LoopbackAuth *auth, const QString &authUrl,
			     bool needPaste, const QString &platform);
	void applyDisplaySettings();
	void refreshLoginState();
	void startupConnect();
	void connectTwitch();
	void connectYouTube();

	QWidget *m_main = nullptr;
	ConfigManager m_config;

	QPointer<ChatTab> m_chatTab;
	QPointer<AlertsTab> m_alertsTab;
	QPointer<SettingsDialog> m_settings;

	TwitchClient *m_twitch = nullptr;
	YouTubeClient *m_youtube = nullptr;
	LoopbackAuth *m_twitchAuth = nullptr;
	LoopbackAuth *m_youtubeAuth = nullptr;
};
