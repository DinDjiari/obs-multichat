#pragma once

#include <QString>

namespace mc {

// Escape a string for safe insertion into the QTextBrowser rich-text views.
inline QString escapeHtml(const QString &in)
{
	QString out;
	out.reserve(in.size());
	for (const QChar c : in) {
		switch (c.unicode()) {
		case '&':
			out += QStringLiteral("&amp;");
			break;
		case '<':
			out += QStringLiteral("&lt;");
			break;
		case '>':
			out += QStringLiteral("&gt;");
			break;
		case '"':
			out += QStringLiteral("&quot;");
			break;
		case '\'':
			out += QStringLiteral("&#39;");
			break;
		default:
			out += c;
		}
	}
	return out;
}

} // namespace mc
