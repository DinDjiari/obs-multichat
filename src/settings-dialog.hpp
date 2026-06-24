#pragma once

#include <QDialog>

class SettingsTab;

// Standalone settings window opened from the OBS Tools menu. OBS does not
// expose a public API to add a page to its built-in Settings dialog, so a
// dedicated window is the supported pattern for plugin configuration.
class SettingsDialog : public QDialog {
	Q_OBJECT
public:
	explicit SettingsDialog(QWidget *parent = nullptr);

	SettingsTab *tab() const { return m_tab; }

private:
	SettingsTab *m_tab = nullptr;
};
