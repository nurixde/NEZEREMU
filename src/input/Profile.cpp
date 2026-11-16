#include "Profile.h"
#include "core/Paths.h"
#include <QDir>
#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>

static QJsonArray pt(QPointF p) { return {p.x(), p.y()}; }
static QPointF ptFrom(const QJsonValue& v, QPointF d) { const auto a = v.toArray(); return a.size() == 2 ? QPointF(a[0].toDouble(), a[1].toDouble()) : d; }

QJsonObject Profile::toJson() const {
    QJsonObject o;
    o["name"] = name; o["package"] = package;
    o["joy"] = QJsonObject{{"pos", pt(joyCenter)}, {"r", joyRadius}, {"a", MouseMath::anchorToString(joyAnchor)}};
    o["cam"] = QJsonObject{{"pos", pt(camAnchor)}, {"r", camRadius}, {"a", MouseMath::anchorToString(camAnchor_a)}};
    o["fire"] = QJsonObject{{"pos", pt(fire)}, {"a", MouseMath::anchorToString(fireAnchor)}};
    o["scope"] = QJsonObject{{"pos", pt(scope)}, {"a", MouseMath::anchorToString(scopeAnchor)}, {"toggle", adsToggle}};
    QJsonArray ks;
    for (const auto& k : keys) ks.append(QJsonObject{{"key", k.key}, {"pos", pt(k.pos)}, {"a", MouseMath::anchorToString(k.anchor)}, {"label", k.label}});
    o["keys"] = ks;
    return o;
}

Profile Profile::fromJson(const QJsonObject& o) {
    Profile p;
    p.name = o.value("name").toString(p.name); p.package = o.value("package").toString();
    const auto j = o.value("joy").toObject(), c = o.value("cam").toObject(), f = o.value("fire").toObject(), s = o.value("scope").toObject();
    p.joyCenter = ptFrom(j["pos"], p.joyCenter); p.joyRadius = j.value("r").toDouble(p.joyRadius); p.joyAnchor = MouseMath::anchorFromString(j.value("a").toString("L"));
    p.camAnchor = ptFrom(c["pos"], p.camAnchor); p.camRadius = c.value("r").toDouble(p.camRadius); p.camAnchor_a = MouseMath::anchorFromString(c.value("a").toString("R"));
    p.fire = ptFrom(f["pos"], p.fire); p.fireAnchor = MouseMath::anchorFromString(f.value("a").toString("R"));
    p.scope = ptFrom(s["pos"], p.scope); p.scopeAnchor = MouseMath::anchorFromString(s.value("a").toString("R")); p.adsToggle = s.value("toggle").toBool();
    for (const auto& v : o.value("keys").toArray()) {
        const auto k = v.toObject();
        p.keys.append({k.value("key").toString(), ptFrom(k["pos"], {}), MouseMath::anchorFromString(k.value("a").toString("R")), k.value("label").toString()});
    }
    return p;
}

// Базовая раскладка-шаблон. Позиции кнопок в каждой игре свои: проверьте и сдвиньте их в редакторе.
Profile Profile::builtinShooter() {
    Profile p; p.name = "Default";
    p.keys = {{"Space", {1800, 760}, Anchor::Right, "Прыжок"}, {"C", {1680, 980}, Anchor::Right, "Присесть"},
              {"Z", {1820, 980}, Anchor::Right, "Лечь"}, {"R", {1520, 980}, Anchor::Right, "Перезарядка"},
              {"F", {1250, 600}, Anchor::Center, "Действие"}, {"G", {1400, 980}, Anchor::Right, "Граната"},
              {"1", {820, 980}, Anchor::Center, "Оружие 1"}, {"2", {1100, 980}, Anchor::Center, "Оружие 2"},
              {"Tab", {120, 80}, Anchor::Left, "Рюкзак"}, {"M", {1800, 120}, Anchor::Right, "Карта"}};
    return p;
}

namespace Profiles {
static QString file(const QString& n) { return Paths::profilesDir() + "/" + n + ".json"; }
QStringList names() {
    QStringList l;
    for (const QString& f : QDir(Paths::profilesDir()).entryList({"*.json"}, QDir::Files)) l << f.chopped(5);
    if (!l.contains("Default")) l.prepend("Default");
    return l;
}
Profile load(const QString& n) {
    QFile f(file(n));
    if (!f.open(QIODevice::ReadOnly)) return Profile::builtinShooter();
    Profile p = Profile::fromJson(QJsonDocument::fromJson(f.readAll()).object());
    p.name = n;
    return p;
}
bool save(const Profile& p) {
    QSaveFile f(file(p.name));
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(QJsonDocument(p.toJson()).toJson());
    return f.commit();
}
bool remove(const QString& n) { return n != "Default" && QFile::remove(file(n)); }
QString forPackage(const QString& pkg) {
    for (const QString& n : names()) if (!pkg.isEmpty() && load(n).package == pkg) return n;
    return {};
}
static const QHash<QString, int>& table() {
    static const QHash<QString, int> t = [] {
        QHash<QString, int> h;
        for (char c = 'A'; c <= 'Z'; ++c) h.insert(QString(QChar::fromLatin1(c)), c);
        for (char c = '0'; c <= '9'; ++c) h.insert(QString(QChar::fromLatin1(c)), c);
        h.insert("Space", 0x20); h.insert("Tab", 0x09); h.insert("Shift", 0x10); h.insert("Ctrl", 0x11); h.insert("Alt", 0x12);
        h.insert("Esc", 0x1B); h.insert("Enter", 0x0D); h.insert("CapsLock", 0x14); h.insert("`", 0xC0);
        for (int i = 1; i <= 12; ++i) h.insert("F" + QString::number(i), 0x6F + i);
        h.insert("Mouse4", 0x05); h.insert("Mouse5", 0x06);
        return h;
    }();
    return t;
}
int keyFromName(const QString& n) { return table().value(n, 0); }
QString keyName(int vk) { return table().key(vk, QString("VK%1").arg(vk)); }
}
