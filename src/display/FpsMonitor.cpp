#include "FpsMonitor.h"
#include "engine/AdbClient.h"
#include "engine/AdbParsers.h"
#include <cmath>

FpsMonitor::FpsMonitor(AdbClient* adb, QObject* parent) : QObject(parent), m_adb(adb) {
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, &FpsMonitor::poll);
}

void FpsMonitor::start() { m_lastFrames = -1; m_failures = 0; m_hist.clear(); m_clock.start(); m_timer.start(); }
void FpsMonitor::stop() { m_timer.stop(); }

double FpsMonitor::fpsFrom(qint64 f0, qint64 f1, qint64 ns) {
    if (f0 < 0 || f1 < f0 || ns <= 0) return -1;
    return double(f1 - f0) * 1e9 / double(ns);
}

void FpsMonitor::poll() {
    if (m_busy) return;
    m_busy = true;
    m_adb->shell("service call SurfaceFlinger 1013", [this](int code, QString out) {
        m_busy = false;
        const qint64 now = m_clock.nsecsElapsed();
        const qint64 frames = code == 0 ? AdbParsers::surfaceFlingerFrameCount(out) : -1;
        if (frames < 0) { if (++m_failures >= 3) { emit unavailable(); emit sample(-1, 0, 0); } return; }
        m_failures = 0;
        const double fps = fpsFrom(m_lastFrames, frames, now - m_lastNs);
        m_lastFrames = frames; m_lastNs = now;
        if (fps < 0) return;
        m_hist.append(fps); if (m_hist.size() > 10) m_hist.removeFirst();
        double sum = 0; for (double v : m_hist) sum += v;
        const double mean = sum / m_hist.size();
        double var = 0; for (double v : m_hist) var += (v - mean) * (v - mean);
        const double frameMs = fps > 0.5 ? 1000.0 / fps : 0;
        const double sdFps = std::sqrt(var / m_hist.size());
        const double jitter = mean > 0.5 ? std::abs(1000.0 / qMax(1.0, mean - sdFps) - 1000.0 / mean) : 0;
        emit sample(fps, frameMs, jitter);
    }, 4000);
}
