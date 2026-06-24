#include "chat-tab.hpp"
#include "html-util.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextBrowser>
#include <QTextDocument>
#include <QPushButton>
#include <QScrollBar>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QUrl>

namespace {

// Draws a round brand-coloured badge with a simple white glyph. These are
// original generic icons (play triangle / speech bubble) in the platform
// colours, not reproductions of the official trademarked logos.
QPixmap makeBadge(const QColor &bg, bool youtube)
{
	const int s = 36; // drawn at 2x, shown at 18px
	QPixmap pm(s, s);
	pm.fill(Qt::transparent);

	QPainter p(&pm);
	p.setRenderHint(QPainter::Antialiasing, true);
	p.setPen(Qt::NoPen);
	p.setBrush(bg);
	p.drawEllipse(0, 0, s, s);

	p.setBrush(Qt::white);
	if (youtube) {
		QPolygonF tri;
		tri << QPointF(s * 0.40, s * 0.31) << QPointF(s * 0.40, s * 0.69)
		    << QPointF(s * 0.71, s * 0.50);
		p.drawPolygon(tri);
	} else {
		QPainterPath path;
		const QRectF body(s * 0.24, s * 0.27, s * 0.52, s * 0.34);
		path.addRoundedRect(body, s * 0.11, s * 0.11);
		QPolygonF tail;
		tail << QPointF(s * 0.36, s * 0.58) << QPointF(s * 0.36, s * 0.74)
		     << QPointF(s * 0.50, s * 0.58);
		path.addPolygon(tail);
		p.drawPath(path.simplified());
	}
	p.end();

	pm.setDevicePixelRatio(2.0);
	return pm;
}

} // namespace

ChatTab::ChatTab(QWidget *parent) : QWidget(parent)
{
	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(4);

	m_view = new QTextBrowser(this);
	m_view->setOpenExternalLinks(true);
	m_view->setStyleSheet(QStringLiteral(
		"QTextBrowser { background-color: #1e1e1e; color: #e0e0e0;"
		" border: none; padding: 4px; }"));
	layout->addWidget(m_view, 1);

	auto *bottom = new QHBoxLayout();
	auto *settingsBtn = new QPushButton(tr("\u2699 Settings"), this);
	connect(settingsBtn, &QPushButton::clicked, this,
		&ChatTab::settingsRequested);
	bottom->addWidget(settingsBtn);
	bottom->addStretch(1);
	auto *clearBtn = new QPushButton(tr("Clear"), this);
	connect(clearBtn, &QPushButton::clicked, this, &ChatTab::clearChat);
	bottom->addWidget(clearBtn);
	layout->addLayout(bottom);

	m_twIcon = makeBadge(QColor("#9146FF"), false);
	m_ytIcon = makeBadge(QColor("#FF0000"), true);
	registerIcons();

	setFontSize(m_fontSize);
}

void ChatTab::registerIcons()
{
	// QTextDocument::clear() drops resources, so (re)register them here.
	m_view->document()->addResource(QTextDocument::ImageResource,
					QUrl(QStringLiteral("mc://tw")),
					QVariant(m_twIcon));
	m_view->document()->addResource(QTextDocument::ImageResource,
					QUrl(QStringLiteral("mc://yt")),
					QVariant(m_ytIcon));
}

void ChatTab::setFontSize(int px)
{
	m_fontSize = px;
	QFont f = m_view->font();
	f.setPointSize(px);
	m_view->setFont(f);
}

void ChatTab::clearChat()
{
	m_view->clear();
	registerIcons();
}

void ChatTab::addMessage(const ChatMessage &m)
{
	const bool twitch = m.platform == Platform::Twitch;

	QString badge;
	if (m_showBadges) {
		const int sz = m_fontSize + 4;
		badge = QStringLiteral(
				"<img src='%1' width='%2' height='%2' "
				"style='vertical-align:middle;'>&nbsp;")
				.arg(twitch ? QStringLiteral("mc://tw")
					    : QStringLiteral("mc://yt"))
				.arg(sz);
	}

	QString ts;
	if (m_showTimestamps) {
		ts = QStringLiteral(
			     "<span style='color:#808080;'>%1</span> ")
			     .arg(m.timestamp.toString(QStringLiteral("HH:mm")));
	}

	QColor nameColor = m.authorColor;
	if (!nameColor.isValid())
		nameColor = twitch ? QColor("#bf94ff") : QColor("#ff7b7b");

	QString modBadge;
	if (m.isModerator)
		modBadge = QStringLiteral(
			"<span style='color:#00c853;'>&#9733;</span> ");

	const QString line = QStringLiteral(
		"<div style='margin:2px 0;'>%1%2%3"
		"<span style='color:%4;font-weight:bold;'>%5</span>"
		"<span style='color:#9aa0a6;'>: </span>"
		"<span style='color:#e0e0e0;'>%6</span></div>")
		.arg(ts)
		.arg(badge)
		.arg(modBadge)
		.arg(nameColor.name())
		.arg(mc::escapeHtml(m.author))
		.arg(mc::escapeHtml(m.message));

	const bool atBottom =
		m_view->verticalScrollBar()->value() >=
		m_view->verticalScrollBar()->maximum() - 4;

	m_view->append(line);

	if (atBottom)
		m_view->verticalScrollBar()->setValue(
			m_view->verticalScrollBar()->maximum());
}
