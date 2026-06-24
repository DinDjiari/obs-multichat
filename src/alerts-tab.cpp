#include "alerts-tab.hpp"
#include "html-util.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QPushButton>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QEasingCurve>

namespace {

struct AlertStyle {
	QString icon;
	QString color;
	QString title;
};

AlertStyle styleFor(const Alert &a)
{
	switch (a.type) {
	case AlertType::TwitchFollow:
		return {QStringLiteral("&#10084;"), QStringLiteral("#9146FF"),
			QStringLiteral("Follow")};
	case AlertType::TwitchSub:
		return {QStringLiteral("&#11088;"), QStringLiteral("#9146FF"),
			QStringLiteral("Subscription")};
	case AlertType::TwitchGiftSub:
		return {QStringLiteral("&#127873;"), QStringLiteral("#9146FF"),
			QStringLiteral("Gift Sub")};
	case AlertType::TwitchBits:
		return {QStringLiteral("&#128176;"), QStringLiteral("#9146FF"),
			QStringLiteral("Bits")};
	case AlertType::TwitchRaid:
		return {QStringLiteral("&#9876;"), QStringLiteral("#9146FF"),
			QStringLiteral("Raid")};
	case AlertType::YouTubeMembership:
		return {QStringLiteral("&#11088;"), QStringLiteral("#FF0000"),
			QStringLiteral("Membership")};
	case AlertType::YouTubeSuperChat:
		return {QStringLiteral("&#128176;"), QStringLiteral("#FF0000"),
			QStringLiteral("Super Chat")};
	case AlertType::YouTubeSuperSticker:
		return {QStringLiteral("&#127991;"), QStringLiteral("#FF0000"),
			QStringLiteral("Super Sticker")};
	}
	return {QStringLiteral("&#10024;"), QStringLiteral("#888888"),
		QStringLiteral("Alert")};
}

QString describe(const Alert &a)
{
	switch (a.type) {
	case AlertType::TwitchFollow:
		return QStringLiteral("started following");
	case AlertType::TwitchSub:
		return QStringLiteral("subscribed (%1%2)")
			.arg(a.detail)
			.arg(a.amount > 0
				     ? QStringLiteral(", %1 months")
					       .arg(a.amount)
				     : QString());
	case AlertType::TwitchGiftSub:
		return a.amount > 1
			       ? QStringLiteral("gifted %1 %2 subs")
					 .arg(a.amount)
					 .arg(a.detail)
			       : QStringLiteral("gifted a %1 sub to %2")
					 .arg(a.detail)
					 .arg(mc::escapeHtml(a.message));
	case AlertType::TwitchBits:
		return QStringLiteral("cheered %1 bits").arg(a.amount);
	case AlertType::TwitchRaid:
		return QStringLiteral("raided with %1 viewers").arg(a.amount);
	case AlertType::YouTubeMembership:
		return a.amount > 0
			       ? QStringLiteral("member for %1 months (%2)")
					 .arg(a.amount)
					 .arg(a.detail)
			       : QStringLiteral("became a member (%1)")
					 .arg(a.detail);
	case AlertType::YouTubeSuperChat:
		return QStringLiteral("sent a Super Chat %1").arg(a.detail);
	case AlertType::YouTubeSuperSticker:
		return QStringLiteral("sent a Super Sticker %1").arg(a.detail);
	}
	return QString();
}

} // namespace

