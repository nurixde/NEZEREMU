#pragma once
#include <QString>
#include <QStringList>

// Проверка APK до установки и диагностика ошибок установки.
struct ApkInfo {
    bool valid = false;
    QStringList abis;        // из lib/<abi>/ внутри APK; пусто = только Java/Kotlin
    bool hasManifest = false;
    bool isSplitBundle = false;
    QString error;
};

namespace ApkInspector {
ApkInfo inspect(const QString& path);
QStringList zipEntryNames(const QByteArray& zipData, QString* error);
QString compatibilityNote(const ApkInfo& info, const QString& deviceAbi);
QString explainInstallError(const QString& adbOutput);
}
