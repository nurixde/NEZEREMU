#include "SideToolbar.h"
#include "Icons.h"
#include <QPainter>
#include <QToolButton>
#include <QVBoxLayout>

SideToolbar::SideToolbar(QWidget* parent) : QWidget(parent) {
    setFixedWidth(kWidth);
    auto* v = new QVBoxLayout(this); v->setContentsMargins(5, 12, 5, 12); v->setSpacing(6);
    struct It { const char* id; const char* icon; const char* tip; };
    const It items[] = {
        {"keymap", "keyboard", "Игровое управление клавиатурой и мышью (захват курсора: ` )"},
        {"volume", "volume", "Громкость"}, {"rotate", "rotate", "Повернуть экран"},
        {"fullscreen", "fullscreen", "Полный экран (F11)"}, {"profiles", "gamepad", "Профили управления"},
        {"apk", "apk", "Установить APK"}, {"screenshot", "scissors", "Скриншот"},
        {"android", "android", "Экран Android"}, {"home", "home", "Рабочий стол NEZER EMU"}};
    for (const It& it : items) {
        auto* b = new QToolButton(this);
        b->setIcon(Icons::get(it.icon, QColor("#e8e2ff"))); b->setIconSize(QSize(20, 20)); b->setFixedSize(40, 40);
        b->setToolTip(it.tip); b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet("QToolButton{border:none;border-radius:10px;background:transparent;}QToolButton:hover{background:rgba(255,255,255,26);}"
                         "QToolButton:checked{background:rgba(139,92,246,150);}QToolButton:disabled{opacity:0.4;}");
        const QString id = it.id;
        connect(b, &QToolButton::clicked, this, [this, id] { emit action(id); });
        m_btn.insert(id, b); v->addWidget(b, 0, Qt::AlignHCenter);
    }
    m_btn["keymap"]->setCheckable(true);
    v->addStretch();
    auto* more = new QToolButton(this); more->setIcon(Icons::get("more", QColor("#e8e2ff"))); more->setIconSize(QSize(20, 20)); more->setFixedSize(40, 40);
    more->setToolTip("Настройки"); more->setCursor(Qt::PointingHandCursor);
    more->setStyleSheet(m_btn["home"]->styleSheet());
    connect(more, &QToolButton::clicked, this, [this] { emit action("settings"); });
    m_btn.insert("settings", more); v->addWidget(more, 0, Qt::AlignHCenter);
    setAndroidReady(false);
}

void SideToolbar::setAndroidReady(bool on) {
    for (const char* id : {"keymap", "volume", "rotate", "apk", "screenshot"}) m_btn[id]->setEnabled(on);
}

void SideToolbar::setChecked(const QString& id, bool on) { if (auto* b = m_btn.value(id)) b->setChecked(on); }

void SideToolbar::paintEvent(QPaintEvent*) {
    QPainter p(this); p.fillRect(rect(), QColor(13, 8, 26, 235));
    p.setPen(QColor(255, 255, 255, 18)); p.drawLine(0, 0, 0, height());
}
