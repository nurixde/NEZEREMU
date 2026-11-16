#include "MouseMath.h"
#include <QString>
#include <algorithm>
#include <cmath>

namespace MouseMath {
QPointF countsToPixels(double dx, double dy, const SensConfig& c, bool ads, int screenH) {
    const double dpiNorm = kRefDpi / qMax(100, c.dpi);
    const double scale = double(screenH) / kRefH;
    const double m = ads ? c.adsMul : 1.0;
    return {dx * c.sensX * dpiNorm * scale * m, dy * c.sensY * dpiNorm * scale * m};
}

QPointF mapPoint(QPointF r, Anchor a, QSize s) {
    const double k = s.height() / kRefH;
    const double y = r.y() * k;
    switch (a) {
    case Anchor::Left: return {r.x() * k, y};
    case Anchor::Right: return {s.width() - (kRefW - r.x()) * k, y};
    default: return {s.width() / 2.0 + (r.x() - kRefW / 2) * k, y};
    }
}

double mapLength(double len, QSize s) { return len * s.height() / kRefH; }
Anchor anchorFromString(const QString& s) { return s == "L" ? Anchor::Left : s == "R" ? Anchor::Right : Anchor::Center; }
QString anchorToString(Anchor a) { return a == Anchor::Left ? "L" : a == Anchor::Right ? "R" : "C"; }
}

QPointF Smoother::step(QPointF in, const SensConfig& c) {
    if (!c.smoothing) return in;
    const double a = qBound(0.05, c.smoothAlpha, 1.0);
    if (!m_has) { m_acc = in; m_has = true; return in; }
    m_acc = m_acc * (1 - a) + in * a;
    return m_acc;
}

QVector<TouchOp> CameraFinger::move(QPointF d) {
    QVector<TouchOp> ops;
    if (!m_down) { m_pos = m_anchor; m_down = true; ops.push_back({TouchOp::Down, m_pos}); }
    QPointF next = m_pos + d;
    const QPointF off = next - m_anchor;
    if (std::hypot(off.x(), off.y()) > m_radius) {
        ops.push_back({TouchOp::Up, m_pos});
        m_pos = m_anchor;
        ops.push_back({TouchOp::Down, m_pos});
        next = m_pos + d;
    }
    m_pos = next;
    ops.push_back({TouchOp::Move, m_pos});
    return ops;
}

QVector<TouchOp> CameraFinger::release() {
    QVector<TouchOp> ops;
    if (m_down) { ops.push_back({TouchOp::Up, m_pos}); m_down = false; }
    return ops;
}

void LatencyStats::add(double ms) { m_v.push_back(ms); if (m_v.size() > 1000) m_v.remove(0, m_v.size() - 1000); }
double LatencyStats::avg() const { if (m_v.isEmpty()) return 0; double s = 0; for (double v : m_v) s += v; return s / m_v.size(); }
double LatencyStats::max() const { return m_v.isEmpty() ? 0 : *std::max_element(m_v.begin(), m_v.end()); }
double LatencyStats::percentile(double p) const {
    if (m_v.isEmpty()) return 0;
    QVector<double> s = m_v; std::sort(s.begin(), s.end());
    const int i = qBound(0, int(std::ceil(p / 100.0 * s.size())) - 1, int(s.size()) - 1);
    return s[i];
}
