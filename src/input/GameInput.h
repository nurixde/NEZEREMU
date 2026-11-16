#pragma once
#include "MouseMath.h"
#include "Profile.h"
#include "TouchInjector.h"
#include <QHash>
#include <QObject>
#include <QSet>
class SettingsStore; class EmuConsole; class RawInput; class QWidget;

// PC Mouse Engine: Raw Input -> чувствительность/ADS/сглаживание -> мультитач в Android.
class GameInput : public QObject {
    Q_OBJECT
public:
    GameInput(SettingsStore* s, EmuConsole* console, QObject* parent = nullptr);
    void setHostWindow(QWidget* top) { m_top = top; }
    void setScreen(QSize s);
    void setProfile(const Profile& p) { m_profile = p; releaseAll(); }
    const Profile& profile() const { return m_profile; }
    void reloadSettings();
    bool isArmed() const { return m_armed; }
    bool isCaptured() const { return m_captured; }
    void setArmed(bool on);
    void setCaptured(bool on);
    LatencyStats& latency() { return m_lat; }
    // тест точности: сумма сырых отсчётов и итогового смещения пальца
    QPointF testRaw() const { return m_testRaw; }
    QPointF testOut() const { return m_testOut; }
    void resetTest() { m_testRaw = m_testOut = {}; }
    void tapCenterForCalibration();
signals:
    void armedChanged(bool on);
    void capturedChanged(bool on);
    void latencyUpdated(double avg, double p95, int n);
private:
    void onMouse(int dx, int dy, qint64 ts);
    void onButton(int b, bool down, qint64 ts);
    void onKey(int vk, bool down, qint64 ts);
    void updateJoystick(qint64 ts);
    void tap(int slot, QPointF px);
    void releaseAll();
    void applyRawMode();
    QPointF P(QPointF ref, Anchor a) const { return MouseMath::mapPoint(ref, a, m_screen); }
    int allocSlot();
    SettingsStore* m_s; EmuConsole* m_console; RawInput* m_raw; TouchInjector m_touch; QWidget* m_top = nullptr;
    SensConfig m_cfg; Smoother m_smooth; CameraFinger m_cam; Profile m_profile = Profile::builtinShooter();
    QSize m_screen{1920, 1080}; bool m_armed = false, m_captured = false, m_ads = false; int m_captureVk = 0xC0;
    QSet<int> m_moveKeys; QHash<int, int> m_keySlot; LatencyStats m_lat; int m_latTick = 0;
    QPointF m_testRaw, m_testOut;
};
