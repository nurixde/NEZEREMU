#pragma once
#include <QColor>
#include <QList>
#include <QString>

namespace Theme {
// Стиль оформления: цвета окон, панелей и акцент по умолчанию.
struct Style { QString id, name, bg, card, border, input, side, hover, muted, accent; };

// Текущие цвета (меняются applyStyle).
inline QString kBg = "#0f0a1e";        // фон окон
inline QString kCard = "#1b1333";      // карточки
inline QString kBorder = "#2e2152";    // обводки
inline QString kInput = "#140e28";     // поля ввода
inline QString kSide = "#120c25";      // боковое меню настроек
inline QString kHover = "#22183f";     // наведение
inline QString kMuted = "#a59ec4";     // подписи
inline const QString kText = "#ffffff";
inline const QString kDefaultAccent = "#8b5cf6";

const QList<Style>& styles();
const Style& style(const QString& id);
void applyStyle(const QString& id);
QColor accent(const QString& stored);
QString globalStyle(const QString& accent);
}
