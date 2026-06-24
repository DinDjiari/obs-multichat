#pragma once

#include <QString>

namespace mc {

// Dark stylesheet applied to Multichat top-level widgets so they blend with
// the OBS dark theme regardless of the active OBS palette.
inline QString darkStyle()
{
	return QStringLiteral(
		"QWidget { background-color:#232323; color:#d0d0d0; }"
		"QGroupBox { border:1px solid #3a3a3a; border-radius:4px;"
		" margin-top:8px; padding-top:8px; }"
		"QGroupBox::title { subcontrol-origin: margin; left:8px;"
		" padding:0 3px; }"
		"QLabel { color:#d0d0d0; }"
		"QLineEdit, QSpinBox { background:#1e1e1e; color:#e0e0e0;"
		" border:1px solid #3a3a3a; border-radius:3px; padding:3px; }"
		"QCheckBox { color:#d0d0d0; }"
		"QScrollArea { border:none; }"
		"QPushButton { background:#3b3b3b; color:#ffffff;"
		" border:1px solid #4a4a4a; border-radius:3px;"
		" padding:5px 10px; }"
		"QPushButton:hover { background:#474747; }");
}

} // namespace mc
