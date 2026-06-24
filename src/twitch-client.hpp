#pragma once

#include <QObject>
#include <QString>
#include "chat-types.hpp"

class QWebSocket;
class QNetworkAccessManager;

// Twitch integration:
//  - Chat, cheers (bits) and sub/gift/raid events come over the Twitch IRC
//    WebSocket gateway (PRIVMSG + USERNOTICE with IRCv3 tags).
//  - Follows are delivered over EventSub (channel.follow v2), since they are
//    not available on IRC.
class TwitchClient : public QObject {
	Q_OBJECT
public:
	explicit TwitchClient(QObject *parent = nullptr);
	~TwitchClient() override;

	void setCredentials(const QString &clientId, const QString &accessToken,
			    const QString &channel);
	void connectChat();
	void disconnectAll();
	bool isConnected() const { return m_chatConnected; }

signals:
	void chatMessage(const ChatMessage &msg);
	void alert(const Alert &a);
	void statusChanged(const QString &status, bool connected);
	void authError();

private:
	// IRC
	void onIrcConnected();
	void onIrcText(const QString &frame);
	void onIrcClosed();
	void handleIrcLine(const QString &line);
	void sendIrc(const QString &line);

	// EventSub
	void connectEventSub();
	void fetchBroadcasterId();
	void onEventSubText(const QString &frame);
	void createFollowSubscription(const QString &sessionId);

	QString m_clientId;
	QString m_token;
	QString m_channel;
	QString m_broadcasterId;

	QWebSocket *m_irc = nullptr;
	QWebSocket *m_eventsub = nullptr;
	QNetworkAccessManager *m_net = nullptr;

	bool m_chatConnected = false;
	bool m_wantConnected = false;
	QString m_pendingBuffer;
};
