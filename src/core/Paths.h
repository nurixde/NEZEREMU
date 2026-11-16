#pragma once
#include <QProcessEnvironment>
#include <QString>

// Все пользовательские данные: %LOCALAPPDATA%/NezerEmu (сохраняются при обновлении программы)
namespace Paths {
QString ensure(const QString& dir);
QString dataDir();
QString sdkRoot();
QString avdHome();
QString profilesDir();
QString logsDir();
QString tempDir();
QString screenshotsDir();
QString assetsDir();
QString emulatorExe();
QString adbExe();
QProcessEnvironment sdkEnv();
}
