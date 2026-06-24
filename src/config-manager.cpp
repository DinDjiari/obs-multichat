#include "config-manager.hpp"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

ConfigManager::ConfigManager() = default;

QString ConfigManager::configDir()
{
	QString dir = QDir::homePath() + QStringLiteral("/.config/obs-multichat");
	QDir().mkpath(dir);
	return dir;
}

QString ConfigManager::configFilePath()
{
	return configDir() + QStringLiteral("/config.json");
}

void ConfigManager::load()
{
	QFile f(configFilePath());
	if (!f.open(QIODevice::ReadOnly))
		return;

	const QByteArray data = f.readAll();
	f.close();

	QJsonParseError err{};
	const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
	if (err.error != QJsonParseError::NoError || !doc.isObject())
		return;

	const QJsonObject o = doc.object();

	twitchClientId = o.value(QStringLiteral("twitch_client_id")).toString(twitchClientId);
	twitchChannel = o.value(QStringLiteral("twitch_channel")).toString(twitchChannel);
	youtubeClientId = o.value(QStringLiteral("youtube_client_id")).toString(youtubeClientId);

	showBadges = o.value(QStringLiteral("show_badges")).toBool(showBadges);
	showTimestamps = o.value(QStringLiteral("show_timestamps")).toBool(showTimestamps);
	fontSize = o.value(QStringLiteral("font_size")).toInt(fontSize);

	alertFollow = o.value(QStringLiteral("alert_follow")).toBool(alertFollow);
	alertSub = o.value(QStringLiteral("alert_sub")).toBool(alertSub);
	alertGift = o.value(QStringLiteral("alert_gift")).toBool(alertGift);
	alertBits = o.value(QStringLiteral("alert_bits")).toBool(alertBits);
	alertRaid = o.value(QStringLiteral("alert_raid")).toBool(alertRaid);
	alertMembership = o.value(QStringLiteral("alert_membership")).toBool(alertMembership);
	alertSuperChat = o.value(QStringLiteral("alert_superchat")).toBool(alertSuperChat);
	alertSuperSticker = o.value(QStringLiteral("alert_supersticker")).toBool(alertSuperSticker);
}

bool ConfigManager::save() const
{
	QJsonObject o;
	o.insert(QStringLiteral("twitch_client_id"), twitchClientId);
	o.insert(QStringLiteral("twitch_channel"), twitchChannel);
	o.insert(QStringLiteral("youtube_client_id"), youtubeClientId);

	o.insert(QStringLiteral("show_badges"), showBadges);
	o.insert(QStringLiteral("show_timestamps"), showTimestamps);
	o.insert(QStringLiteral("font_size"), fontSize);

	o.insert(QStringLiteral("alert_follow"), alertFollow);
	o.insert(QStringLiteral("alert_sub"), alertSub);
	o.insert(QStringLiteral("alert_gift"), alertGift);
	o.insert(QStringLiteral("alert_bits"), alertBits);
	o.insert(QStringLiteral("alert_raid"), alertRaid);
	o.insert(QStringLiteral("alert_membership"), alertMembership);
	o.insert(QStringLiteral("alert_superchat"), alertSuperChat);
	o.insert(QStringLiteral("alert_supersticker"), alertSuperSticker);

	const QJsonDocument doc(o);

	QSaveFile f(configFilePath());
	if (!f.open(QIODevice::WriteOnly))
		return false;
	f.write(doc.toJson(QJsonDocument::Indented));
	return f.commit();
}
