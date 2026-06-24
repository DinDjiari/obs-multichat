#include "token-store.hpp"
#include "config-manager.hpp"

// glib's gio headers (pulled in by libsecret) declare struct members named
// "signals"/"slots", which clash with Qt's keyword macros. Neutralise them
// for the duration of this include.
#if defined(signals)
#undef signals
#endif
#if defined(slots)
#undef slots
#endif
#include <libsecret/secret.h>

#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

const SecretSchema *multichat_schema()
{
	static const SecretSchema schema = {
		"com.obsproject.multichat.Secret",
		SECRET_SCHEMA_NONE,
		{
			{"key", SECRET_SCHEMA_ATTRIBUTE_STRING},
			{nullptr, SECRET_SCHEMA_ATTRIBUTE_STRING},
		},
		// reserved fields
		0, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	};
	return &schema;
}

// --- File fallback ---------------------------------------------------------
// Used when no Secret Service is available (e.g. no keyring daemon running),
// so logins survive an OBS restart. Stored 0600 in the config directory.

QString fallbackPath()
{
	return ConfigManager::configDir() + QStringLiteral("/tokens.json");
}

QJsonObject readFallback()
{
	QFile f(fallbackPath());
	if (!f.open(QIODevice::ReadOnly))
		return {};
	const QJsonObject o = QJsonDocument::fromJson(f.readAll()).object();
	f.close();
	return o;
}

void writeFallback(const QJsonObject &o)
{
	QDir().mkpath(ConfigManager::configDir());
	QSaveFile f(fallbackPath());
	if (!f.open(QIODevice::WriteOnly))
		return;
	f.write(QJsonDocument(o).toJson(QJsonDocument::Compact));
	f.commit();
	QFile::setPermissions(fallbackPath(),
			      QFile::ReadOwner | QFile::WriteOwner);
}

bool fileSet(const QString &key, const QString &value)
{
	QJsonObject o = readFallback();
	o.insert(key, value);
	writeFallback(o);
	return true;
}

QString fileGet(const QString &key)
{
	return readFallback().value(key).toString();
}

void fileClear(const QString &key)
{
	QJsonObject o = readFallback();
	if (o.contains(key)) {
		o.remove(key);
		writeFallback(o);
	}
}

} // namespace

namespace TokenStore {

bool set(const QString &key, const QString &value)
{
	GError *error = nullptr;
	const QByteArray k = key.toUtf8();
	const QByteArray v = value.toUtf8();

	gboolean ok = secret_password_store_sync(
		multichat_schema(), SECRET_COLLECTION_DEFAULT,
		"OBS Multichat", v.constData(), nullptr, &error,
		"key", k.constData(), nullptr);

	if (error) {
		g_error_free(error);
		ok = FALSE;
	}

	// Always keep a file copy as well: on systems without a working secret
	// service the keyring value would not survive a restart, which logged
	// the user out on every update. The file persists regardless.
	fileSet(key, value);
	return ok != FALSE;
}

QString get(const QString &key)
{
	GError *error = nullptr;
	const QByteArray k = key.toUtf8();

	gchar *password = secret_password_lookup_sync(
		multichat_schema(), nullptr, &error, "key", k.constData(),
		nullptr);

	if (error) {
		g_error_free(error);
		return fileGet(key);
	}
	if (!password)
		return fileGet(key);

	QString result = QString::fromUtf8(password);
	secret_password_free(password);
	return result;
}

bool clear(const QString &key)
{
	fileClear(key);

	GError *error = nullptr;
	const QByteArray k = key.toUtf8();

	gboolean ok = secret_password_clear_sync(
		multichat_schema(), nullptr, &error, "key", k.constData(),
		nullptr);

	if (error) {
		g_error_free(error);
		return true; // file copy already cleared
	}
	return ok != FALSE;
}

} // namespace TokenStore
