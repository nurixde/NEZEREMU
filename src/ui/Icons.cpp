#include "Icons.h"
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>

namespace Icons {
QIcon get(const QString& n, const QColor& col, int S) {
    QPixmap pm(S, S); pm.fill(Qt::transparent);
    QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
    const double u = S / 24.0;
    p.scale(u, u);
    QPen pen(col, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin); p.setPen(pen); p.setBrush(Qt::NoBrush);
    if (n == "keyboard") {
        p.drawRoundedRect(QRectF(2.5, 6, 19, 12), 2.5, 2.5);
        for (int i = 0; i < 5; ++i) p.drawPoint(QPointF(6 + i * 3, 10));
        for (int i = 0; i < 4; ++i) p.drawPoint(QPointF(7.5 + i * 3, 12.5));
        p.drawLine(QPointF(8, 15), QPointF(16, 15));
    } else if (n == "volume") {
        QPainterPath s; s.moveTo(4, 9.5); s.lineTo(7.5, 9.5); s.lineTo(12, 5.5); s.lineTo(12, 18.5); s.lineTo(7.5, 14.5); s.lineTo(4, 14.5); s.closeSubpath(); p.drawPath(s);
        p.drawArc(QRectF(11, 8, 6, 8), -60 * 16, 120 * 16); p.drawArc(QRectF(11, 5, 10, 14), -60 * 16, 120 * 16);
    } else if (n == "rotate") {
        p.drawArc(QRectF(4, 4, 16, 16), 40 * 16, 280 * 16);
        p.drawLine(QPointF(17.5, 3.5), QPointF(18.3, 7.6)); p.drawLine(QPointF(18.3, 7.6), QPointF(14.2, 8.2));
    } else if (n == "fullscreen") {
        p.drawPolyline(QPolygonF({{4, 9}, {4, 4}, {9, 4}})); p.drawPolyline(QPolygonF({{15, 4}, {20, 4}, {20, 9}}));
        p.drawPolyline(QPolygonF({{20, 15}, {20, 20}, {15, 20}})); p.drawPolyline(QPolygonF({{9, 20}, {4, 20}, {4, 15}}));
    } else if (n == "gamepad") {
        p.drawRoundedRect(QRectF(2.5, 7, 19, 11), 5.5, 5.5);
        p.drawLine(QPointF(7, 10.5), QPointF(7, 14.5)); p.drawLine(QPointF(5, 12.5), QPointF(9, 12.5));
        p.drawPoint(QPointF(16, 11)); p.drawPoint(QPointF(18, 13.5));
    } else if (n == "apk") {
        p.drawRoundedRect(QRectF(3, 3, 18, 18), 3, 3);
        QFont f("Segoe UI"); f.setPixelSize(7); f.setBold(true); p.setFont(f);
        p.drawText(QRectF(3, 3, 18, 18), Qt::AlignCenter, "APK");
    } else if (n == "scissors") {
        p.drawEllipse(QRectF(3.5, 14, 5, 5)); p.drawEllipse(QRectF(15.5, 14, 5, 5));
        p.drawLine(QPointF(7.5, 14.5), QPointF(17, 4)); p.drawLine(QPointF(16.5, 14.5), QPointF(7, 4));
    } else if (n == "home") {
        p.drawPolyline(QPolygonF({{3.5, 11}, {12, 4}, {20.5, 11}})); p.drawPolyline(QPolygonF({{6, 9.5}, {6, 20}, {18, 20}, {18, 9.5}}));
    } else if (n == "android") {
        p.drawRoundedRect(QRectF(6, 2.5, 12, 19), 2.5, 2.5); p.drawLine(QPointF(10.5, 18.5), QPointF(13.5, 18.5));
    } else if (n == "more") {
        p.setBrush(col); for (int i = 0; i < 3; ++i) p.drawEllipse(QPointF(12, 6 + i * 6), 1.3, 1.3);
    } else if (n == "settings") {
        p.drawEllipse(QPointF(12, 12), 3, 3);
        for (int i = 0; i < 8; ++i) { p.save(); p.translate(12, 12); p.rotate(i * 45); p.drawLine(QPointF(0, -6), QPointF(0, -8.8)); p.restore(); }
        p.drawEllipse(QPointF(12, 12), 6.2, 6.2);
    } else if (n == "plus") {
        p.drawLine(QPointF(12, 5), QPointF(12, 19)); p.drawLine(QPointF(5, 12), QPointF(19, 12));
    } else if (n == "grid") {
        for (int i = 0; i < 3; ++i) for (int j = 0; j < 3; ++j) p.drawRoundedRect(QRectF(4 + i * 6, 4 + j * 6, 4, 4), 1, 1);
    } else if (n == "lock") {
        p.drawRoundedRect(QRectF(5, 10.5, 14, 10), 2, 2); p.drawArc(QRectF(8, 4, 8, 11), 0, 180 * 16);
        p.drawLine(QPointF(8, 9.5), QPointF(8, 10.5)); p.drawLine(QPointF(16, 9.5), QPointF(16, 10.5));
    } else if (n == "min") { p.drawLine(QPointF(6, 12), QPointF(18, 12));
    } else if (n == "max") { p.drawRect(QRectF(6.5, 6.5, 11, 11));
    } else if (n == "close") { p.drawLine(QPointF(7, 7), QPointF(17, 17)); p.drawLine(QPointF(17, 7), QPointF(7, 17));
    } else if (n == "landscape") { p.drawRoundedRect(QRectF(3, 7, 18, 10), 2, 2);
    } else if (n == "portrait") { p.drawRoundedRect(QRectF(7, 3, 10, 18), 2, 2);
    } else if (n == "chip") {
        p.drawRoundedRect(QRectF(6, 6, 12, 12), 2, 2); p.drawRect(QRectF(9.5, 9.5, 5, 5));
        for (int i = 0; i < 3; ++i) { const double k = 9 + i * 3; p.drawLine(QPointF(k, 3), QPointF(k, 6)); p.drawLine(QPointF(k, 18), QPointF(k, 21)); p.drawLine(QPointF(3, k), QPointF(6, k)); p.drawLine(QPointF(18, k), QPointF(21, k)); }
    } else if (n == "monitor") { p.drawRoundedRect(QRectF(3, 4, 18, 12), 2, 2); p.drawLine(QPointF(9, 20), QPointF(15, 20)); p.drawLine(QPointF(12, 16), QPointF(12, 20));
    } else if (n == "brush") { p.drawLine(QPointF(19, 4), QPointF(10, 13)); p.drawEllipse(QRectF(4, 13.5, 7, 7));
    } else if (n == "general") { p.drawEllipse(QPointF(12, 12), 8.5, 8.5); p.drawLine(QPointF(12, 11), QPointF(12, 16.5)); p.setBrush(col); p.drawEllipse(QPointF(12, 7.8), 0.9, 0.9);
    } else if (n == "graphics") { p.drawRoundedRect(QRectF(3, 5, 18, 14), 2, 2); p.drawPolyline(QPolygonF({{6, 16}, {10, 11}, {13, 14}, {15.5, 11.5}, {18, 16}}));
    } else if (n == "mouse") { p.drawRoundedRect(QRectF(7, 3, 10, 18), 5, 5); p.drawLine(QPointF(12, 6), QPointF(12, 10));
    } else if (n == "extra") { p.drawLine(QPointF(4, 7), QPointF(20, 7)); p.drawLine(QPointF(4, 12), QPointF(20, 12)); p.drawLine(QPointF(4, 17), QPointF(20, 17));
    }
    return QIcon(pm);
}
}
