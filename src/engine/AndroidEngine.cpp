#include "AndroidEngine.h"
#include "AdbClient.h"
#include "BlueStacks.h"
#include "EmuConsole.h"
#include "ProcUtil.h"
#include "core/WinUtil.h"
#include "display/FpsMonitor.h"
#include "settings/SettingsStore.h"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QProcess>
#include <QThread>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

AndroidEngine::AndroidEngine(SettingsStore* s, QObject* parent) : QObject(parent), m_s(s) {
    m_adb = new AdbClient(this);
    m_console = new EmuConsole(this);   // с BlueStacks не подключается; оставлен для совместимости модулей ввода
    m_fps = new FpsMonitor(m_adb, this);
    m_scan.setInterval(500);
    connect(&m_scan, &QTimer::timeout, this, &AndroidEngine::scanWindow);
    m_boot.setInterval(2000);
    connect(&m_boot, &QTimer::timeout, this, &AndroidEngine::pollBoot);
    m_watch.setInterval(3000);
    connect(&m_watch, &QTimer::timeout, this, &AndroidEngine::watch);
    refreshInstalledState();
}

// Без сигналов: главное окно уже разрушается. BlueStacks закрывается в MainWindow::closeEvent.
AndroidEngine::~AndroidEngine() { m_scan.stop(); m_boot.stop(); m_watch.stop(); }

void AndroidEngine::refreshInstalledState() {
    if (!BlueStacks::isInstalled()) setState(State::NotInstalled, "BlueStacks 5 не найден. Установите его с bluestacks.com");
    else if (m_state == State::NotInstalled) setState(State::Stopped, "Android остановлен");
}

void AndroidEngine::modeDefaults(const QString& mode, int* cores, int* ram, int* prio) {
    const int n = qMax(2, QThread::idealThreadCount());
    if (mode == "balance") { *cores = qBound(2, n / 2, 4); *ram = 4096; *prio = 0; }
    else if (mode == "max") { *cores = qBound(2, n - 1, 16); *ram = 8192; *prio = 2; }
    else { *cores = qBound(2, n - 2, 8); *ram = 6144; *prio = 1; }
}

AvdParams AndroidEngine::params() const {
    AvdParams p;
    p.cores = m_s->value("perf.cores").toInt();
    p.ramMb = m_s->value("perf.ramMb").toInt();
    p.width = m_s->value("screen.width").toInt();
    p.height = m_s->value("screen.height").toInt();
    p.dpi = m_s->value("screen.dpi").toInt();
    const int fps = m_s->value("perf.fps").toInt();
    p.refreshHz = fps <= 0 ? 240 : fps;
    p.gpuMode = m_s->value("gfx.bsRenderer").toString();
    p.landscape = m_s->value("screen.orientation").toString() != "portrait";
    return p;
}

QSize AndroidEngine::screenSize() const {
    const AvdParams p = params();
    return p.landscape ? QSize(qMax(p.width, p.height), qMin(p.width, p.height)) : QSize(qMin(p.width, p.height), qMax(p.width, p.height));
}

QString AndroidEngine::instance() const {
    const QStringList all = BlueStacks::instances();
    const QString want = m_s->value("bs.instance").toString();
    if (!want.isEmpty() && (all.isEmpty() || all.contains(want))) return want;
    return BlueStacks::defaultInstance();
}

// Ключи bluestacks.conf, которые NEZER EMU выставляет перед запуском (только когда BlueStacks закрыт).
QMap<QString, QString> AndroidEngine::confChanges() const {
    const AvdParams p = params();
    const QSize sz = screenSize();
    const QString k = "bst.instance." + m_inst + ".";
    QMap<QString, QString> m;
    m["bst.enable_adb_access"] = "1";
    m[k + "cpus"] = QString::number(p.cores);
    m[k + "ram"] = QString::number(p.ramMb);
    m[k + "fb_width"] = QString::number(sz.width());
    m[k + "fb_height"] = QString::number(sz.height());
    m[k + "dpi"] = QString::number(p.dpi);
    m[k + "fps"] = QString::number(p.refreshHz);
    m[k + "enable_high_fps"] = p.refreshHz > 60 ? "1" : "0";
    if (!p.gpuMode.isEmpty()) m[k + "graphics_renderer"] = p.gpuMode;
    return m;
}

void AndroidEngine::setState(State s, const QString& m) { m_state = s; m_msg = m; emit stateChanged(s, m); }

void AndroidEngine::start() {
    refreshInstalledState();
    if (m_state == State::NotInstalled || m_state == State::Starting || m_state == State::Booting || m_state == State::Ready) return;
    const BlueStacks::Install bs = BlueStacks::detect();
    m_inst = instance();
    if (m_s->value("bs.applySettings").toBool()) {
        if (BlueStacks::playerPids().isEmpty()) {
            QString err;
            if (BlueStacks::writeConf(confChanges(), &err)) emit logLine("Настройки записаны в " + bs.conf);
            else emit logLine("Настройки не записаны в BlueStacks: " + err);
        } else emit logLine("BlueStacks уже запущен: CPU, RAM и разрешение применятся после его перезапуска.");
    }
    setState(State::Starting, "Запуск BlueStacks (" + m_inst + ")…");
    emit logLine("Запуск: " + bs.player + " --instance " + m_inst);
    if (!QProcess::startDetached(bs.player, {"--instance", m_inst})) {
        setState(State::Error, "Не удалось запустить BlueStacks (HD-Player.exe).");
        return;
    }
    m_hwnd = 0; m_pid = 0; m_scanTries = 0; m_bootTries = 0; m_polling = false;
    m_scan.start(); m_boot.start(); m_watch.start();
}

