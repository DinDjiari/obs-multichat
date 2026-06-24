#pragma once

#include <QString>

// Stores secrets (OAuth tokens, the YouTube client secret) in the system
// secret service through libsecret (GNOME Keyring / KWallet via the
// freedesktop Secret Service API). Values never touch the plain config file.
namespace TokenStore {

// Store / overwrite a secret. Returns true on success.
bool set(const QString &key, const QString &value);

// Look up a secret. Returns an empty string if not present.
QString get(const QString &key);

// Remove a secret. Returns true on success (or if it did not exist).
bool clear(const QString &key);

} // namespace TokenStore

// Well-known keys used across the plugin.
namespace TokenKeys {
inline const char *TwitchAccess = "twitch_access_token";
inline const char *TwitchRefresh = "twitch_refresh_token";
inline const char *TwitchClientSecret = "twitch_client_secret";
inline const char *YouTubeAccess = "youtube_access_token";
inline const char *YouTubeRefresh = "youtube_refresh_token";
inline const char *YouTubeClientSecret = "youtube_client_secret";
} // namespace TokenKeys
