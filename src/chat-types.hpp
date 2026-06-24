#pragma once

#include <QString>
#include <QColor>
#include <QDateTime>

// Origin platform of a message or alert.
enum class Platform {
	Twitch,
	YouTube,
};

// A single chat line shown in the combined chat list.
struct ChatMessage {
	Platform platform = Platform::Twitch;
	QString author;
	QString message;
	QColor authorColor;
	QDateTime timestamp = QDateTime::currentDateTime();
	bool isModerator = false;
	bool isMember = false; // Twitch subscriber / YouTube member
};

// Categories of alert events from both platforms.
enum class AlertType {
	TwitchFollow,
	TwitchSub,
	TwitchGiftSub,
	TwitchBits,
	TwitchRaid,
	YouTubeMembership,
	YouTubeSuperChat,
	YouTubeSuperSticker,
};

// A single alert event.
struct Alert {
	AlertType type = AlertType::TwitchFollow;
	Platform platform = Platform::Twitch;
	QString user;
	QString message; // optional user comment
	QString detail;  // tier name, plan, sticker name, etc.
	int amount = 0;  // bits / months / gift count / raid viewers
	double money = 0.0; // super chat / super sticker amount
	QString currency;
	QDateTime timestamp = QDateTime::currentDateTime();
};
