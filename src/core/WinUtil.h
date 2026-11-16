#pragma once
#include <QList>
#include <QString>

namespace WinUtil {
QList<quint32> processTree(quint32 root);    // процесс и все его потомки
void setPriority(quint32 pid, int level);     // 0 обычный, 1 выше обычного, 2 высокий
void captureCursor(quintptr hwnd);            // ограничить и скрыть курсор внутри окна
void releaseCursor();
void setAutostart(bool on);
}
