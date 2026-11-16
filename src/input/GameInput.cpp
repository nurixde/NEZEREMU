#include "GameInput.h"
#include "RawInput.h"
#include "core/WinUtil.h"
#include "engine/EmuConsole.h"
#include "settings/SettingsStore.h"
#include <QTimer>
#include <QWidget>
#include <cmath>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

enum { SlotCam = 0, SlotJoy = 1, SlotFire = 2, SlotScope = 3, SlotKeysFirst = 4 };

GameInput::GameInput(SettingsStore* s, EmuConsole* c, QObject* parent)
    : QObject(parent), m_s(s), m_console(c), m_raw(new RawInput(this)), m_touch(c) {
    connect(m_raw, &RawInput::mouseMoved, this, &GameInput::onMouse);
    connect(m_raw, &RawInput::mouseButton, this, &GameInput::onButton);
    connect(m_raw, &RawInput::key, this, &GameInput::onKey);
    connect(c, &EmuConsole::ackLatency, this, [this](double ms) {
        m_lat.add(ms);
        if (++m_latTick % 20 == 0) emit latencyUpdated(m_lat.avg(), m_lat.percentile(95), m_lat.count());
    });
    reloadSettings();
}

void GameInput::setScreen(QSize s) { m_screen = s; m_touch.setScreen(s); releaseAll(); }

void GameInput::reloadSettings() {
    m_cfg.sensX = m_s->value("input.sensX").toDouble();
    m_cfg.sensY = m_s->value("input.sensY").toDouble();
    m_cfg.adsMul = m_s->value("input.adsMul").toDouble();
    m_cfg.dpi = m_s->value("input.dpi").toInt();
    m_cfg.smoothing = m_s->value("input.smoothing").toBool();
    m_cfg.smoothAlpha = 1.0 - m_s->value("input.smoothStrength").toDouble() / 100.0;
    m_touch.setAbsMax(m_s->value("input.absMax", 0).toInt());
    m_captureVk = Profiles::keyFromName(m_s->value("input.captureKey").toString());
    if (!m_captureVk) m_captureVk = 0xC0;
    m_smooth.reset();
}

void GameInput::applyRawMode() {
    const quintptr h = m_top ? quintptr(m_top->winId()) : 0;
    m_raw->setMode(!m_armed ? RawInput::Mode::Off : m_captured ? RawInput::Mode::Exclusive : RawInput::Mode::KeyboardSink, h);
}

void GameInput::setArmed(bool on) {
    if (m_armed == on) return;
    m_armed = on;
    if (!on) setCaptured(false);
    applyRawMode();
    emit armedChanged(on);
}

void GameInput::setCaptured(bool on) {
    if (on && !m_armed) return;
    if (m_captured == on) return;
    m_captured = on;
    if (!on) releaseAll();
    if (on && m_top) WinUtil::captureCursor(quintptr(m_top->winId())); else WinUtil::releaseCursor();
    m_cam.configure(P(m_profile.camAnchor, m_profile.camAnchor_a), MouseMath::mapLength(m_profile.camRadius, m_screen));
    m_smooth.reset();
    applyRawMode();
    emit capturedChanged(on);
}

void GameInput::onMouse(int dx, int dy, qint64 ts) {
    if (!m_captured) return;
    QPointF d = MouseMath::countsToPixels(dx, dy, m_cfg, m_ads, m_screen.height());
    d = m_smooth.step(d, m_cfg);
    m_testRaw += QPointF(dx, dy); m_testOut += d;
    for (const TouchOp& op : m_cam.move(d)) {
        if (op.kind == TouchOp::Down) m_touch.down(SlotCam, op.pos, ts);
        else if (op.kind == TouchOp::Up) m_touch.up(SlotCam, ts);
        else m_touch.move(SlotCam, op.pos, ts);
    }
}

void GameInput::tap(int slot, QPointF px) {
    m_touch.down(slot, px);
    QTimer::singleShot(35, this, [this, slot] { m_touch.up(slot); });
}

void GameInput::onButton(int b, bool down, qint64 ts) {
    if (!m_captured) return;
    if (b == 0) { if (down) m_touch.down(SlotFire, P(m_profile.fire, m_profile.fireAnchor), ts); else m_touch.up(SlotFire, ts); return; }
    if (b == 1) {
        const QPointF sc = P(m_profile.scope, m_profile.scopeAnchor);
        if (m_profile.adsToggle) { if (down) { tap(SlotScope, sc); m_ads = !m_ads; } }
        else { tap(SlotScope, sc); m_ads = down; }
        return;
    }
    if (b == 3 || b == 4) onKey(b == 3 ? 0x05 : 0x06, down, ts);
}

int GameInput::allocSlot() {
    for (int s = SlotKeysFirst; s < TouchInjector::kSlots; ++s) if (!m_touch.isDown(s) && !m_keySlot.values().contains(s)) return s;
    return -1;
}

void GameInput::onKey(int vk, bool down, qint64 ts) {
#ifdef Q_OS_WIN
    if (m_top && GetForegroundWindow() != reinterpret_cast<HWND>(m_top->winId())) return;
#endif
    if (vk == m_captureVk) { if (down) setCaptured(!m_captured); return; }
    if (!m_captured) return;
    if (vk == 0x1B) { if (down) setCaptured(false); return; }   // Esc освобождает курсор
    if (vk == 'W' || vk == 'A' || vk == 'S' || vk == 'D') {
        if (down) m_moveKeys.insert(vk); else m_moveKeys.remove(vk);
        updateJoystick(ts);
        return;
    }
    for (const KeyBind& k : m_profile.keys) {
        if (Profiles::keyFromName(k.key) != vk) continue;
        if (down && !m_keySlot.contains(vk)) {
            const int s = allocSlot(); if (s < 0) return;
            m_keySlot.insert(vk, s);
            m_touch.down(s, P(k.pos, k.anchor), ts);
        } else if (!down && m_keySlot.contains(vk)) {
            m_touch.up(m_keySlot.take(vk), ts);
        }
        return;
    }
}

void GameInput::updateJoystick(qint64 ts) {
    QPointF dir(0, 0);
    if (m_moveKeys.contains('W')) dir.ry() -= 1;
    if (m_moveKeys.contains('S')) dir.ry() += 1;
    if (m_moveKeys.contains('A')) dir.rx() -= 1;
    if (m_moveKeys.contains('D')) dir.rx() += 1;
    const QPointF c = P(m_profile.joyCenter, m_profile.joyAnchor);
    if (dir.isNull()) { m_touch.up(SlotJoy, ts); return; }
    const double len = std::hypot(dir.x(), dir.y());
    const QPointF target = c + dir / len * MouseMath::mapLength(m_profile.joyRadius, m_screen);
    if (!m_touch.isDown(SlotJoy)) m_touch.down(SlotJoy, c, ts);
    m_touch.move(SlotJoy, target, ts);
}

void GameInput::releaseAll() {
    m_cam.release();
    m_touch.releaseAll();
    m_moveKeys.clear(); m_keySlot.clear(); m_ads = false;
}

void GameInput::tapCenterForCalibration() { m_touch.setScreen(m_screen); tap(SlotKeysFirst, QPointF(m_screen.width() / 2.0, m_screen.height() / 2.0)); }
