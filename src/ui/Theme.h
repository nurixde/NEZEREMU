#pragma once
#include <QColor>
#include <QString>

namespace Theme {
inline const QString kBg = "#0f0a1e";        // фон окон
inline const QString kCard = "#1b1333";      // карточки
inline const QString kBorder = "#2e2152";    // обводки
inline const QString kText = "#ffffff";
inline const QString kMuted = "#a59ec4";     // подписи
inline const QString kDefaultAccent = "#8b5cf6";
QColor accent(const QString& stored);
QString globalStyle(const QString& accent);
}
