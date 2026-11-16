#pragma once
#include "MouseMath.h"
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

// Профиль управления для игры. Координаты в системе 1920x1080 + привязка к краю (L/C/R).
struct KeyBind { QString key; QPointF pos; Anchor anchor = Anchor::Right; QString label; };
struct Profile {
    QString name = "Default";
    QString package;
    QPointF joyCenter{300, 800}; double joyRadius = 130; Anchor joyAnchor = Anchor::Left;
    QPointF camAnchor{1350, 480}; double camRadius = 320; Anchor camAnchor_a = Anchor::Right;
    QPointF fire{1650, 820}; Anchor fireAnchor = Anchor::Right;
    QPointF scope{1790, 560}; Anchor scopeAnchor = Anchor::Right; bool adsToggle = false;
    QList<KeyBind> keys;

    QJsonObject toJson() const;
    static Profile fromJson(const QJsonObject& o);
    static Profile builtinShooter();
};

namespace Profiles {
QStringList names();
Profile load(const QString& name);
bool save(const Profile& p);
bool remove(const QString& name);
QString forPackage(const QString& pkg);   // имя профиля, привязанного к игре, или пусто
int keyFromName(const QString& name);     // имя клавиши -> Windows virtual-key
QString keyName(int vk);
}
