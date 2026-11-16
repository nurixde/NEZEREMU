#include "WinUtil.h"
#include <QCoreApplication>
#include <QDir>
#include <QPair>
#include <QSettings>
#ifdef Q_OS_WIN
#include <windows.h>
#include <tlhelp32.h>
#endif

namespace WinUtil {
QList<quint32> processTree(quint32 root) {
    QList<quint32> out{root};
#ifdef Q_OS_WIN
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return out;
    QList<QPair<quint32, quint32>> all;
    PROCESSENTRY32W pe{}; pe.dwSize = sizeof(pe);
    if (Process32FirstW(snap, &pe)) {
        do { all.append({quint32(pe.th32ProcessID), quint32(pe.th32ParentProcessID)}); } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    for (int i = 0; i < out.size(); ++i)
        for (const auto& p : all)
            if (p.second == out[i] && !out.contains(p.first)) out << p.first;
#endif
    return out;
}

void setPriority(quint32 pid, int level) {
#ifdef Q_OS_WIN
    HANDLE h = OpenProcess(PROCESS_SET_INFORMATION, FALSE, pid);
    if (!h) return;
    const DWORD cls = level >= 2 ? HIGH_PRIORITY_CLASS : level == 1 ? ABOVE_NORMAL_PRIORITY_CLASS : NORMAL_PRIORITY_CLASS;
    SetPriorityClass(h, cls);
    CloseHandle(h);
#else
    Q_UNUSED(pid) Q_UNUSED(level)
#endif
}

void captureCursor(quintptr hwnd) {
#ifdef Q_OS_WIN
    RECT r;
    if (GetWindowRect(reinterpret_cast<HWND>(hwnd), &r)) {
        ClipCursor(&r);
        SetCursorPos((r.left + r.right) / 2, (r.top + r.bottom) / 2);
    }
    while (ShowCursor(FALSE) >= 0) {}
#else
    Q_UNUSED(hwnd)
#endif
}

void releaseCursor() {
#ifdef Q_OS_WIN
    ClipCursor(nullptr);
    while (ShowCursor(TRUE) < 0) {}
#endif
}

void setAutostart(bool on) {
    QSettings run("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    if (on) run.setValue("NEZER EMU", "\"" + QDir::toNativeSeparators(QCoreApplication::applicationFilePath()) + "\"");
    else run.remove("NEZER EMU");
}
}
