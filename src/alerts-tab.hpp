#pragma once

#include <QFrame>
#include <QWidget>
#include "chat-types.hpp"

class QVBoxLayout;
class QScrollArea;

// A single alert entry. Carries a checkmark button that dismisses the card
// with a fade + collapse animation.
class AlertCard : public QFrame {
	Q_OBJECT
public:
	explicit AlertCard(const Alert &a, QWidget *parent = nullptr);

private slots:
	void dismiss();

private:
	bool m_dismissing = false;
};

// Scrolling list of alert cards from both platforms (newest on top).
class AlertsTab : public QWidget {
	Q_OBJECT
public:
	explicit AlertsTab(QWidget *parent = nullptr);

	void addAlert(const Alert &a);
	void clearAlerts();

private:
	QScrollArea *m_scroll = nullptr;
	QVBoxLayout *m_listLayout = nullptr;
};
