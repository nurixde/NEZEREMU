#include "MainWindow.h"
#include "AllAppsDialog.h"
#include "AndroidView.h"
#include "DesktopScreen.h"
#include "Dock.h"
#include "LockScreen.h"
#include "SettingsDialog.h"
#include "SetupWizard.h"
#include "SideToolbar.h"
#include "Theme.h"
#include "TitleBar.h"
#include "Widgets.h"
#include "apps/AppManager.h"
#include "core/Paths.h"
#include "display/EmbeddedHost.h"
#include "display/FpsMonitor.h"
#include "engine/AdbClient.h"
#include "engine/ApkInspector.h"
#include "engine/EmuConsole.h"
#include "input/GameInput.h"
#include "settings/SettingsStore.h"
#include <QApplication>
#include <QCloseEvent>
#include <QDateTime>
#include <QDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QCursor>
#include <QPlainTextEdit>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QShortcut>
#include <QStackedWidget>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#ifdef Q_OS_WIN
#include <windows.h>
#include <windowsx.h>
#endif

MainWindow::MainWindow(SettingsStore* s, QWidget* parent) : QWidget(parent), m_s(s) {
    setWindowTitle("NEZER EMU");
    setWindowFlag(Qt::FramelessWindowHint);
    setMinimumSize(960, 560);
    setAcceptDrops(true);
    setStyleSheet(QString("MainWindow{background:%1;}").arg(Theme::kBg));
    m_engine = new AndroidEngine(s, this);
    m_apps = new AppManager(s, this);
    m_game = new GameInput(s, m_engine->console(), this);
    m_game->setHostWindow(this);
    m_game->setProfile(Profiles::load(s->value("input.profile").toString()));

    m_title = new TitleBar(this);
    m_tool = new SideToolbar(this);
    m_content = new QWidget(this);
    m_stack = new QStackedWidget(m_content);
    m_desktop = new DesktopScreen; m_android = new AndroidView;
    m_stack->addWidget(m_desktop); m_stack->addWidget(m_android);
    m_lock = new LockScreen(m_content);   // поверх содержимого, при каждом запуске
    m_lock->raise(); m_lock->setFocus();
    auto* body = new QHBoxLayout; body->setContentsMargins(0, 0, 0, 0); body->setSpacing(0);
    body->addWidget(m_content, 1); body->addWidget(m_tool);
    auto* root = new QVBoxLayout(this); root->setContentsMargins(0, 0, 0, 0); root->setSpacing(0);
    root->addWidget(m_title); root->addLayout(body, 1);
    m_tool->hide();

    connect(m_title, &TitleBar::minimizeRequested, this, &QWidget::showMinimized);
    connect(m_title, &TitleBar::maximizeRequested, this, [this] { isMaximized() ? showNormal() : showMaximized(); });
    connect(m_title, &TitleBar::closeRequested, this, &QWidget::close);
    connect(m_lock, &LockScreen::unlockRequested, this, &MainWindow::unlock);
    connect(m_tool, &SideToolbar::action, this, &MainWindow::onToolbar);
    connect(m_desktop, &DesktopScreen::launchRequested, this, &MainWindow::launch);
    connect(m_desktop, &DesktopScreen::setupRequested, this, &MainWindow::openSetup);
    connect(m_desktop, &DesktopScreen::startRequested, m_engine, &AndroidEngine::start);
    connect(m_desktop, &DesktopScreen::logRequested, this, &MainWindow::showLog);
    Dock* dock = m_desktop->dock();
    connect(dock, &Dock::launchRequested, this, &MainWindow::launch);
    connect(dock, &Dock::removeRequested, m_apps, &AppManager::unpin);
    connect(dock, &Dock::addRequested, this, [this] {
        if (m_engine->state() != AndroidEngine::State::Ready) { QMessageBox::information(this, "NEZER EMU", "Добавлять ярлыки можно после запуска Android: в Dock попадают только реально установленные приложения."); return; }
        AllAppsDialog d(m_apps, true, this); connect(&d, &AllAppsDialog::installApkRequested, this, [this] { installApk(); }); d.exec();
    });
    connect(dock, &Dock::allAppsRequested, this, [this] {
        AllAppsDialog d(m_apps, false, this);
        connect(&d, &AllAppsDialog::launchRequested, this, &MainWindow::launch);
        connect(&d, &AllAppsDialog::installApkRequested, this, [this] { installApk(); });
        d.exec();
    });
    connect(m_apps, &AppManager::dockChanged, this, &MainWindow::refreshDock);

    connect(m_engine, &AndroidEngine::stateChanged, this, &MainWindow::onState);
    connect(m_engine, &AndroidEngine::windowReady, this, [this](quintptr h) { m_android->attach(h); m_android->host()->setChildInputEnabled(!m_game->isCaptured()); });
    connect(m_engine, &AndroidEngine::appsChanged, m_apps, &AppManager::setInstalled);
    connect(m_engine, &AndroidEngine::logLine, this, [this](const QString& l) { m_log << l; if (m_log.size() > 4000) m_log.removeFirst(); });
    connect(m_engine->fps(), &FpsMonitor::sample, this, [this](double fps, double frameMs, double jitter) {
        m_fpsText = fps < 0 ? "FPS: н/д (образ не отдаёт счётчик)" : QString("FPS %1 · кадр %2 мс · разброс %3 мс").arg(fps, 0, 'f', 1).arg(frameMs, 0, 'f', 1).arg(jitter, 0, 'f', 1);
        updateTitle();
    });
    connect(m_game, &GameInput::latencyUpdated, this, [this](double avg, double p95, int) { m_latText = QString("ввод %1 мс (p95 %2)").arg(avg, 0, 'f', 2).arg(p95, 0, 'f', 2); updateTitle(); });
    connect(m_game, &GameInput::capturedChanged, this, [this](bool on) {
        m_android->host()->setChildInputEnabled(!on);
        m_flash = on ? "Курсор захвачен (` или Esc — освободить)" : QString();
        updateTitle();
    });
    connect(m_game, &GameInput::armedChanged, this, [this](bool on) { m_tool->setChecked("keymap", on); if (on) showAndroid(); });

    auto* fs = new QShortcut(QKeySequence(Qt::Key_F11), this); connect(fs, &QShortcut::activated, this, [this] { onToolbar("fullscreen"); });
    auto* home = new QShortcut(QKeySequence("Ctrl+H"), this); connect(home, &QShortcut::activated, this, &MainWindow::showDesktop);

    if (s->value("general.rememberSize").toBool() && s->contains("window.geometry"))
        restoreGeometry(QByteArray::fromBase64(s->value("window.geometry").toByteArray()));
    else resize(1280, 760);
    applyAppearance();
    onState(m_engine->state(), m_engine->message());
    refreshDock();
    if (s->value("general.autoAndroid", true).toBool()) QTimer::singleShot(300, m_engine, &AndroidEngine::start);
}

