#include "TitleBar.h"
#include "Icons.h"
#include "core/Paths.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QToolButton>
#include <QWindow>

TitleBar::TitleBar(QWidget* parent) : QWidget(parent) {
    setFixedHeight(kHeight);
    auto* h = new QHBoxLayout(this); h->setContentsMargins(12, 0, 4, 0); h->setSpacing(8);
    auto* logo = new QLabel; logo->setPixmap(QPixmap(Paths::assetsDir() + "/logo.png").scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    auto* name = new QLabel("NEZER EMU"); name->setStyleSheet("font-size:14px;font-weight:700;letter-spacing:1px;background:transparent;");
    m_status = new QLabel; m_status->setStyleSheet("font-size:11px;color:#b9b2d6;background:transparent;");
    h->addWidget(logo); h->addWidget(name); h->addSpacing(14); h->addWidget(m_status); h->addStretch();
    const char* ids[] = {"min", "max", "close"};
    for (int i = 0; i < 3; ++i) {
        auto* b = new QToolButton; b->setIcon(Icons::get(ids[i], QColor("#e8e2ff"))); b->setIconSize(QSize(16, 16)); b->setFixedSize(42, 30);
        b->setStyleSheet(QString("QToolButton{border:none;border-radius:6px;background:transparent;}QToolButton:hover{background:%1;}").arg(i == 2 ? "#e5484d" : "rgba(255,255,255,30)"));
        h->addWidget(b);
        if (i == 0) connect(b, &QToolButton::clicked, this, &TitleBar::minimizeRequested);
        if (i == 1) connect(b, &QToolButton::clicked, this, &TitleBar::maximizeRequested);
        if (i == 2) connect(b, &QToolButton::clicked, this, &TitleBar::closeRequested);
    }
}

void TitleBar::setStatus(const QString& t, const QColor& dot) { m_status->setText("●  " + t); m_dot = dot; m_status->setStyleSheet(QString("font-size:11px;color:%1;background:transparent;").arg(dot.name())); }

void TitleBar::paintEvent(QPaintEvent*) { QPainter p(this); p.fillRect(rect(), QColor(13, 8, 26, 235)); p.setPen(QColor(255, 255, 255, 18)); p.drawLine(0, height() - 1, width(), height() - 1); }
void TitleBar::mousePressEvent(QMouseEvent* e) { if (e->button() == Qt::LeftButton && window()->windowHandle()) window()->windowHandle()->startSystemMove(); }
void TitleBar::mouseDoubleClickEvent(QMouseEvent*) { emit maximizeRequested(); }
