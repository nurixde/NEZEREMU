#include "DesktopScreen.h"
#include "Dock.h"
#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

DesktopScreen::DesktopScreen(QWidget* parent) : WallpaperWidget(parent) {
    m_dock = new Dock(this);
    m_shortcuts = new QWidget(this); m_shortcuts->setAttribute(Qt::WA_TranslucentBackground);
    m_scLay = new QHBoxLayout(m_shortcuts); m_scLay->setContentsMargins(0, 0, 0, 0); m_scLay->setSpacing(18);
    m_clock = new QLabel(this); m_clock->setStyleSheet("color:white;font-size:13px;font-weight:600;background:transparent;");
    auto* t = new QTimer(this);
    connect(t, &QTimer::timeout, this, [this] { m_clock->setText(QTime::currentTime().toString("HH:mm")); m_clock->adjustSize(); relayout(); });
    t->start(1000 * 10); m_clock->setText(QTime::currentTime().toString("HH:mm"));

    m_status = new QFrame(this); m_status->setObjectName("st");
    m_status->setStyleSheet("QFrame#st{background:rgba(16,10,32,215);border:1px solid rgba(255,255,255,30);border-radius:16px;}");
    auto* v = new QVBoxLayout(m_status); v->setContentsMargins(22, 18, 22, 18); v->setSpacing(12);
    m_statusText = new QLabel; m_statusText->setWordWrap(true); m_statusText->setAlignment(Qt::AlignCenter);
    m_statusText->setStyleSheet("font-size:14px;background:transparent;");
    m_busy = new QProgressBar; m_busy->setRange(0, 0); m_busy->setTextVisible(false); m_busy->setFixedHeight(6);
    auto* h = new QHBoxLayout; m_primary = new QPushButton; m_primary->setObjectName("primary"); m_log = new QPushButton("Журнал");
    h->addStretch(); h->addWidget(m_primary); h->addWidget(m_log); h->addStretch();
    v->addWidget(m_statusText); v->addWidget(m_busy); v->addLayout(h);
    connect(m_primary, &QPushButton::clicked, this, [this] { if (m_state == AndroidEngine::State::NotInstalled) emit setupRequested(); else emit startRequested(); });
    connect(m_log, &QPushButton::clicked, this, &DesktopScreen::logRequested);
    setAndroidState(AndroidEngine::State::Stopped, "Android остановлен");
}

void DesktopScreen::setShortcuts(const QList<AppEntry>& apps) {
    while (auto* it = m_scLay->takeAt(0)) { delete it->widget(); delete it; }
    for (const AppEntry& a : apps) {
        auto* w = new QWidget; w->setAttribute(Qt::WA_TranslucentBackground);
        auto* v = new QVBoxLayout(w); v->setContentsMargins(0, 0, 0, 0); v->setSpacing(4);
        auto* b = new QToolButton; b->setIcon(a.icon); b->setIconSize(QSize(50, 50)); b->setFixedSize(58, 58); b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet("QToolButton{border:none;border-radius:14px;background:transparent;}QToolButton:hover{background:rgba(255,255,255,25);}");
        auto* l = new QLabel(a.label); l->setAlignment(Qt::AlignCenter); l->setStyleSheet("font-size:11px;color:white;background:transparent;");
        v->addWidget(b, 0, Qt::AlignHCenter); v->addWidget(l, 0, Qt::AlignHCenter);
        connect(b, &QToolButton::clicked, this, [this, p = a.package] { emit launchRequested(p); });
        m_scLay->addWidget(w);
    }
    m_shortcuts->adjustSize();
    relayout();
}

void DesktopScreen::setAndroidState(AndroidEngine::State st, const QString& msg) {
    using S = AndroidEngine::State;
    m_state = st;
    m_status->setVisible(st != S::Ready);
    m_statusText->setText(msg);
    m_busy->setVisible(st == S::Starting || st == S::Booting);
    m_primary->setVisible(st == S::NotInstalled || st == S::Stopped || st == S::Error);
    m_primary->setText(st == S::NotInstalled ? "Установить Android" : st == S::Error ? "Повторить" : "Запустить Android");
    m_log->setVisible(st == S::Error || st == S::Booting || st == S::Starting);
    relayout();
}

void DesktopScreen::relayout() {
    const int W = width(), H = height();
    m_shortcuts->move(int(W * 0.035), int(H * 0.06));
    m_clock->move(W - m_clock->width() - 18, 12);
    const QSize ds = m_dock->sizeHint();
    const int dw = qMin(ds.width(), W - 40);
    m_dock->setGeometry((W - dw) / 2, H - ds.height() - int(H * 0.035), dw, ds.height());
    const int sw = qMin(460, W - 60);
    m_status->setFixedWidth(sw); m_status->adjustSize();
    m_status->move((W - sw) / 2, (m_dock->y() - m_status->height()) / 2);
}

void DesktopScreen::resizeEvent(QResizeEvent* e) { WallpaperWidget::resizeEvent(e); relayout(); }
