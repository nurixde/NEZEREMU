#include "AndroidEngine.h"
#include "AdbClient.h"
#include "EmuConsole.h"
#include "EmulatorProcess.h"
#include "SdkInstaller.h"
#include "core/WinUtil.h"
#include "display/FpsMonitor.h"
#include "settings/SettingsStore.h"
#include <QThread>

AndroidEngine::AndroidEngine(SettingsStore* s, QObject* parent) : QObject(parent), m_s(s) {
    m_adb = new AdbClient(this);
    m_console = new EmuConsole(this);
    m_proc = new EmulatorProcess(this);
    m_fps = new FpsMonitor(m_adb, this);
    connect(m_proc, &EmulatorProcess::logLine, this, &AndroidEngine::logLine);
    connect(m_proc, &EmulatorProcess::windowFound, this, [this](quintptr h) {
        if (m_state == State::Starting) setState(State::Booting, "Загрузка Android…");
        emit windowReady(h);
    });
    connect(m_proc, &EmulatorProcess::failed, this, [this](const QString& r) { m_boot.stop(); setState(State::Error, r); });
    connect(m_proc, &EmulatorProcess::exited, this, [this] {
        m_boot.stop(); m_fps->stop();
        if (m_state != State::Error) setState(State::Stopped, "Android остановлен");
    });
    m_boot.setInterval(2000);
    connect(&m_boot, &QTimer::timeout, this, &AndroidEngine::pollBoot);
    refreshInstalledState();
}

AndroidEngine::~AndroidEngine() { m_proc->stop(); }

void AndroidEngine::refreshInstalledState() {
    if (!SdkInstaller::isInstalled()) setState(State::NotInstalled, "Android не установлен");
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
    p.gpuMode = m_s->value("gfx.mode").toString();
    p.landscape = m_s->value("screen.orientation").toString() != "portrait";
    return p;
}

QSize AndroidEngine::screenSize() const {
    const AvdParams p = params();
    return p.landscape ? QSize(qMax(p.width, p.height), qMin(p.width, p.height)) : QSize(qMin(p.width, p.height), qMax(p.width, p.height));
}

void AndroidEngine::setState(State s, const QString& m) { m_state = s; m_msg = m; emit stateChanged(s, m); }

void AndroidEngine::start() {
    refreshInstalledState();
    if (m_state == State::NotInstalled || m_state == State::Starting || m_state == State::Booting || m_state == State::Ready) return;
    const bool wipe = m_s->value("android.wipeNext").toBool();
    if (wipe) { m_s->setValue("android.wipeNext", false); m_s->save(); }
    setState(State::Starting, "Запуск Android Emulator…");
    m_proc->start(params(), m_s->value("android.coldBoot").toBool(), wipe);
    m_bootTries = 0;
    m_boot.start();
}

void AndroidEngine::stop() { m_boot.stop(); m_fps->stop(); m_proc->stop(); }
void AndroidEngine::restart() { stop(); QTimer::singleShot(1500, this, &AndroidEngine::start); }

void AndroidEngine::pollBoot() {
    if (++m_bootTries > 150) { m_boot.stop(); setState(State::Error, "Android не загрузился за 5 минут. Попробуйте «Холодный запуск» в «Системе Android»."); return; }
    m_adb->bootCompleted([this](bool done) {
        if (!done || m_state == State::Ready) return;
        m_boot.stop();
        const int prio = m_s->value("perf.highPriority").toBool() ? (m_s->value("perf.mode").toString() == "max" ? 2 : 1) : 0;
        for (quint32 pid : WinUtil::processTree(m_proc->pid())) WinUtil::setPriority(pid, prio);
        m_console->connectTo(5554);
        m_adb->deviceAbi([this](QString abi) { if (!abi.isEmpty()) m_abi = abi; });
        applyFrameRate();
        if (m_s->value("perf.showFps").toBool()) m_fps->start();
        setState(State::Ready, "Android готов");
        refreshApps();
    });
}

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
