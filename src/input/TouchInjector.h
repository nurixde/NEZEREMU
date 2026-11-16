#pragma once
#include <QByteArray>
#include <QPointF>
#include <QSize>
class EmuConsole;

// Мультитач (протокол B) через консоль эмулятора: одна TCP-строка на событие, без запуска процессов.
class TouchInjector {
public:
    static constexpr int kSlots = 10;
    explicit TouchInjector(EmuConsole* c) : m_console(c) {}
    void setScreen(QSize s) { m_screen = s; }
    void setAbsMax(int v) { m_absMax = v; }   // 0 = координаты в пикселях экрана, иначе 0..absMax
    void down(int slot, QPointF px, qint64 ts = 0);
    void move(int slot, QPointF px, qint64 ts = 0);
    void up(int slot, qint64 ts = 0);
    void releaseAll();
    bool isDown(int slot) const { return slot >= 0 && slot < kSlots && m_down[slot]; }
    QByteArray buildDown(int slot, int trackingId, QPointF px, bool firstFinger) const;
    QByteArray buildMove(int slot, QPointF px) const;
    QByteArray buildUp(int slot, bool lastFinger) const;
    QPoint toDevice(QPointF px) const;
private:
    int activeCount() const;
    EmuConsole* m_console; QSize m_screen{1920, 1080}; int m_absMax = 0;
    bool m_down[kSlots]{}; int m_nextId = 1;
};
