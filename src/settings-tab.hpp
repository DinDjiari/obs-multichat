#pragma once

#include <QWidget>

class ConfigManager;
class QLineEdit;
class QCheckBox;
class QSpinBox;
class QLabel;
class QPushButton;

// Settings tab: credentials, login buttons, display + alert toggles.
class SettingsTab : public QWidget {
	Q_OBJECT
public:
	explicit SettingsTab(QWidget *parent = nullptr);

	void loadFrom(const ConfigManager &cfg, const QString &youtubeSecret);
	void applyTo(ConfigManager &cfg, QString &youtubeSecretOut) const;

	void setTwitchStatus(const QString &text, bool connected);
	void setYouTubeStatus(const QString &text, bool connected);
	void setTwitchLoggedIn(bool in);
	void setYouTubeLoggedIn(bool in);

	// Hide the Twitch Client ID field (used when an ID is baked in at
	// build time, leaving only the login button).
	void setTwitchManaged(bool managed);
	void setYouTubeManaged(bool managed);

signals:
	void saveRequested();
	void setupRequested();
	void loginTwitchRequested();
	void logoutTwitchRequested();
	void loginYouTubeRequested();
	void logoutYouTubeRequested();

private:
	QLineEdit *m_twitchClientId = nullptr;
	QLabel *m_twitchClientIdLabel = nullptr;
	QLineEdit *m_twitchChannel = nullptr;
	QLineEdit *m_youtubeClientId = nullptr;
	QLabel *m_youtubeClientIdLabel = nullptr;
	QLineEdit *m_youtubeClientSecret = nullptr;
	QLabel *m_youtubeClientSecretLabel = nullptr;

	QLabel *m_twitchStatus = nullptr;
	QLabel *m_youtubeStatus = nullptr;
	QPushButton *m_twitchLoginBtn = nullptr;
	QPushButton *m_youtubeLoginBtn = nullptr;

	QCheckBox *m_showBadges = nullptr;
	QCheckBox *m_showTimestamps = nullptr;
	QSpinBox *m_fontSize = nullptr;

	QCheckBox *m_alertFollow = nullptr;
	QCheckBox *m_alertSub = nullptr;
	QCheckBox *m_alertGift = nullptr;
	QCheckBox *m_alertBits = nullptr;
	QCheckBox *m_alertRaid = nullptr;
	QCheckBox *m_alertMembership = nullptr;
	QCheckBox *m_alertSuperChat = nullptr;
	QCheckBox *m_alertSuperSticker = nullptr;

	bool m_twitchLoggedIn = false;
	bool m_youtubeLoggedIn = false;
};
