#include "LockScreen.h"
#include "Icons.h"
#include <QDateTime>
#include <QGraphicsDropShadowEffect>
#include <QKeyEvent>
#include <QLabel>
#include <QLocale>
#include <QMouseEvent>
#include <QPushButton>
#include <QTimer>

LockScreen::LockScreen(QWidget* parent) : WallpaperWidget(parent) {
    setFocusPolicy(Qt::StrongFocus);
    m_clock = new QLabel(this); m_date = new QLabel(this);
    m_hint = new QLabel("Проведите вверх, чтобы войти", this);
    m_clock->setStyleSheet("color:white;font-size:104px;font-weight:300;background:transparent;");
    m_date->setStyleSheet("color:#ece6ff;font-size:24px;background:transparent;");
    m_hint->setStyleSheet("color:rgba(255,255,255,170);font-size:13px;background:transparent;");
    m_hint->setAlignment(Qt::AlignCenter);
    for (QLabel* l : {m_clock, m_date}) {
        auto* sh = new QGraphicsDropShadowEffect(l); sh->setBlurRadius(24); sh->setOffset(0, 2); sh->setColor(QColor(0, 0, 0, 160)); l->setGraphicsEffect(sh);
    }
    m_enter = new QPushButton("   Войти в систему   »", this);
    m_enter->setIcon(Icons::get("lock")); m_enter->setIconSize(QSize(18, 18));
    m_enter->setCursor(Qt::PointingHandCursor);
    m_enter->setFixedSize(270, 54);
    setAccent(QColor("#8b5cf6"));
    connect(m_enter, &QPushButton::clicked, this, &LockScreen::unlockRequested);
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &LockScreen::tick);
    m_timer->start(1000);
    tick();
}

void LockScreen::setAccent(const QColor& c) {
    m_enter->setStyleSheet(QString("QPushButton{background:rgba(%1,%2,%3,215);color:white;border:1px solid rgba(255,255,255,60);border-radius:27px;font-size:16px;font-weight:600;}"
                                   "QPushButton:hover{background:rgba(%1,%2,%3,255);border-color:rgba(255,255,255,140);}").arg(c.red()).arg(c.green()).arg(c.blue()));
}

void LockScreen::tick() {
    const QDateTime now = QDateTime::currentDateTime();
    m_clock->setText(now.toString("HH:mm"));
    QString d = QLocale(QLocale::Russian).toString(now.date(), "ddd, d MMMM");
    if (!d.isEmpty()) d[0] = d[0].toUpper();
    m_date->setText(d);
    m_clock->adjustSize(); m_date->adjustSize();
}

void LockScreen::resizeEvent(QResizeEvent* e) { WallpaperWidget::resizeEvent(e); relayout(); }

void LockScreen::relayout() {
    const int x = int(width() * 0.075), y = int(height() * 0.16) - m_offset;
    m_clock->move(x, y);
    m_date->move(x + 6, y + m_clock->height() - 4);
    m_enter->move((width() - m_enter->width()) / 2, int(height() * 0.80) - m_offset);
    m_hint->setGeometry(0, m_enter->y() + m_enter->height() + 12, width(), 20);
}

void LockScreen::resetPosition() { m_offset = 0; relayout(); }

void LockScreen::mousePressEvent(QMouseEvent* e) {
    if (e->position().y() < 40) { emit windowDragRequested(); return; }
    m_pressY = int(e->position().y());
}

void LockScreen::mouseMoveEvent(QMouseEvent* e) {
    if (m_pressY < 0) return;
    m_offset = qMax(0, m_pressY - int(e->position().y()));
    relayout();
    setDim(qMin(160, m_offset / 3));
}

void LockScreen::mouseReleaseEvent(QMouseEvent*) {
    const bool go = m_pressY >= 0 && m_offset > height() * 0.18;
    m_pressY = -1;
    if (go) emit unlockRequested(); else { m_offset = 0; setDim(0); relayout(); }
}

void LockScreen::keyPressEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter || e->key() == Qt::Key_Space || e->key() == Qt::Key_Up) emit unlockRequested();
}