AlertCard::AlertCard(const Alert &a, QWidget *parent) : QFrame(parent)
{
	const AlertStyle s = styleFor(a);

	setObjectName(QStringLiteral("AlertCard"));
	setStyleSheet(QStringLiteral(
			      "QFrame#AlertCard { background:#262626;"
			      " border-left:3px solid %1; border-radius:3px; }")
			      .arg(s.color));

	auto *row = new QHBoxLayout(this);
	row->setContentsMargins(8, 6, 6, 6);
	row->setSpacing(8);

	QString comment;
	if (!a.message.isEmpty() &&
	    (a.type == AlertType::YouTubeSuperChat ||
	     a.type == AlertType::TwitchBits ||
	     a.type == AlertType::YouTubeMembership)) {
		comment = QStringLiteral(
				  " <span style='color:#9aa0a6;'>&mdash; %1</span>")
				  .arg(mc::escapeHtml(a.message));
	}

	auto *text = new QLabel(this);
	text->setTextFormat(Qt::RichText);
	text->setWordWrap(true);
	text->setText(QStringLiteral(
			      "<span style='color:%1;'>%2</span> "
			      "<span style='color:#9aa0a6;font-size:11px;'>"
			      "[%3]</span> "
			      "<span style='color:#ffffff;font-weight:bold;'>"
			      "%4</span> "
			      "<span style='color:#cfcfcf;'>%5</span>%6")
			      .arg(s.color)
			      .arg(s.icon)
			      .arg(s.title)
			      .arg(mc::escapeHtml(a.user))
			      .arg(describe(a))
			      .arg(comment));
	row->addWidget(text, 1);

	auto *done = new QPushButton(QStringLiteral("\u2713"), this);
	done->setToolTip(tr("Dismiss"));
	done->setFixedSize(26, 26);
	done->setCursor(Qt::PointingHandCursor);
	done->setStyleSheet(QStringLiteral(
		"QPushButton { background:#333; color:#cfcfcf;"
		" border:1px solid #4a4a4a; border-radius:13px;"
		" font-weight:bold; }"
		"QPushButton:hover { background:#1f7a33; color:#ffffff;"
		" border-color:#1f7a33; }"));
	connect(done, &QPushButton::clicked, this, &AlertCard::dismiss);
	row->addWidget(done, 0, Qt::AlignTop);
}

void AlertCard::dismiss()
{
	if (m_dismissing)
		return;
	m_dismissing = true;
	setEnabled(false);

	auto *eff = new QGraphicsOpacityEffect(this);
	setGraphicsEffect(eff);

	auto *group = new QParallelAnimationGroup(this);

	auto *fade = new QPropertyAnimation(eff, "opacity", group);
	fade->setDuration(220);
	fade->setStartValue(1.0);
	fade->setEndValue(0.0);

	auto *collapse = new QPropertyAnimation(this, "maximumHeight", group);
	collapse->setDuration(220);
	collapse->setStartValue(height());
	collapse->setEndValue(0);
	collapse->setEasingCurve(QEasingCurve::InCubic);

	group->addAnimation(fade);
	group->addAnimation(collapse);
	connect(group, &QParallelAnimationGroup::finished, this,
		[this]() { deleteLater(); });
	group->start(QAbstractAnimation::DeleteWhenStopped);
}

AlertsTab::AlertsTab(QWidget *parent) : QWidget(parent)
{
	auto *outer = new QVBoxLayout(this);
	outer->setContentsMargins(0, 0, 0, 0);

	m_scroll = new QScrollArea(this);
	m_scroll->setWidgetResizable(true);
	m_scroll->setFrameShape(QFrame::NoFrame);
	outer->addWidget(m_scroll);

	auto *container = new QWidget(m_scroll);
	m_scroll->setWidget(container);

	m_listLayout = new QVBoxLayout(container);
	m_listLayout->setContentsMargins(6, 6, 6, 6);
	m_listLayout->setSpacing(5);
	m_listLayout->addStretch(1); // keeps cards stacked at the top
}

void AlertsTab::clearAlerts()
{
	// Remove every card immediately (keeps the trailing stretch).
	while (m_listLayout->count() > 1) {
		QLayoutItem *item = m_listLayout->takeAt(0);
		if (item->widget())
			item->widget()->deleteLater();
		delete item;
	}
}

void AlertsTab::addAlert(const Alert &a)
{
	auto *card = new AlertCard(a, m_scroll->widget());
	m_listLayout->insertWidget(0, card); // newest on top
}