void MainWindow::applyAppearance() {
    const QString accent = m_s->value("appearance.accent").toString();
    const QColor ac = Theme::accent(accent);
    ToggleSwitch::accent = ac;
    qApp->setStyleSheet(Theme::globalStyle(accent));
    auto wp = [this](const QString& key, const QString& def) { const QString p = m_s->value(key).toString(); return p.isEmpty() ? Paths::assetsDir() + def : p; };
    if (!m_desktop->setImage(wp("appearance.mainWallpaper", "/wallpaper_main.jpg"))) m_desktop->setImage(Paths::assetsDir() + "/wallpaper_main.jpg");
    if (!m_lock->setImage(wp("appearance.lockWallpaper", "/wallpaper_lock.jpg"))) m_lock->setImage(Paths::assetsDir() + "/wallpaper_lock.jpg");
    m_lock->setAccent(ac);
    m_desktop->dock()->setOpacityPercent(m_s->value("appearance.dockOpacity").toInt());
    m_desktop->dock()->setAccent(ac);
    if (m_s->value("perf.showFps").toBool() && m_engine->state() == AndroidEngine::State::Ready) m_engine->fps()->start();
    if (!m_s->value("perf.showFps").toBool()) { m_engine->fps()->stop(); m_fpsText.clear(); }
    updateTitle();
}

void MainWindow::resizeEvent(QResizeEvent*) {
    if (m_locked) m_lock->setGeometry(m_content->rect());
    m_stack->setGeometry(m_content->rect());
    QTimer::singleShot(0, this, [this] { m_stack->setGeometry(m_content->rect()); if (m_locked) m_lock->setGeometry(m_content->rect()); });
}

void MainWindow::unlock() {
    if (!m_locked) return;
    m_locked = false;
    auto* a = new QPropertyAnimation(m_lock, "geometry", this);
    a->setDuration(420); a->setEasingCurve(QEasingCurve::InOutCubic);
    a->setStartValue(m_lock->geometry()); a->setEndValue(m_lock->geometry().translated(0, -m_content->height()));
    connect(a, &QPropertyAnimation::finished, this, [this] {
        m_lock->hide(); m_tool->show();
        if (m_engine->state() == AndroidEngine::State::NotInstalled && !m_setupShown) openSetup();
    });
    a->start(QAbstractAnimation::DeleteWhenStopped);
    showDesktop();
}

