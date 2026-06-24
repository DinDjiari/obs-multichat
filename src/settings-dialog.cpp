#include "settings-dialog.hpp"
#include "settings-tab.hpp"
#include "theme.hpp"

#include <QVBoxLayout>

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
	setWindowTitle(tr("Multichat Settings"));
	setStyleSheet(mc::darkStyle());
	resize(420, 620);

	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(8, 8, 8, 8);

	m_tab = new SettingsTab(this);
	layout->addWidget(m_tab);
}
