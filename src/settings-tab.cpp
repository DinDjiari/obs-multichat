#include "settings-tab.hpp"
#include "config-manager.hpp"

#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>

SettingsTab::SettingsTab(QWidget *parent) : QWidget(parent)
{
	auto *outer = new QVBoxLayout(this);
	outer->setContentsMargins(0, 0, 0, 0);

	auto *scroll = new QScrollArea(this);
	scroll->setWidgetResizable(true);
	scroll->setFrameShape(QFrame::NoFrame);
	outer->addWidget(scroll);

	auto *content = new QWidget(scroll);
	scroll->setWidget(content);
	auto *layout = new QVBoxLayout(content);
	layout->setSpacing(10);

	// --- Twitch group ---
	auto *twitchBox = new QGroupBox(tr("Twitch"), content);
	auto *twitchForm = new QFormLayout(twitchBox);
	m_twitchClientId = new QLineEdit(twitchBox);
	m_twitchClientIdLabel = new QLabel(tr("Client ID:"), twitchBox);
	m_twitchChannel = new QLineEdit(twitchBox);
	twitchForm->addRow(m_twitchClientIdLabel, m_twitchClientId);
	twitchForm->addRow(tr("Channel:"), m_twitchChannel);

	m_twitchStatus = new QLabel(tr("Not connected"), twitchBox);
	auto *twitchBtnRow = new QHBoxLayout();
	m_twitchLoginBtn = new QPushButton(tr("Login with Twitch"), twitchBox);
	twitchBtnRow->addWidget(m_twitchStatus, 1);
	twitchBtnRow->addWidget(m_twitchLoginBtn);
	twitchForm->addRow(twitchBtnRow);
	layout->addWidget(twitchBox);

	// --- YouTube group ---
	auto *ytBox = new QGroupBox(tr("YouTube"), content);
	auto *ytForm = new QFormLayout(ytBox);
	m_youtubeClientId = new QLineEdit(ytBox);
	m_youtubeClientSecret = new QLineEdit(ytBox);
	m_youtubeClientSecret->setEchoMode(QLineEdit::Password);
	m_youtubeClientIdLabel = new QLabel(tr("Client ID:"), ytBox);
	m_youtubeClientSecretLabel = new QLabel(tr("Client Secret:"), ytBox);
	ytForm->addRow(m_youtubeClientIdLabel, m_youtubeClientId);
	ytForm->addRow(m_youtubeClientSecretLabel, m_youtubeClientSecret);

	m_youtubeStatus = new QLabel(tr("Not connected"), ytBox);
	auto *ytBtnRow = new QHBoxLayout();
	m_youtubeLoginBtn = new QPushButton(tr("Login with YouTube"), ytBox);
	ytBtnRow->addWidget(m_youtubeStatus, 1);
	ytBtnRow->addWidget(m_youtubeLoginBtn);
	ytForm->addRow(ytBtnRow);
	layout->addWidget(ytBox);

	// --- Display group ---
	auto *dispBox = new QGroupBox(tr("Display"), content);
	auto *dispForm = new QFormLayout(dispBox);
	m_showBadges = new QCheckBox(tr("Show platform badges"), dispBox);
	m_showTimestamps = new QCheckBox(tr("Show timestamps"), dispBox);
	m_fontSize = new QSpinBox(dispBox);
	m_fontSize->setRange(8, 24);
	dispForm->addRow(m_showBadges);
	dispForm->addRow(m_showTimestamps);
	dispForm->addRow(tr("Font size:"), m_fontSize);
	layout->addWidget(dispBox);

	// --- Alerts group ---
	auto *alertBox = new QGroupBox(tr("Enabled Alerts"), content);
	auto *alertLayout = new QVBoxLayout(alertBox);
	m_alertFollow = new QCheckBox(tr("Twitch Follow"), alertBox);
	m_alertSub = new QCheckBox(tr("Twitch Subscription"), alertBox);
	m_alertGift = new QCheckBox(tr("Twitch Gift Subs"), alertBox);
	m_alertBits = new QCheckBox(tr("Twitch Bits / Cheers"), alertBox);
	m_alertRaid = new QCheckBox(tr("Twitch Raids"), alertBox);
	m_alertMembership = new QCheckBox(tr("YouTube Membership"), alertBox);
	m_alertSuperChat = new QCheckBox(tr("YouTube Super Chat"), alertBox);
	m_alertSuperSticker =
		new QCheckBox(tr("YouTube Super Sticker"), alertBox);
	for (QCheckBox *cb :
	     {m_alertFollow, m_alertSub, m_alertGift, m_alertBits, m_alertRaid,
	      m_alertMembership, m_alertSuperChat, m_alertSuperSticker})
		alertLayout->addWidget(cb);
	layout->addWidget(alertBox);

	// --- Save ---
	auto *setupBtn = new QPushButton(tr("Setup wizard..."), content);
	layout->addWidget(setupBtn);
	auto *saveBtn = new QPushButton(tr("Save settings"), content);
	layout->addWidget(saveBtn);
	layout->addStretch(1);

	connect(setupBtn, &QPushButton::clicked, this,
		&SettingsTab::setupRequested);
	connect(saveBtn, &QPushButton::clicked, this,
		&SettingsTab::saveRequested);
	connect(m_twitchLoginBtn, &QPushButton::clicked, this, [this]() {
		if (m_twitchLoggedIn)
			emit logoutTwitchRequested();
		else
			emit loginTwitchRequested();
	});
	connect(m_youtubeLoginBtn, &QPushButton::clicked, this, [this]() {
		if (m_youtubeLoggedIn)
			emit logoutYouTubeRequested();
		else
			emit loginYouTubeRequested();
	});
}

