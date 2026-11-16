#include "EmulatorProcess.h"
#include "core/Paths.h"
#include "core/WinUtil.h"
#ifdef Q_OS_WIN
#include <windows.h>
#endif

EmulatorProcess::EmulatorProcess(QObject* parent) : QObject(parent) {
    m_proc.setProcessChannelMode(QProcess::MergedChannels);
    connect(&m_proc, &QProcess::readyRead, this, [this] {
        const QString chunk = QString::fromLocal8Bit(m_proc.readAll());
        m_log += chunk; if (m_log.size() > 200000) m_log = m_log.right(100000);
        for (const QString& l : chunk.split('\n', Qt::SkipEmptyParts)) emit logLine(l.trimmed());
    });
    connect(&m_proc, &QProcess::finished, this, [this](int code, QProcess::ExitStatus) {
        m_scan.stop();
        if (code != 0) emit failed(diagnose(m_log));
        emit exited();
    });
    connect(&m_proc, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) {
        if (e == QProcess::FailedToStart) emit failed("Не найден Android Emulator. Запустите первоначальную настройку в «Система Android».");
    });
    m_scan.setInterval(500);
    connect(&m_scan, &QTimer::timeout, this, &EmulatorProcess::scanWindows);
}

EmulatorProcess::~EmulatorProcess() { stop(); }

void EmulatorProcess::start(const AvdParams& p, bool coldBoot, bool wipe) {
    if (isRunning()) return;
    QString err;
    AvdConfig::write(p, &err);
    QStringList a{"-avd", AvdConfig::kAvdName, "-port", "5554", "-gpu", p.gpuMode,
                  "-cores", QString::number(p.cores), "-memory", QString::number(p.ramMb), "-no-boot-anim"};
    if (coldBoot) a << "-no-snapshot-load";
    if (wipe) a << "-wipe-data";
    m_log.clear();
    m_proc.setProcessEnvironment(Paths::sdkEnv());
    m_proc.start(Paths::emulatorExe(), a);
    m_scanTries = 0;
    m_scan.start();
}

void EmulatorProcess::stop() {
    m_scan.stop();
    if (m_proc.state() == QProcess::NotRunning) { m_hwnd = 0; return; }
#ifdef Q_OS_WIN
    if (m_hwnd) PostMessageW(reinterpret_cast<HWND>(m_hwnd), WM_CLOSE, 0, 0); else m_proc.terminate();
#else
    m_proc.terminate();
#endif
    if (!m_proc.waitForFinished(20000)) m_proc.kill();
    m_hwnd = 0;
}

QString EmulatorProcess::diagnose(const QString& log) {
    const QString l = log.toLower();
    if (l.contains("whpx") && (l.contains("not") || l.contains("fail")))
        return "Не работает аппаратная виртуализация (WHPX). Включите «Платформа гипервизора Windows» в компонентах Windows и VT-x/AMD-V в BIOS.";
    if (l.contains("requires hardware acceleration") || l.contains("haxm") || l.contains("aehd"))
        return "Нет аппаратного ускорения. Включите виртуализацию в BIOS и «Платформу гипервизора Windows».";
    if (l.contains("unknown avd") || l.contains("no avd")) return "Виртуальное устройство не найдено. Повторите настройку Android.";
    if (l.contains("not enough space") || l.contains("disk space")) return "Недостаточно места на диске.";
    if (l.contains("vulkan") || l.contains("opengl") || l.contains("gles")) return "Ошибка графики. Обновите драйвер видеокарты или выберите другой рендеринг в «Графике».";
    if (l.contains("already running") || l.contains("in use")) return "Эмулятор уже запущен или порт 5554 занят.";
    return "Android Emulator завершился с ошибкой. Подробности в журнале (Дополнительно → Журнал).";
}

#ifdef Q_OS_WIN
struct ScanCtx { QList<quint32> pids; HWND found = nullptr; };
static BOOL CALLBACK enumProc(HWND h, LPARAM lp) {
    auto* c = reinterpret_cast<ScanCtx*>(lp);
    DWORD pid = 0; GetWindowThreadProcessId(h, &pid);
    if (!c->pids.contains(quint32(pid)) || !IsWindowVisible(h)) return TRUE;
    wchar_t title[256]{}; GetWindowTextW(h, title, 255);
    const QString t = QString::fromWCharArray(title);
    if (t.startsWith("Android Emulator")) { c->found = h; return FALSE; }
    return TRUE;
}
#endif

void EmulatorProcess::scanWindows() {
    if (++m_scanTries > 240) { m_scan.stop(); emit failed("Окно Android Emulator не появилось за 2 минуты."); return; }
#ifdef Q_OS_WIN
    ScanCtx ctx; ctx.pids = WinUtil::processTree(pid());
    EnumWindows(enumProc, reinterpret_cast<LPARAM>(&ctx));
    if (ctx.found) { m_scan.stop(); m_hwnd = reinterpret_cast<quintptr>(ctx.found); emit windowFound(reinterpret_cast<quintptr>(ctx.found)); }
#endif
}
