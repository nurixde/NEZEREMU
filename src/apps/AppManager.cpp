#include "AppManager.h"
#include "engine/AdbParsers.h"
#include "settings/SettingsStore.h"
#include <QHash>
#include <QPainter>
#include <QPainterPath>

AppManager::AppManager(SettingsStore* s, QObject* parent) : QObject(parent), m_s(s) {}

QStringList AppManager::autoPinCandidates() {
    return {"com.android.vending", "com.dts.freefireth", "com.dts.freefiremax", "com.tencent.ig",
            "com.axlebolt.standoff2", "com.roblox.client"};
}

void AppManager::setInstalled(const QStringList& pk) {
    m_installed = pk;
    // Игры и Play Маркет появляются в Dock автоматически после установки (если пользователь их не убирал).
    QStringList pinned = m_s->value("dock.pinned", QStringList()).toStringList();
    const QStringList removed = m_s->value("dock.removed", QStringList()).toStringList();
    bool changed = false;
    for (const QString& c : autoPinCandidates())
        if (pk.contains(c) && !pinned.contains(c) && !removed.contains(c)) { pinned << c; changed = true; }
    if (changed) { m_s->setValue("dock.pinned", pinned); m_s->save(); }
    emit dockChanged();
}

AppEntry AppManager::entry(const QString& pkg) const {
    const QString label = AdbParsers::displayLabel(pkg);
    return {pkg, label, iconFor(pkg, label)};
}

QList<AppEntry> AppManager::installed() const { QList<AppEntry> l; for (const auto& p : m_installed) l << entry(p); return l; }

QList<AppEntry> AppManager::dockApps() const {
    QList<AppEntry> l;
    for (const QString& p : m_s->value("dock.pinned", QStringList()).toStringList())
        if (m_installed.contains(p)) l << entry(p);
    return l;
}

bool AppManager::isPinned(const QString& pkg) const { return m_s->value("dock.pinned", QStringList()).toStringList().contains(pkg); }

void AppManager::pin(const QString& pkg) {
    QStringList p = m_s->value("dock.pinned", QStringList()).toStringList();
    QStringList r = m_s->value("dock.removed", QStringList()).toStringList();
    if (!p.contains(pkg)) p << pkg; r.removeAll(pkg);
    m_s->setValue("dock.pinned", p); m_s->setValue("dock.removed", r); m_s->save();
    emit dockChanged();
}

void AppManager::unpin(const QString& pkg) {
    QStringList p = m_s->value("dock.pinned", QStringList()).toStringList();
    QStringList r = m_s->value("dock.removed", QStringList()).toStringList();
    p.removeAll(pkg); if (!r.contains(pkg)) r << pkg;
    m_s->setValue("dock.pinned", p); m_s->setValue("dock.removed", r); m_s->save();
    emit dockChanged();
}

// Упрощённая иконка: первая буква на цветной плитке (настоящие иконки APK пока не извлекаются).
QIcon AppManager::iconFor(const QString& pkg, const QString& label) {
    static const QHash<QString, QColor> brand = {
        {"com.android.vending", QColor("#01875f")}, {"com.dts.freefireth", QColor("#f59e0b")}, {"com.dts.freefiremax", QColor("#ea580c")},
        {"com.tencent.ig", QColor("#c9a227")}, {"com.axlebolt.standoff2", QColor("#475569")}, {"com.roblox.client", QColor("#334155")},
        {"com.android.chrome", QColor("#2563eb")}, {"com.google.android.documentsui", QColor("#0ea5e9")}};
    QColor c = brand.value(pkg, QColor::fromHsv(int(qHash(pkg) % 360), 150, 200));
    QPixmap pm(128, 128); pm.fill(Qt::transparent);
    QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
    QPainterPath path; path.addRoundedRect(4, 4, 120, 120, 28, 28);
    QLinearGradient g(0, 0, 0, 128); g.setColorAt(0, c.lighter(125)); g.setColorAt(1, c.darker(120));
    p.fillPath(path, g);
    QFont f("Segoe UI"); f.setPixelSize(60); f.setBold(true); p.setFont(f); p.setPen(Qt::white);
    p.drawText(QRect(0, 0, 128, 128), Qt::AlignCenter, label.left(1).toUpper());
    return QIcon(pm);
}