#ifdef Q_OS_WIN
struct BsScan { QList<quint32> pids; HWND best = nullptr; qint64 area = 0; };
static BOOL CALLBACK bsEnum(HWND h, LPARAM lp) {
    auto* c = reinterpret_cast<BsScan*>(lp);
    if (!IsWindowVisible(h) || GetWindow(h, GW_OWNER)) return TRUE;
    DWORD pid = 0; GetWindowThreadProcessId(h, &pid);
    if (!c->pids.contains(quint32(pid))) return TRUE;
    RECT r{}; GetWindowRect(h, &r);
    const int w = r.right - r.left, hh = r.bottom - r.top;
    if (w < 300 || hh < 200) return TRUE;   // пропускаем маленькие служебные окна
    const qint64 a = qint64(w) * hh;
    if (a > c->area) { c->area = a; c->best = h; }
    return TRUE;
}
#endif

void AndroidEngine::scanWindow() {
    if (++m_scanTries > 240) { m_scan.stop(); m_boot.stop(); setState(State::Error, "Окно BlueStacks не появилось за 2 минуты."); return; }
#ifdef Q_OS_WIN
    BsScan c;
    c.pids = BlueStacks::playerPids();
    if (c.pids.isEmpty()) return;
    EnumWindows(bsEnum, reinterpret_cast<LPARAM>(&c));
    if (!c.best) return;
    m_scan.stop();
    m_hwnd = reinterpret_cast<quintptr>(c.best);
    DWORD pid = 0; GetWindowThreadProcessId(c.best, &pid);
    m_pid = quint32(pid);
    if (m_state == State::Starting) setState(State::Booting, "Загрузка Android в BlueStacks…");
    emit windowReady(m_hwnd);
#endif
}

void AndroidEngine::pollBoot() {
    if (m_polling) return;
    if (++m_bootTries > 150) {
        m_boot.stop();
        setState(State::Error, "BlueStacks не отвечает по ADB. В BlueStacks откройте Настройки → Дополнительно и включите «Android Debug Bridge (ADB)».");
        return;
    }
    const QString serial = "127.0.0.1:" + QString::number(BlueStacks::adbPort(m_inst));   // порт BlueStacks пишет при старте
    m_adb->setSerial(serial);
    m_polling = true;
    ProcUtil::run(this, BlueStacks::adbExe(), {"connect", serial}, 8000, [this](int, QString) {
        m_adb->bootCompleted([this](bool done) {
            m_polling = false;
            if (!done || m_state == State::Ready || m_state == State::Stopped || m_state == State::Error) return;
            m_boot.stop();
            const int prio = m_s->value("perf.highPriority").toBool() ? (m_s->value("perf.mode").toString() == "max" ? 2 : 1) : 0;
            if (m_pid) for (quint32 pid : WinUtil::processTree(m_pid)) WinUtil::setPriority(pid, prio);
            m_adb->deviceAbi([this](QString abi) { if (!abi.isEmpty()) m_abi = abi; });
            applyFrameRate();
            if (m_s->value("perf.showFps").toBool()) m_fps->start();
            setState(State::Ready, "Android готов (BlueStacks " + m_inst + ")");
            refreshApps();
        });
    });
}

// BlueStacks закрыли самим (или он упал): синхронизируем состояние.
void AndroidEngine::watch() {
    if (m_state == State::Stopped || m_state == State::NotInstalled || m_state == State::Error) { m_watch.stop(); return; }
    if (m_state == State::Starting && m_scanTries < 20) return;
    const QList<quint32> pids = BlueStacks::playerPids();
    if (pids.isEmpty() || (m_pid && !pids.contains(m_pid))) {
        m_scan.stop(); m_boot.stop(); m_fps->stop(); m_watch.stop();
        m_hwnd = 0; m_pid = 0;
        setState(State::Stopped, "BlueStacks закрыт");
    }
}

void AndroidEngine::stop() {
    m_scan.stop(); m_boot.stop(); m_watch.stop(); m_fps->stop();
#ifdef Q_OS_WIN
    if (m_pid && BlueStacks::playerPids().contains(m_pid)) {
        if (m_hwnd && IsWindow(reinterpret_cast<HWND>(m_hwnd))) PostMessageW(reinterpret_cast<HWND>(m_hwnd), WM_CLOSE, 0, 0);
        QElapsedTimer t; t.start();
        while (BlueStacks::playerPids().contains(m_pid) && t.elapsed() < 12000) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
            QThread::msleep(150);
        }
        if (BlueStacks::playerPids().contains(m_pid)) {
            HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, DWORD(m_pid));
            if (h) { TerminateProcess(h, 0); CloseHandle(h); }
        }
    }
#endif
    m_hwnd = 0; m_pid = 0;
    if (m_state != State::NotInstalled && m_state != State::Stopped) setState(State::Stopped, "Android остановлен");
}

void AndroidEngine::restart() { stop(); QTimer::singleShot(1500, this, &AndroidEngine::start); }

void AndroidEngine::refreshApps() {
    if (m_state != State::Ready) return;
    m_adb->launchablePackages([this](QStringList pk) { pk.sort(); m_apps = pk; emit appsChanged(pk); });
}

void AndroidEngine::applyLiveDisplay() {
    if (m_state != State::Ready) return;
    const QSize s = screenSize();
    m_adb->setDisplay(s.width(), s.height(), m_s->value("screen.dpi").toInt());
}

void AndroidEngine::applyFrameRate() {
    if (m_state != State::Ready && m_state != State::Booting) return;
    const int fps = m_s->value("perf.fps").toInt();
    m_adb->setRefreshRate(fps <= 0 ? 240 : fps);
}
