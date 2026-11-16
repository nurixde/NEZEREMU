#include "Widgets.h"
#include "Icons.h"
#include "Theme.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>

QColor ToggleSwitch::accent = QColor("#8b5cf6");

ToggleSwitch::ToggleSwitch(QWidget* parent) : QAbstractButton(parent) {
    setCheckable(true); setCursor(Qt::PointingHandCursor);
    setFixedSize(sizeHint());
    m_anim = new QPropertyAnimation(this, "knob", this); m_anim->setDuration(140);
    connect(this, &QAbstractButton::toggled, this, [this](bool on) { m_anim->stop(); m_anim->setStartValue(m_pos); m_anim->setEndValue(on ? 1.0 : 0.0); m_anim->start(); });
}
void ToggleSwitch::checkStateSet() { m_pos = isChecked() ? 1 : 0; update(); }

void ToggleSwitch::paintEvent(QPaintEvent*) {
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
    QColor off("#3a2f5c");
    QColor bg = QColor::fromRgbF(off.redF() + (accent.redF() - off.redF()) * m_pos, off.greenF() + (accent.greenF() - off.greenF()) * m_pos, off.blueF() + (accent.blueF() - off.blueF()) * m_pos);
    if (!isEnabled()) bg = QColor("#2a2340");
    p.setPen(Qt::NoPen); p.setBrush(bg);
    p.drawRoundedRect(rect(), height() / 2.0, height() / 2.0);
    p.setBrush(Qt::white);
    const qreal d = height() - 6;
    p.drawEllipse(QRectF(3 + (width() - d - 6) * m_pos, 3, d, d));
}

Card::Card(const QString& icon, const QString& title, QWidget* parent) : QFrame(parent) {
    setObjectName("card");
    setStyleSheet(QString("QFrame#card{background:%1;border:1px solid %2;border-radius:14px;}").arg(Theme::kCard, Theme::kBorder));
    auto* v = new QVBoxLayout(this); v->setContentsMargins(18, 16, 18, 16); v->setSpacing(12);
    auto* head = new QHBoxLayout;
    auto* ic = new QLabel; ic->setPixmap(Icons::get(icon, ToggleSwitch::accent.lighter(130)).pixmap(20, 20));
    auto* t = new QLabel(title); t->setStyleSheet("font-size:15px;font-weight:600;background:transparent;");
    head->addWidget(ic); head->addWidget(t); head->addStretch();
    v->addLayout(head);
    m_body = new QVBoxLayout; m_body->setSpacing(12);
    v->addLayout(m_body);
}

QWidget* Card::addRow(const QString& title, const QString& hint, QWidget* control) {
    auto* row = new QWidget; row->setStyleSheet("background:transparent;");
    auto* h = new QHBoxLayout(row); h->setContentsMargins(0, 0, 0, 0);
    auto* texts = new QVBoxLayout; texts->setSpacing(1);
    auto* a = new QLabel(title); a->setStyleSheet("font-size:13px;");
    texts->addWidget(a);
    if (!hint.isEmpty()) { auto* b = new QLabel(hint); b->setWordWrap(true); b->setStyleSheet(QString("font-size:11px;color:%1;").arg(Theme::kMuted)); texts->addWidget(b); }
    h->addLayout(texts, 1);
    if (control) h->addWidget(control, 0, Qt::AlignRight | Qt::AlignVCenter);
    m_body->addWidget(row);
    return row;
}

QLabel* Card::addNote(const QString& text) {
    auto* l = new QLabel(text); l->setWordWrap(true); l->setTextInteractionFlags(Qt::TextSelectableByMouse);
    l->setStyleSheet(QString("font-size:11px;color:%1;background:transparent;").arg(Theme::kMuted));
    m_body->addWidget(l);
    return l;
}

Segmented::Segmented(const QStringList& labels, QWidget* parent) : QWidget(parent), m_group(new QButtonGroup(this)) {
    auto* h = new QHBoxLayout(this); h->setContentsMargins(0, 0, 0, 0); h->setSpacing(6);
    for (int i = 0; i < labels.size(); ++i) {
        auto* b = new QPushButton(labels[i]); b->setCheckable(true); b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet(QString("QPushButton{padding:7px 10px;font-size:12px;}QPushButton:checked{background:%1;border-color:%1;font-weight:600;}").arg(ToggleSwitch::accent.name()));
        m_group->addButton(b, i); h->addWidget(b);
    }
    connect(m_group, &QButtonGroup::idClicked, this, &Segmented::changed);
}
void Segmented::setCurrent(int i) { if (auto* b = m_group->button(i)) b->setChecked(true); }
