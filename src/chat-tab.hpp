#pragma once

#include <QWidget>
#include <QPixmap>
#include "chat-types.hpp"

class QTextBrowser;

// Combined chat view for both platforms. Each message is prefixed with a
// small platform icon (brand-coloured badge) instead of a text label.
class ChatTab : public QWidget {
	Q_OBJECT
public:
	explicit ChatTab(QWidget *parent = nullptr);

	void addMessage(const ChatMessage &m);
	void clearChat();

	void setShowBadges(bool on) { m_showBadges = on; }
	void setShowTimestamps(bool on) { m_showTimestamps = on; }
	void setFontSize(int px);

signals:
	void settingsRequested();

private:
	void registerIcons();

	QTextBrowser *m_view = nullptr;
	QPixmap m_twIcon;
	QPixmap m_ytIcon;
	bool m_showBadges = true;
	bool m_showTimestamps = true;
	int m_fontSize = 11;
};
