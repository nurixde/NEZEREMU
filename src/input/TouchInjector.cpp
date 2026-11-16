#include "TouchInjector.h"
#include "engine/EmuConsole.h"
#include <cmath>

QPoint TouchInjector::toDevice(QPointF px) const {
    const double x = qBound(0.0, px.x(), double(m_screen.width() - 1));
    const double y = qBound(0.0, px.y(), double(m_screen.height() - 1));
    if (m_absMax <= 0) return {int(std::lround(x)), int(std::lround(y))};
    return {int(std::lround(x * m_absMax / (m_screen.width() - 1))), int(std::lround(y * m_absMax / (m_screen.height() - 1)))};
}

QByteArray TouchInjector::buildDown(int slot, int id, QPointF px, bool first) const {
    const QPoint d = toDevice(px);
    QByteArray s = "event send EV_ABS:ABS_MT_SLOT:" + QByteArray::number(slot) + " EV_ABS:ABS_MT_TRACKING_ID:" + QByteArray::number(id) +
                   " EV_ABS:ABS_MT_POSITION_X:" + QByteArray::number(d.x()) + " EV_ABS:ABS_MT_POSITION_Y:" + QByteArray::number(d.y()) +
                   " EV_ABS:ABS_MT_PRESSURE:100";
    if (first) s += " EV_KEY:BTN_TOUCH:1";
    return s + " EV_SYN:SYN_REPORT:0";
}

QByteArray TouchInjector::buildMove(int slot, QPointF px) const {
    const QPoint d = toDevice(px);
    return "event send EV_ABS:ABS_MT_SLOT:" + QByteArray::number(slot) + " EV_ABS:ABS_MT_POSITION_X:" + QByteArray::number(d.x()) +
           " EV_ABS:ABS_MT_POSITION_Y:" + QByteArray::number(d.y()) + " EV_SYN:SYN_REPORT:0";
}

QByteArray TouchInjector::buildUp(int slot, bool last) const {
    QByteArray s = "event send EV_ABS:ABS_MT_SLOT:" + QByteArray::number(slot) + " EV_ABS:ABS_MT_TRACKING_ID:-1";
    if (last) s += " EV_KEY:BTN_TOUCH:0";
    return s + " EV_SYN:SYN_REPORT:0";
}

int TouchInjector::activeCount() const { int n = 0; for (bool b : m_down) n += b; return n; }

void TouchInjector::down(int slot, QPointF px, qint64 ts) {
    if (slot < 0 || slot >= kSlots) return;
    if (m_down[slot]) { move(slot, px, ts); return; }
    const bool first = activeCount() == 0;
    m_down[slot] = true;
    m_console->send(buildDown(slot, m_nextId++, px, first), ts);
}

void TouchInjector::move(int slot, QPointF px, qint64 ts) {
    if (slot < 0 || slot >= kSlots || !m_down[slot]) return;
    m_console->send(buildMove(slot, px), ts);
}

void TouchInjector::up(int slot, qint64 ts) {
    if (slot < 0 || slot >= kSlots || !m_down[slot]) return;
    m_down[slot] = false;
    m_console->send(buildUp(slot, activeCount() == 0), ts);
}

void TouchInjector::releaseAll() { for (int i = 0; i < kSlots; ++i) up(i); }
