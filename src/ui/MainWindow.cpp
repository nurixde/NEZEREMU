#include "MainWindow.h"
#include "AllAppsDialog.h"
#include "AndroidView.h"
#include "DesktopScreen.h"
#include "Dock.h"
#include "LockScreen.h"
#include "SettingsDialog.h"
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
#include "engine/BlueStacks.h"
#include "engine/EmuConsole.h"
#include "input/GameInput.h"
#include "settings/SettingsStore.h"
#include <QApplication>
#include <QCloseEvent>
#include <QDateTime>
#include <QDesktopServices>
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
#include <QUrl>
#include <QVBoxLayout>
#ifdef Q_OS_WIN
#include <windows.h>
#include <windowsx.h>
#endif

#ifdef Q_OS_WIN
// Ctrl+Shift+<vk> в окно с фокусом (BlueStacks): Ctrl+Shift+A открывает редактор игрового управления.
static void sendCtrlShift(WORD vk) {
    INPUT in[6]{};
    for (INPUT& i : in) i.type = INPUT_KEYBOARD;
    in[0].ki.wVk = VK_CONTROL; in[1].ki.wVk = VK_SHIFT; in[2].ki.wVk = vk;
    in[3].ki.wVk = vk; in[3].ki.dwFlags = KEYEVENTF_KEYUP;
    in[4].ki.wVk = VK_SHIFT; in[4].ki.dwFlags = KEYEVENTF_KEYUP;
    in[5].ki.wVk = VK_CONTROL; in[5].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(6, in, sizeof(INPUT));
}
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
    connect(m_engine, &AndroidEngine::windowReady, this, [this](quintptr h) { m_android->attach(h); m_android->host()->setChildInputEnabled(true); });
    connect(m_engine, &AndroidEngine::appsChanged, m_apps, &AppManager::setInstalled);
    connect(m_engine, &AndroidEngine::logLine, this, [this](const QString& l) { m_log << l; if (m_log.size() > 4000) m_log.removeFirst(); });
    connect(m_engine->fps(), &FpsMonitor::sample, this, [this](double fps, double frameMs, double jitter) {
        m_fpsText = fps < 0 ? "FPS: н/д" : QString("FPS %1 · кадр %2 мс · разброс %3 мс").arg(fps, 0, 'f', 1).arg(frameMs, 0, 'f', 1).arg(jitter, 0, 'f', 1);
        updateTitle();
    });

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
    Theme::applyStyle(m_s->value("appearance.style").toString());
    setStyleSheet(QString("MainWindow{background:%1;}").arg(Theme::kBg));
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
    if (!m_flash.isEmpty()) parts << m_flash;
    const QColor dot = st == S::Ready ? QColor("#4ade80") : st == S::Error ? QColor("#f87171") : (st == S::Starting || st == S::Booting) ? QColor("#fbbf24") : QColor(Theme::kMuted);
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
    m_engine->adb()->launch(pkg);
    showAndroid();
}

void MainWindow::onToolbar(const QString& id) {
    AdbClient* adb = m_engine->adb();
    const bool ready = m_engine->state() == AndroidEngine::State::Ready;
    if (id == "fullscreen") { isFullScreen() ? showNormal() : showFullScreen(); m_title->setVisible(!isFullScreen()); }
    else if (id == "settings") openSettings(0);
    else if (id == "profiles") openSettings(4);
    else if (id == "home") { if (ready) adb->keyEvent(3); showDesktop(); }
    else if (id == "android") showAndroid();
    else if (id == "keymap") {
        if (!ready) { QMessageBox::information(this, "NEZER EMU", "Сначала дождитесь запуска Android."); return; }
        showAndroid();
        QTimer::singleShot(250, this, [this] {
            m_android->host()->focusChild();
#ifdef Q_OS_WIN
            sendCtrlShift('A');
#endif
            m_flash = "Игровое управление BlueStacks (Ctrl+Shift+A)"; updateTitle();
            QTimer::singleShot(5000, this, [this] { m_flash.clear(); updateTitle(); });
        });
    }
    else if (id == "volume") {
        QMenu m; m.addAction("Громче", this, [adb] { adb->keyEvent(24); }); m.addAction("Тише", this, [adb] { adb->keyEvent(25); });
        m.addAction("Без звука / со звуком", this, [adb] { adb->keyEvent(164); });
        m.exec(QCursor::pos());
    }
    else if (id == "rotate") {
        if (!ready) return;
        static int rot = 0;
        rot = rot ? 0 : 1;
        adb->shell(QString("settings put system accelerometer_rotation 0 && settings put system user_rotation %1").arg(rot), nullptr);
    }
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
    m_engine->adb()->install(path, [this](int code, QString out) {
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

// Вместо загрузки Android SDK: проверяем, что BlueStacks 5 установлен.
void MainWindow::openSetup() {
    m_setupShown = true;
    for (;;) {
        m_engine->refreshInstalledState();
        const AndroidEngine::State st = m_engine->state();
        if (st != AndroidEngine::State::NotInstalled) {
            if (st == AndroidEngine::State::Stopped || st == AndroidEngine::State::Error) m_engine->start();
            return;
        }
        QMessageBox box(this);
        box.setWindowTitle("NEZER EMU");
        box.setIcon(QMessageBox::Information);
        box.setText("Для работы NEZER EMU нужен BlueStacks 5.");
        box.setInformativeText("1. Скачайте и установите BlueStacks 5 с официального сайта.\n"
                               "2. Запустите его один раз и закройте.\n"
                               "3. Нажмите «Проверить снова».\n\n"
                               "NEZER EMU встроит окно BlueStacks в свой интерфейс и будет управлять Android через ADB.");
        QPushButton* dl = box.addButton("Скачать BlueStacks", QMessageBox::ActionRole);
        QPushButton* again = box.addButton("Проверить снова", QMessageBox::AcceptRole);
        box.addButton("Позже", QMessageBox::RejectRole);
        box.exec();
        if (box.clickedButton() == dl) { QDesktopServices::openUrl(QUrl(QString::fromLatin1(BlueStacks::kDownloadUrl))); continue; }
        if (box.clickedButton() != again) return;
    }
}

void MainWindow::showLog() {
    QDialog d(this); d.setWindowTitle("Журнал NEZER EMU / BlueStacks"); d.resize(860, 520);
    auto* v = new QVBoxLayout(&d); auto* t = new QPlainTextEdit; t->setReadOnly(true);
    t->setPlainText(m_log.isEmpty() ? "Журнал пуст: BlueStacks ещё не запускался." : m_log.join('\n'));
    auto* close = new QPushButton("Закрыть"); connect(close, &QPushButton::clicked, &d, &QDialog::accept);
    v->addWidget(t); v->addWidget(close, 0, Qt::AlignRight); d.exec();
}

void MainWindow::closeEvent(QCloseEvent* e) {
    m_s->setValue("window.geometry", QString::fromLatin1(saveGeometry().toBase64())); m_s->save();
    m_game->setArmed(false);
    m_engine->stop();   // BlueStacks закрывается штатно и сохраняет данные
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