void MainWindow::showDesktop() { m_stack->setCurrentWidget(m_desktop); m_game->setCaptured(false); }
void MainWindow::showAndroid() { m_stack->setCurrentWidget(m_android); m_android->host()->focusChild(); }

void MainWindow::onState(AndroidEngine::State st, const QString& msg) {
    using S = AndroidEngine::State;
    m_desktop->setAndroidState(st, msg);
    m_android->setMessage(msg);
    m_tool->setAndroidReady(st == S::Ready);
    if (st != S::Ready) { m_fpsText.clear(); m_game->setArmed(false); }
    if (st == S::Stopped || st == S::Error || st == S::NotInstalled) { m_android->detach(); m_apps->setInstalled({}); }
    if (st == S::Ready) m_game->setScreen(m_engine->screenSize());
    updateTitle();
}

void MainWindow::updateTitle() {
    using S = AndroidEngine::State;
    const S st = m_engine->state();
    QStringList parts{m_engine->message()};
    if (!m_fpsText.isEmpty()) parts << m_fpsText;
    if (!m_latText.isEmpty() && m_game->isArmed()) parts << m_latText;
    if (!m_flash.isEmpty()) parts << m_flash;
    const QColor dot = st == S::Ready ? QColor("#4ade80") : st == S::Error ? QColor("#f87171") : (st == S::Starting || st == S::Booting) ? QColor("#fbbf24") : QColor("#a59ec4");
    m_title->setStatus(parts.join("   ·   "), dot);
}

void MainWindow::refreshDock() {
    const bool ready = m_engine->state() == AndroidEngine::State::Ready;
    m_desktop->dock()->setApps(m_apps->dockApps(), ready);
    QList<AppEntry> sc;
    for (const AppEntry& a : m_apps->installed())
        if (a.package == "com.android.vending" || a.package == "com.google.android.documentsui" || a.package == "com.android.chrome") sc << a;
    m_desktop->setShortcuts(sc);
}

void MainWindow::launch(const QString& pkg) {
    if (m_engine->state() != AndroidEngine::State::Ready) return;
    const QString prof = Profiles::forPackage(pkg);
    if (!prof.isEmpty()) { m_s->setValue("input.profile", prof); m_s->save(); m_game->setProfile(Profiles::load(prof)); }
    m_engine->adb()->launch(pkg);
    showAndroid();
}

void MainWindow::onToolbar(const QString& id) {
    AdbClient* adb = m_engine->adb();
    if (id == "fullscreen") { isFullScreen() ? showNormal() : showFullScreen(); m_title->setVisible(!isFullScreen()); }
    else if (id == "settings") openSettings(0);
    else if (id == "profiles") openSettings(4);
    else if (id == "home") { if (m_engine->state() == AndroidEngine::State::Ready) adb->keyEvent(3); showDesktop(); }
    else if (id == "android") showAndroid();
    else if (id == "keymap") {
        m_game->setArmed(!m_game->isArmed());
        if (m_game->isArmed()) { m_flash = "Игровой режим: нажмите ` для захвата курсора"; updateTitle(); }
        else { m_flash.clear(); updateTitle(); }
    }
    else if (id == "volume") {
        QMenu m; m.addAction("Громче", this, [adb] { adb->keyEvent(24); }); m.addAction("Тише", this, [adb] { adb->keyEvent(25); });
        m.addAction("Без звука / со звуком", this, [adb] { adb->keyEvent(164); });
        m.exec(QCursor::pos());
    }
    else if (id == "rotate") m_engine->console()->send("rotate");
    else if (id == "apk") installApk();
    else if (id == "screenshot") {
        const QString out = Paths::screenshotsDir() + "/NEZER_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".png";
        adb->screenshot(out, [this, out](bool ok) { m_flash = ok ? "Скриншот сохранён: " + out : "Не удалось сделать скриншот"; updateTitle();
            QTimer::singleShot(5000, this, [this] { m_flash.clear(); updateTitle(); }); });
    }
}

