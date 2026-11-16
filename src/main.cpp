#include "core/Paths.h"
#include "settings/SettingsStore.h"
#include "ui/MainWindow.h"
#include "ui/Theme.h"
#include <QApplication>
#include <QLockFile>
#include <QMessageBox>

int main(int argc, char** argv) {
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication app(argc, argv);
    app.setApplicationName("NezerEmu");
    app.setOrganizationName("NEZER");
    app.setApplicationVersion(NEZER_VERSION);
    app.setWindowIcon(QIcon(Paths::assetsDir() + "/nezer.ico"));
    QFont f("Segoe UI"); f.setPixelSize(13); app.setFont(f);

    QLockFile lock(Paths::dataDir() + "/nezer.lock");
    if (!lock.tryLock(100)) {
        QMessageBox::information(nullptr, "NEZER EMU", "NEZER EMU уже запущен.");
        return 0;
    }
    SettingsStore settings;
    settings.load();
    Theme::applyStyle(settings.value("appearance.style").toString());
    app.setStyleSheet(Theme::globalStyle(settings.value("appearance.accent").toString()));
    MainWindow w(&settings);
    w.show();
    return app.exec();
}
