#include "SystemCheck.h"
#include "Paths.h"
#include <QOperatingSystemVersion>
#include <QStorageInfo>
#include <QSysInfo>
#include <QThread>
#ifdef Q_OS_WIN
#include <windows.h>
#include <intrin.h>
#endif

namespace SystemCheck {
quint64 totalRamMb() {
#ifdef Q_OS_WIN
    MEMORYSTATUSEX m{}; m.dwLength = sizeof(m);
    if (GlobalMemoryStatusEx(&m)) return m.ullTotalPhys / (1024ull * 1024ull);
#endif
    return 0;
}

bool virtualizationAvailable(QString* how) {
#ifdef Q_OS_WIN
    int r[4]{}; __cpuid(r, 1);
    if (r[2] & (1 << 31)) { if (how) *how = "Гипервизор Windows активен"; return true; }
    if (IsProcessorFeaturePresent(PF_VIRT_FIRMWARE_ENABLED)) { if (how) *how = "Включена в BIOS/UEFI"; return true; }
    if (how) *how = "Отключена: включите Intel VT-x / AMD SVM в BIOS";
#else
    if (how) *how = "Проверка доступна только в Windows";
#endif
    return false;
}

QList<CheckResult> run() {
    QList<CheckResult> l;
    const auto os = QOperatingSystemVersion::current();
    const bool win10 = os >= QOperatingSystemVersion::Windows10;
    l << CheckResult{"Windows 10/11 (64-bit)", win10 && QSysInfo::currentCpuArchitecture() == "x86_64", true,
                     QSysInfo::prettyProductName() + ", " + QSysInfo::currentCpuArchitecture()};
    const int cores = QThread::idealThreadCount();
    l << CheckResult{"Процессор: не менее 4 потоков", cores >= 4, false, QString::number(cores) + " логических процессоров"};
    const quint64 ram = totalRamMb();
    l << CheckResult{"Оперативная память: не менее 8 ГБ", ram >= 7600, ram < 3800, QString::number(ram / 1024.0, 'f', 1) + " ГБ"};
    QStorageInfo st(Paths::dataDir());
    const double freeGb = st.bytesAvailable() / 1073741824.0;
    l << CheckResult{"Свободно на диске: не менее 20 ГБ", freeGb >= 20, freeGb < 12, QString::number(freeGb, 'f', 1) + " ГБ на " + st.rootPath()};
    QString how; const bool v = virtualizationAvailable(&how);
    l << CheckResult{"Аппаратная виртуализация", v, true, how};
    return l;
}
}
