#pragma once
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>

// Работа с установленным BlueStacks 5 (BlueStacks_nxt / MSI App Player).
// NEZER EMU не распространяет BlueStacks: пользователь ставит его сам с bluestacks.com.
namespace BlueStacks {
struct Install {
    bool found = false;
    QString installDir, dataDir, player, adb, multiInstance, conf;
};
Install detect();
bool isInstalled();
QString adbExe();
QMap<QString, QString> readConf();
QStringList instances();
QString defaultInstance();
int adbPort(const QString& instance);
bool writeConf(const QMap<QString, QString>& changes, QString* error = nullptr);
QList<quint32> playerPids();
inline const char* kDownloadUrl = "https://www.bluestacks.com/download.html";
}
