#pragma once

#include <QObject>
#include <QString>
#include "chat-types.hpp"

class QNetworkAccessManager;
class QTimer;

// YouTube Live Chat integration via the YouTube Data API v3.
//  - Finds the currently active broadcast for the authorized account.
//  - Polls liveChatMessages and classifies each item into chat messages or
//    alerts (Super Chat, Super Sticker, memberships).
class YouTubeClient : public QObject {
	Q_OBJECT
public:
	explicit YouTubeClient(QObject *parent = nullptr);

	void setCredentials(const QString &accessToken);
	void connectChat();
	void disconnectAll();
	bool isConnected() const { return !m_liveChatId.isEmpty(); }

signals:
	void chatMessage(const ChatMessage &msg);
	void alert(const Alert &a);
	void statusChanged(const QString &status, bool connected);
	void authError();

private:
	void findActiveBroadcast();
	void pollMessages();
	void handleItems(const class QJsonArray &items);

	QNetworkAccessManager *m_net = nullptr;
	QTimer *m_findTimer = nullptr;
	QTimer *m_pollTimer = nullptr;

	QString m_token;
	QString m_liveChatId;
	QString m_pageToken;
	bool m_wantConnected = false;
	bool m_primingPoll = true;
	int m_pollFailures = 0;
};