void MainWindow::installApk(const QString& given) {
    if (m_engine->state() != AndroidEngine::State::Ready) { QMessageBox::information(this, "NEZER EMU", "Сначала дождитесь запуска Android."); return; }
    const QString path = given.isEmpty() ? QFileDialog::getOpenFileName(this, "Установить APK", {}, "Android APK (*.apk)") : given;
    if (path.isEmpty()) return;
    const ApkInfo info = ApkInspector::inspect(path);
    if (!info.valid) { QMessageBox::warning(this, "NEZER EMU", info.error); return; }
    const QString note = ApkInspector::compatibilityNote(info, m_engine->deviceAbi());
    if (QMessageBox::question(this, "Установка APK", QFileInfo(path).fileName() + "\n\n" + note + "\n\nУстановить?") != QMessageBox::Yes) return;
    m_flash = "Установка " + QFileInfo(path).fileName() + "…"; updateTitle();
    m_engine->adb()->install(path, [this, path](int code, QString out) {
        m_flash.clear(); updateTitle();
        if (code == 0 && out.contains("Success")) { QMessageBox::information(this, "NEZER EMU", "Приложение установлено."); m_engine->refreshApps(); }
        else QMessageBox::warning(this, "NEZER EMU", ApkInspector::explainInstallError(out));
    });
}

void MainWindow::openSettings(int section) {
    SettingsDialog d(m_s, m_engine, m_game, this);
    d.openSection(section);
    connect(&d, &SettingsDialog::appearanceChanged, this, &MainWindow::applyAppearance);
    connect(&d, &SettingsDialog::inputChanged, this, [this] { m_game->reloadSettings(); m_game->setScreen(m_engine->screenSize()); m_game->setProfile(Profiles::load(m_s->value("input.profile").toString())); });
    connect(&d, &SettingsDialog::setupRequested, this, [this, &d] { d.close(); openSetup(); });
    connect(&d, &SettingsDialog::logRequested, this, &MainWindow::showLog);
    d.exec();
}

void MainWindow::openSetup() {
    m_setupShown = true;
    if (m_engine->state() == AndroidEngine::State::Ready || m_engine->state() == AndroidEngine::State::Booting) m_engine->stop();
    SetupWizard w(m_s, this);
    const bool ok = w.exec() == QDialog::Accepted;
    m_engine->refreshInstalledState();
    if (ok) m_engine->start();
}

void MainWindow::showLog() {
    QDialog d(this); d.setWindowTitle("Журнал Android Emulator"); d.resize(860, 520);
    auto* v = new QVBoxLayout(&d); auto* t = new QPlainTextEdit; t->setReadOnly(true);
    t->setPlainText(m_log.isEmpty() ? "Журнал пуст: эмулятор ещё не запускался." : m_log.join('\n'));
    auto* close = new QPushButton("Закрыть"); connect(close, &QPushButton::clicked, &d, &QDialog::accept);
    v->addWidget(t); v->addWidget(close, 0, Qt::AlignRight); d.exec();
}

void MainWindow::closeEvent(QCloseEvent* e) {
    m_s->setValue("window.geometry", QString::fromLatin1(saveGeometry().toBase64())); m_s->save();
    m_game->setArmed(false);
    m_engine->stop();   // эмулятор завершается штатно и сохраняет состояние
    e->accept();
}

void MainWindow::dragEnterEvent(QDragEnterEvent* e) {
    for (const QUrl& u : e->mimeData()->urls()) if (u.toLocalFile().endsWith(".apk", Qt::CaseInsensitive)) { e->acceptProposedAction(); return; }
}
void MainWindow::dropEvent(QDropEvent* e) {
    for (const QUrl& u : e->mimeData()->urls()) if (u.toLocalFile().endsWith(".apk", Qt::CaseInsensitive)) { installApk(u.toLocalFile()); return; }
}

// Изменение размера окна без рамки за края.
bool MainWindow::nativeEvent(const QByteArray& type, void* message, qintptr* result) {
#ifdef Q_OS_WIN
    MSG* msg = static_cast<MSG*>(message);
    if (type == "windows_generic_MSG" && msg->message == WM_NCHITTEST && !isMaximized() && !isFullScreen()) {
        const qreal k = devicePixelRatioF();
        const QPoint p = mapFromGlobal(QPoint(int(GET_X_LPARAM(msg->lParam) / k), int(GET_Y_LPARAM(msg->lParam) / k)));
        const int b = 6;
        const bool l = p.x() < b, r = p.x() > width() - b, t = p.y() < b, bo = p.y() > height() - b;
        if (t && l) { *result = HTTOPLEFT; return true; }
        if (t && r) { *result = HTTOPRIGHT; return true; }
        if (bo && l) { *result = HTBOTTOMLEFT; return true; }
        if (bo && r) { *result = HTBOTTOMRIGHT; return true; }
        if (l) { *result = HTLEFT; return true; }
        if (r) { *result = HTRIGHT; return true; }
        if (t) { *result = HTTOP; return true; }
        if (bo) { *result = HTBOTTOM; return true; }
    }
#else
    Q_UNUSED(type) Q_UNUSED(message) Q_UNUSED(result)
#endif
    return false;
}
