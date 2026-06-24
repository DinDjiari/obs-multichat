#include <obs-module.h>
#include <obs-frontend-api.h>

#include <QMainWindow>
#include <QAction>

#include "multichat-core.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("multichat", "en-US")

static const char *kChatDock = "multichat_chat_dock";
static const char *kAlertsDock = "multichat_alerts_dock";

static MultichatCore *g_core = nullptr;

bool obs_module_load(void)
{
	QMainWindow *main =
		static_cast<QMainWindow *>(obs_frontend_get_main_window());
	if (!main) {
		blog(LOG_ERROR, "[multichat] no main window; not loaded");
		return false;
	}

	g_core = new MultichatCore(main);

	obs_frontend_add_dock_by_id(kChatDock, "Multichat",
				    g_core->createChatWidget());
	obs_frontend_add_dock_by_id(kAlertsDock, "Multichat Alerts",
				    g_core->createAlertsWidget());

	// Settings live in their own window, opened from the Tools menu.
	// (OBS exposes no public API to add a page to its built-in Settings
	// dialog, so a dedicated window is the supported pattern.)
	QAction *act = static_cast<QAction *>(
		obs_frontend_add_tools_menu_qaction("Multichat Settings"));
	if (act)
		QObject::connect(act, &QAction::triggered, []() {
			if (g_core)
				g_core->openSettings();
		});

	QAction *setupAct = static_cast<QAction *>(
		obs_frontend_add_tools_menu_qaction("Multichat Setup"));
	if (setupAct)
		QObject::connect(setupAct, &QAction::triggered, []() {
			if (g_core)
				g_core->openSetupWizard();
		});

	blog(LOG_INFO, "[multichat] plugin loaded");
	return true;
}

void obs_module_unload(void)
{
	obs_frontend_remove_dock(kChatDock);
	obs_frontend_remove_dock(kAlertsDock);
	delete g_core;
	g_core = nullptr;
	blog(LOG_INFO, "[multichat] plugin unloaded");
}

const char *obs_module_name(void)
{
	return "Multichat";
}

const char *obs_module_description(void)
{
	return "Combined Twitch and YouTube live chat and alerts docks.";
}
