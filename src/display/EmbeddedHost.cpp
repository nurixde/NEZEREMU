#include "EmbeddedHost.h"
#ifdef Q_OS_WIN
#include <windows.h>
#endif

EmbeddedHost::EmbeddedHost(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_DontCreateNativeAncestors);
    setStyleSheet("background:#000;");
}

void EmbeddedHost::attach(quintptr hwnd) {
#ifdef Q_OS_WIN
    HWND h = reinterpret_cast<HWND>(hwnd);
    LONG_PTR st = GetWindowLongPtrW(h, GWL_STYLE);
    st &= ~(WS_POPUP | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
    st |= WS_CHILD | WS_VISIBLE;
    SetWindowLongPtrW(h, GWL_STYLE, st);
    LONG_PTR ex = GetWindowLongPtrW(h, GWL_EXSTYLE);
    ex &= ~(WS_EX_APPWINDOW | WS_EX_WINDOWEDGE | WS_EX_DLGMODALFRAME);
    SetWindowLongPtrW(h, GWL_EXSTYLE, ex);
    SetParent(h, reinterpret_cast<HWND>(winId()));
    m_child = hwnd;
    fit();
#else
    Q_UNUSED(hwnd)
#endif
}

void EmbeddedHost::detach() { m_child = 0; }

void EmbeddedHost::setChildInputEnabled(bool on) {
#ifdef Q_OS_WIN
    if (m_child) EnableWindow(reinterpret_cast<HWND>(m_child), on ? TRUE : FALSE);
#else
    Q_UNUSED(on)
#endif
}

void EmbeddedHost::focusChild() {
#ifdef Q_OS_WIN
    if (m_child) SetFocus(reinterpret_cast<HWND>(m_child));
#endif
}

void EmbeddedHost::fit() {
#ifdef Q_OS_WIN
    if (!m_child) return;
    const qreal k = devicePixelRatioF();
    SetWindowPos(reinterpret_cast<HWND>(m_child), nullptr, 0, 0, int(width() * k), int(height() * k),
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
#endif
}

void EmbeddedHost::resizeEvent(QResizeEvent*) { fit(); }
void EmbeddedHost::showEvent(QShowEvent*) { fit(); }
