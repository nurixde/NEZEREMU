#include "Paths.h"
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

namespace Paths {
QString ensure(const QString& d) { QDir().mkpath(d); return d; }
QString dataDir() { return ensure(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/NezerEmu"); }
QString sdkRoot() { return ensure(dataDir() + "/sdk"); }
QString avdHome() { return ensure(dataDir() + "/avd"); }
QString profilesDir() { return ensure(dataDir() + "/profiles"); }
QString logsDir() { return ensure(dataDir() + "/logs"); }
QString tempDir() { return ensure(dataDir() + "/tmp"); }
QString screenshotsDir() { return ensure(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "/NEZER EMU"); }
QString assetsDir() { return QCoreApplication::applicationDirPath() + "/assets"; }
QString emulatorExe() { return sdkRoot() + "/emulator/emulator.exe"; }
QString adbExe() { return sdkRoot() + "/platform-tools/adb.exe"; }
QProcessEnvironment sdkEnv() {
    auto env = QProcessEnvironment::systemEnvironment();
    env.insert("ANDROID_SDK_ROOT", QDir::toNativeSeparators(sdkRoot()));
    env.insert("ANDROID_HOME", QDir::toNativeSeparators(sdkRoot()));
    env.insert("ANDROID_AVD_HOME", QDir::toNativeSeparators(avdHome()));
    return env;
}
}
