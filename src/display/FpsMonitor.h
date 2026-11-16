#pragma once
#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include <QVector>
class AdbClient;

// Реальный FPS Android: счётчик выведенных кадров SurfaceFlinger за фактический интервал.
// Если образ не отдаёт счётчик, показывается «н/д», а не выдуманное число.
class FpsMonitor : public QObject {
    Q_OBJECT
public:
    FpsMonitor(AdbClient* adb, QObject* parent = nullptr);
    void start();
    void stop();
    static double fpsFrom(qint64 frames0, qint64 frames1, qint64 ns);
signals:
    void sample(double fps, double avgFrameMs, double jitterMs);   // fps < 0: недоступно
    void unavailable();
private:
    void poll();
    AdbClient* m_adb; QTimer m_timer; QElapsedTimer m_clock;
    qint64 m_lastFrames = -1, m_lastNs = 0; bool m_busy = false; int m_failures = 0;
    QVector<double> m_hist;
};
