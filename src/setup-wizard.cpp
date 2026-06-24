#include "setup-wizard.hpp"
#include "theme.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QDesktopServices>
#include <QUrl>

SetupWizard::SetupWizard(QWidget *parent) : QDialog(parent)
{
	setWindowTitle(tr("Multichat Setup"));
	setStyleSheet(mc::darkStyle());
	resize(540, 620);

	auto *root = new QVBoxLayout(this);

	auto *intro = new QLabel(
		tr("To log in, Multichat needs your own free app credentials "
		   "from Twitch and Google. This is a one-time setup. Use the "
		   "buttons below to open each console, then paste the values "
		   "here."),
		this);
	intro->setWordWrap(true);
	root->addWidget(intro);

	// --- Twitch ---
	auto *twBox = new QGroupBox(tr("Twitch"), this);
	auto *twLayout = new QVBoxLayout(twBox);

	auto *twOpen = new QPushButton(tr("1. Open Twitch Developer Console"),
				       twBox);
	connect(twOpen, &QPushButton::clicked, this, []() {
		QDesktopServices::openUrl(
			QUrl(QStringLiteral("https://dev.twitch.tv/console/apps")));
	});
	twLayout->addWidget(twOpen);

	auto *twHelp = new QLabel(
		tr("Register an application: Client type \"Public\", OAuth "
		   "Redirect URL exactly http://localhost:3000 . Then copy the "
		   "Client ID below. Channel = your login name (lowercase)."),
		twBox);
	twHelp->setWordWrap(true);
	twLayout->addWidget(twHelp);

	auto *twForm = new QFormLayout();
	m_twId = new QLineEdit(twBox);
	m_twChannel = new QLineEdit(twBox);
	twForm->addRow(tr("Client ID:"), m_twId);
	twForm->addRow(tr("Channel:"), m_twChannel);
	twLayout->addLayout(twForm);
	root->addWidget(twBox);

	// --- YouTube ---
	auto *ytBox = new QGroupBox(tr("YouTube"), this);
	auto *ytLayout = new QVBoxLayout(ytBox);

	auto *ytOpen = new QPushButton(tr("2. Open Google Cloud Console"),
				       ytBox);
	connect(ytOpen, &QPushButton::clicked, this, []() {
		QDesktopServices::openUrl(
			QUrl(QStringLiteral("https://console.cloud.google.com/")));
	});
	ytLayout->addWidget(ytOpen);

	auto *ytHelp = new QLabel(
		tr("Create a project, enable \"YouTube Data API v3\", configure "
		   "the OAuth consent screen (External, add yourself as a test "
		   "user), then create an OAuth client of type \"Desktop app\". "
		   "Copy its Client ID and Secret below."),
		ytBox);
	ytHelp->setWordWrap(true);
	ytLayout->addWidget(ytHelp);

	auto *ytForm = new QFormLayout();
	m_ytId = new QLineEdit(ytBox);
	m_ytSecret = new QLineEdit(ytBox);
	m_ytSecret->setEchoMode(QLineEdit::Password);
	ytForm->addRow(tr("Client ID:"), m_ytId);
	ytForm->addRow(tr("Client Secret:"), m_ytSecret);
	ytLayout->addLayout(ytForm);
	root->addWidget(ytBox);

	auto *note = new QLabel(
		tr("You can leave one platform empty if you only use the "
		   "other. Values are saved to "
		   "~/.config/obs-multichat/credentials.env"),
		this);
	note->setWordWrap(true);
	note->setStyleSheet(QStringLiteral("color:#9aa0a6;"));
	root->addWidget(note);

	root->addStretch(1);

	auto *buttons = new QDialogButtonBox(
		QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
	buttons->button(QDialogButtonBox::Save)->setText(tr("Save"));
	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	root->addWidget(buttons);
}

void SetupWizard::setValues(const QString &twId, const QString &twChannel,
			    const QString &ytId, const QString &ytSecret)
{
	m_twId->setText(twId);
	m_twChannel->setText(twChannel);
	m_ytId->setText(ytId);
	m_ytSecret->setText(ytSecret);
}

QString SetupWizard::twitchClientId() const
{
	return m_twId->text().trimmed();
}

QString SetupWizard::twitchChannel() const
{
	return m_twChannel->text().trimmed();
}

QString SetupWizard::youtubeClientId() const
{
	return m_ytId->text().trimmed();
}

QString SetupWizard::youtubeClientSecret() const
{
	return m_ytSecret->text().trimmed();
}
