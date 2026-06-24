#pragma once

#include <QDialog>

class QLineEdit;

// First-run setup helper. Has buttons that open the Twitch / Google consoles
// and fields for the resulting client credentials, which the core writes to
// ~/.config/obs-multichat/credentials.env.
class SetupWizard : public QDialog {
	Q_OBJECT
public:
	explicit SetupWizard(QWidget *parent = nullptr);

	void setValues(const QString &twId, const QString &twChannel,
		       const QString &ytId, const QString &ytSecret);

	QString twitchClientId() const;
	QString twitchChannel() const;
	QString youtubeClientId() const;
	QString youtubeClientSecret() const;

private:
	QLineEdit *m_twId = nullptr;
	QLineEdit *m_twChannel = nullptr;
	QLineEdit *m_ytId = nullptr;
	QLineEdit *m_ytSecret = nullptr;
};
