#pragma once

#include <QString>

// Non-secret configuration persisted to ~/.config/obs-multichat/config.json.
// OAuth tokens and client secrets are NOT stored here; they live in the
// system secret service (libsecret) via TokenStore.
class ConfigManager {
public:
	ConfigManager();

	void load();
	bool save() const;

	// Returns the directory used for configuration (created on demand).
	static QString configDir();
	static QString configFilePath();

	// Connection identity (public, non-secret values).
	QString twitchClientId;
	QString twitchChannel;     // broadcaster login name
	QString youtubeClientId;
	// NOTE: youtube client secret + all tokens are kept in TokenStore.

	// Display options.
	bool showBadges = true;
	bool showTimestamps = true;
	int fontSize = 11;

	// Alert toggles.
	bool alertFollow = true;
	bool alertSub = true;
	bool alertGift = true;
	bool alertBits = true;
	bool alertRaid = true;
	bool alertMembership = true;
	bool alertSuperChat = true;
	bool alertSuperSticker = true;
};