void SettingsTab::loadFrom(const ConfigManager &cfg,
			   const QString &youtubeSecret)
{
	m_twitchClientId->setText(cfg.twitchClientId);
	m_twitchChannel->setText(cfg.twitchChannel);
	m_youtubeClientId->setText(cfg.youtubeClientId);
	m_youtubeClientSecret->setText(youtubeSecret);

	m_showBadges->setChecked(cfg.showBadges);
	m_showTimestamps->setChecked(cfg.showTimestamps);
	m_fontSize->setValue(cfg.fontSize);

	m_alertFollow->setChecked(cfg.alertFollow);
	m_alertSub->setChecked(cfg.alertSub);
	m_alertGift->setChecked(cfg.alertGift);
	m_alertBits->setChecked(cfg.alertBits);
	m_alertRaid->setChecked(cfg.alertRaid);
	m_alertMembership->setChecked(cfg.alertMembership);
	m_alertSuperChat->setChecked(cfg.alertSuperChat);
	m_alertSuperSticker->setChecked(cfg.alertSuperSticker);
}

void SettingsTab::applyTo(ConfigManager &cfg, QString &youtubeSecretOut) const
{
	cfg.twitchClientId = m_twitchClientId->text().trimmed();
	cfg.twitchChannel = m_twitchChannel->text().trimmed();
	cfg.youtubeClientId = m_youtubeClientId->text().trimmed();
	youtubeSecretOut = m_youtubeClientSecret->text().trimmed();

	cfg.showBadges = m_showBadges->isChecked();
	cfg.showTimestamps = m_showTimestamps->isChecked();
	cfg.fontSize = m_fontSize->value();

	cfg.alertFollow = m_alertFollow->isChecked();
	cfg.alertSub = m_alertSub->isChecked();
	cfg.alertGift = m_alertGift->isChecked();
	cfg.alertBits = m_alertBits->isChecked();
	cfg.alertRaid = m_alertRaid->isChecked();
	cfg.alertMembership = m_alertMembership->isChecked();
	cfg.alertSuperChat = m_alertSuperChat->isChecked();
	cfg.alertSuperSticker = m_alertSuperSticker->isChecked();
}

void SettingsTab::setTwitchManaged(bool managed)
{
	if (m_twitchClientIdLabel)
		m_twitchClientIdLabel->setVisible(!managed);
	m_twitchClientId->setVisible(!managed);
}

void SettingsTab::setYouTubeManaged(bool managed)
{
	if (m_youtubeClientIdLabel)
		m_youtubeClientIdLabel->setVisible(!managed);
	if (m_youtubeClientSecretLabel)
		m_youtubeClientSecretLabel->setVisible(!managed);
	m_youtubeClientId->setVisible(!managed);
	m_youtubeClientSecret->setVisible(!managed);
}

void SettingsTab::setTwitchStatus(const QString &text, bool connected)
{
	m_twitchStatus->setText(text);
	m_twitchStatus->setStyleSheet(
		connected ? QStringLiteral("color:#00c853;")
			  : QStringLiteral("color:#9aa0a6;"));
}

void SettingsTab::setYouTubeStatus(const QString &text, bool connected)
{
	m_youtubeStatus->setText(text);
	m_youtubeStatus->setStyleSheet(
		connected ? QStringLiteral("color:#00c853;")
			  : QStringLiteral("color:#9aa0a6;"));
}

void SettingsTab::setTwitchLoggedIn(bool in)
{
	m_twitchLoggedIn = in;
	m_twitchLoginBtn->setText(in ? tr("Logout") : tr("Login with Twitch"));
}

void SettingsTab::setYouTubeLoggedIn(bool in)
{
	m_youtubeLoggedIn = in;
	m_youtubeLoginBtn->setText(in ? tr("Logout")
				      : tr("Login with YouTube"));
}
