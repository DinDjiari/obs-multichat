#include <obs-module.h>
#include <obs-frontend-api.h>

#include <QMainWindow>
#include <QPointer>

#include "multichat-dock.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("multichat", "en-US")

static const char *kDockId = "multichat_dock";

// Kept so the dock can be cleaned up explicitly on unload.
static QPointer<MultichatDock> g_dock;

bool obs_module_load(void)
{
	QMainWindow *main =
		static_cast<QMainWindow *>(obs_frontend_get_main_window());
	if (!main) {
		blog(LOG_ERROR,
		     "[multichat] no main window available; dock not created");
		return false;
	}

	// Parent the dock to the OBS main window so Qt owns its lifetime.
	g_dock = new MultichatDock(main);
	g_dock->setObjectName("MultichatDock");

	if (!obs_frontend_add_dock_by_id(kDockId, "Multichat", g_dock)) {
		blog(LOG_ERROR, "[multichat] failed to register dock");
		delete g_dock;
		g_dock = nullptr;
		return false;
	}

	blog(LOG_INFO, "[multichat] plugin loaded");
	return true;
}

void obs_module_unload(void)
{
	obs_frontend_remove_dock(kDockId);
	// The widget is owned/destroyed by the frontend dock machinery; clear
	// our reference. QPointer becomes null automatically once deleted.
	g_dock = nullptr;
	blog(LOG_INFO, "[multichat] plugin unloaded");
}

const char *obs_module_name(void)
{
	return "Multichat";
}

const char *obs_module_description(void)
{
	return "Combined Twitch and YouTube live chat and alerts dock.";
}
