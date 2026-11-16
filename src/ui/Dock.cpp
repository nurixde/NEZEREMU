#include "Dock.h"
#include "Icons.h"
#include <QContextMenuEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QToolButton>
#include <QVBoxLayout>

Dock::Dock(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground);
    m_lay = new QHBoxLayout(this); m_lay->setContentsMargins(20, 10, 20, 14); m_lay->setSpacing(14);
    setApps({}, false);
}

QWidget* Dock::makeTile(const QIcon& icon, const QString& label, bool dashed) {
    auto* w = new QWidget(this); w->setAttribute(Qt::WA_TranslucentBackground);
    auto* v = new QVBoxLayout(w); v->setContentsMargins(0, 0, 0, 0); v->setSpacing(4);
    auto* b = new QToolButton(w); b->setIcon(icon); b->setIconSize(dashed ? QSize(24, 24) : QSize(52, 52)); b->setFixedSize(58, 58);
    b->setCursor(Qt::PointingHandCursor);
    b->setStyleSheet(dashed ? "QToolButton{border:1.5px dashed rgba(255,255,255,90);border-radius:14px;background:rgba(255,255,255,10);}QToolButton:hover{background:rgba(255,255,255,30);}"
                            : "QToolButton{border:none;border-radius:14px;background:transparent;}QToolButton:hover{background:rgba(255,255,255,25);}");
    auto* l = new QLabel(label, w); l->setAlignment(Qt::AlignCenter); l->setFixedWidth(76);
    l->setStyleSheet("font-size:11px;color:#f3efff;background:transparent;");
    l->setText(l->fontMetrics().elidedText(label, Qt::ElideRight, 74));
    v->addWidget(b, 0, Qt::AlignHCenter); v->addWidget(l, 0, Qt::AlignHCenter);
    w->setProperty("btn", QVariant::fromValue<QObject*>(b));
    return w;
}

void Dock::setApps(const QList<AppEntry>& apps, bool ready) {
    qDeleteAll(m_tiles); m_tiles.clear();
    auto* add = makeTile(Icons::get("plus"), "Добавить", true);
    connect(qobject_cast<QToolButton*>(add->property("btn").value<QObject*>()), &QToolButton::clicked, this, &Dock::addRequested);
    m_tiles << add;
    if (!ready) {
        auto* note = new QLabel("Приложения появятся после запуска Android", this);
        note->setStyleSheet("color:#cfc6f2;font-size:12px;background:transparent;");
        m_tiles << note;
    }
    for (const AppEntry& a : apps) {
        auto* t = makeTile(a.icon, a.label, false);
        auto* b = qobject_cast<QToolButton*>(t->property("btn").value<QObject*>());
        b->setToolTip(a.label + "\n" + a.package);
        connect(b, &QToolButton::clicked, this, [this, p = a.package] { emit launchRequested(p); });
        b->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(b, &QWidget::customContextMenuRequested, this, [this, b, p = a.package](const QPoint& pos) {
            QMenu m; m.addAction("Запустить", this, [this, p] { emit launchRequested(p); });
            m.addAction("Убрать из Dock", this, [this, p] { emit removeRequested(p); });
            m.exec(b->mapToGlobal(pos));
        });
        m_tiles << t;
    }
    auto* all = makeTile(Icons::get("grid"), "Все приложения", false);
    connect(qobject_cast<QToolButton*>(all->property("btn").value<QObject*>()), &QToolButton::clicked, this, &Dock::allAppsRequested);
    m_tiles << all;
    for (QWidget* w : m_tiles) m_lay->addWidget(w);
    adjustSize();
    if (parentWidget()) QMetaObject::invokeMethod(parentWidget(), "relayout", Qt::QueuedConnection);
}

QSize Dock::sizeHint() const { return m_lay->sizeHint(); }

void Dock::paintEvent(QPaintEvent*) {
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
    QRectF r = rect().adjusted(1, 1, -1, -5);
    QPainterPath path; path.addRoundedRect(r, 20, 20);
    p.fillPath(path, QColor(20, 12, 40, m_alpha));
    p.setPen(QPen(QColor(m_accent.red(), m_accent.green(), m_accent.blue(), 90), 1)); p.drawPath(path);
    // неоновая полоса под Dock, как в референсе
    QLinearGradient g(width() * 0.3, 0, width() * 0.7, 0);
    QColor a = m_accent; a.setAlpha(0); g.setColorAt(0, a); a.setAlpha(255); g.setColorAt(0.5, a); a.setAlpha(0); g.setColorAt(1, a);
    p.setPen(Qt::NoPen); p.setBrush(g); p.drawRoundedRect(QRectF(width() * 0.3, height() - 4, width() * 0.4, 3), 1.5, 1.5);
}
