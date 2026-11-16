#include "RawInput.h"
#include "engine/EmuConsole.h"
#include <QCoreApplication>
#include <QVector>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

RawInput::RawInput(QObject* parent) : QObject(parent) { QCoreApplication::instance()->installNativeEventFilter(this); }
RawInput::~RawInput() { setMode(Mode::Off, 0); QCoreApplication::instance()->removeNativeEventFilter(this); }

void RawInput::setMode(Mode m, quintptr hwnd) {
#ifdef Q_OS_WIN
    RAWINPUTDEVICE d[2]{};
    d[0].usUsagePage = 0x01; d[0].usUsage = 0x02;   // мышь
    d[1].usUsagePage = 0x01; d[1].usUsage = 0x06;   // клавиатура
    if (m_mode != Mode::Off) {
        d[0].dwFlags = RIDEV_REMOVE; d[1].dwFlags = RIDEV_REMOVE; d[0].hwndTarget = d[1].hwndTarget = nullptr;
        RegisterRawInputDevices(d, 2, sizeof(RAWINPUTDEVICE));
    }
    HWND h = reinterpret_cast<HWND>(hwnd);
    if (m == Mode::KeyboardSink) {
        d[1].dwFlags = RIDEV_INPUTSINK; d[1].hwndTarget = h;
        RegisterRawInputDevices(&d[1], 1, sizeof(RAWINPUTDEVICE));
    } else if (m == Mode::Exclusive) {
        d[0].dwFlags = RIDEV_NOLEGACY | RIDEV_CAPTUREMOUSE; d[0].hwndTarget = h;
        d[1].dwFlags = RIDEV_NOLEGACY; d[1].hwndTarget = h;
        RegisterRawInputDevices(d, 2, sizeof(RAWINPUTDEVICE));
    }
#else
    Q_UNUSED(hwnd)
#endif
    m_mode = m; m_hwnd = hwnd;
}

bool RawInput::nativeEventFilter(const QByteArray& type, void* message, qintptr* result) {
    Q_UNUSED(result)
#ifdef Q_OS_WIN
    if (m_mode == Mode::Off || type != "windows_generic_MSG") return false;
    MSG* msg = static_cast<MSG*>(message);
    if (msg->message != WM_INPUT) return false;
    const qint64 ts = EmuConsole::clock().nsecsElapsed();
    UINT size = 0;
    GetRawInputData(reinterpret_cast<HRAWINPUT>(msg->lParam), RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));
    static QVector<BYTE> buf; if (buf.size() < int(size)) buf.resize(int(size));
    if (GetRawInputData(reinterpret_cast<HRAWINPUT>(msg->lParam), RID_INPUT, buf.data(), &size, sizeof(RAWINPUTHEADER)) != size) return false;
    const RAWINPUT* ri = reinterpret_cast<const RAWINPUT*>(buf.data());
    if (ri->header.dwType == RIM_TYPEMOUSE && m_mode == Mode::Exclusive) {
        const RAWMOUSE& m = ri->data.mouse;
        if (!(m.usFlags & MOUSE_MOVE_ABSOLUTE) && (m.lLastX || m.lLastY)) emit mouseMoved(int(m.lLastX), int(m.lLastY), ts);
        const USHORT f = m.usButtonFlags;
        const USHORT downs[5] = {RI_MOUSE_LEFT_BUTTON_DOWN, RI_MOUSE_RIGHT_BUTTON_DOWN, RI_MOUSE_MIDDLE_BUTTON_DOWN, RI_MOUSE_BUTTON_4_DOWN, RI_MOUSE_BUTTON_5_DOWN};
        const USHORT ups[5] = {RI_MOUSE_LEFT_BUTTON_UP, RI_MOUSE_RIGHT_BUTTON_UP, RI_MOUSE_MIDDLE_BUTTON_UP, RI_MOUSE_BUTTON_4_UP, RI_MOUSE_BUTTON_5_UP};
        for (int i = 0; i < 5; ++i) { if (f & downs[i]) emit mouseButton(i, true, ts); if (f & ups[i]) emit mouseButton(i, false, ts); }
    } else if (ri->header.dwType == RIM_TYPEKEYBOARD) {
        const RAWKEYBOARD& k = ri->data.keyboard;
        if (k.VKey != 0xFF) emit key(int(k.VKey), !(k.Flags & RI_KEY_BREAK), ts);
    }
    return false;
#else
    Q_UNUSED(type) Q_UNUSED(message)
    return false;
#endif
}
