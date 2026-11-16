#pragma once
#include <QPointF>
#include <QSize>
#include <QVector>

// Чистая математика PC Mouse Engine (покрыта тестами). Линейная, без ускорения.
struct SensConfig {
    double sensX = 1.0, sensY = 1.0;   // множители чувствительности
    double adsMul = 0.6;               // множитель при прицеливании
    int dpi = 800;                     // DPI мыши: нормирует ощущения при смене DPI
    bool smoothing = false;
    double smoothAlpha = 0.35;         // 0..1, больше = меньше сглаживания
};

enum class Anchor { Left, Center, Right };

namespace MouseMath {
constexpr double kRefDpi = 800.0;
constexpr double kRefW = 1920.0, kRefH = 1080.0;
// отсчёты мыши -> смещение пальца в пикселях Android (масштаб по высоте экрана)
QPointF countsToPixels(double dx, double dy, const SensConfig& c, bool ads, int screenH);
// точка раскладки в координатах 1920x1080 + привязка к краю -> точка на любом соотношении сторон
QPointF mapPoint(QPointF ref, Anchor a, QSize screen);
double mapLength(double refLen, QSize screen);
Anchor anchorFromString(const QString& s);
QString anchorToString(Anchor a);
}

class Smoother {
public:
    QPointF step(QPointF in, const SensConfig& c);
    void reset() { m_acc = {}; m_has = false; }
private:
    QPointF m_acc; bool m_has = false;
};

// Палец обзора: держится в зоне камеры, при выходе за радиус поднимается и ставится в центр.
struct TouchOp { enum Kind { Down, Move, Up } kind; QPointF pos; };
class CameraFinger {
public:
    void configure(QPointF anchor, double radius) { m_anchor = anchor; m_radius = radius; }
    QVector<TouchOp> move(QPointF delta);
    QVector<TouchOp> release();
    bool isDown() const { return m_down; }
    QPointF pos() const { return m_pos; }
private:
    QPointF m_anchor; double m_radius = 300; QPointF m_pos; bool m_down = false;
};

// Статистика задержек (скользящее окно).
class LatencyStats {
public:
    void add(double ms);
    int count() const { return m_v.size(); }
    double avg() const;
    double percentile(double p) const;
    double max() const;
    void clear() { m_v.clear(); }
private:
    QVector<double> m_v;
};
