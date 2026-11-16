#include <QtTest>
#include "input/MouseMath.h"

class TestMouseMath : public QObject {
    Q_OBJECT
private slots:
    void linearNoAcceleration() {
        SensConfig c;
        const QPointF one = MouseMath::countsToPixels(1, 1, c, false, 1080);
        QPointF sum; for (int i = 0; i < 500; ++i) sum += MouseMath::countsToPixels(1, -1, c, false, 1080);
        const QPointF big = MouseMath::countsToPixels(500, -500, c, false, 1080);
        QVERIFY(qAbs(sum.x() - big.x()) < 1e-9 && qAbs(sum.y() - big.y()) < 1e-9);
        QCOMPARE(MouseMath::countsToPixels(10, 0, c, false, 1080).x(), 10 * one.x());
    }
    void dpiNormalization() {
        SensConfig a; a.dpi = 800; SensConfig b; b.dpi = 1600;
        QCOMPARE(MouseMath::countsToPixels(800, 0, a, false, 1080).x(), MouseMath::countsToPixels(1600, 0, b, false, 1080).x());
    }
    void adsAndAxes() {
        SensConfig c; c.sensX = 2; c.sensY = 0.5; c.adsMul = 0.25;
        const QPointF n = MouseMath::countsToPixels(100, 100, c, false, 1080), a = MouseMath::countsToPixels(100, 100, c, true, 1080);
        QCOMPARE(n.x(), 200.0); QCOMPARE(n.y(), 50.0); QCOMPARE(a.x(), 50.0); QCOMPARE(a.y(), 12.5);
    }
    void resolutionScaling() {
        SensConfig c;
        QCOMPARE(MouseMath::countsToPixels(100, 0, c, false, 1440).x(), 100.0 * 1440 / 1080);
    }
    void aspectAnchors() {
        const QPointF fire(1650, 820);
        QCOMPARE(MouseMath::mapPoint(fire, Anchor::Right, {1920, 1080}), fire);
        QCOMPARE(MouseMath::mapPoint(fire, Anchor::Right, {2560, 1080}).x(), 2560.0 - 270);
        QCOMPARE(MouseMath::mapPoint(fire, Anchor::Right, {1440, 1080}).x(), 1440.0 - 270);
        QCOMPARE(MouseMath::mapPoint({300, 800}, Anchor::Left, {2560, 1080}).x(), 300.0);
        QCOMPARE(MouseMath::mapPoint({960, 540}, Anchor::Center, {2560, 1080}), QPointF(1280, 540));
        QCOMPARE(MouseMath::mapPoint(fire, Anchor::Right, {3840, 2160}), QPointF(3840 - 540, 1640));
    }
    void cameraRecenter() {
        CameraFinger f; f.configure({1000, 500}, 100);
        auto ops = f.move({10, 0});
        QCOMPARE(ops.size(), 2); QCOMPARE(int(ops[0].kind), int(TouchOp::Down)); QCOMPARE(ops[1].pos, QPointF(1010, 500));
        ops = f.move({95, 0});
        QCOMPARE(ops.size(), 3); QCOMPARE(int(ops[0].kind), int(TouchOp::Up)); QCOMPARE(ops[1].pos, QPointF(1000, 500)); QCOMPARE(ops[2].pos, QPointF(1095, 500));
        QCOMPARE(f.release().size(), 1); QVERIFY(!f.isDown());
    }
    void smoothing() {
        SensConfig c; Smoother s;
        QCOMPARE(s.step({3, 4}, c), QPointF(3, 4)); QCOMPARE(s.step({-7, 1}, c), QPointF(-7, 1));
        c.smoothing = true; c.smoothAlpha = 0.5; s.reset();
        QCOMPARE(s.step({10, 0}, c), QPointF(10, 0)); QCOMPARE(s.step({0, 0}, c), QPointF(5, 0));
    }
    void latencyStats() {
        LatencyStats l; for (int i = 1; i <= 100; ++i) l.add(i);
        QCOMPARE(l.avg(), 50.5); QCOMPARE(l.percentile(95), 95.0); QCOMPARE(l.max(), 100.0);
    }
};
QTEST_GUILESS_MAIN(TestMouseMath)
#include "test_mousemath.moc"
