#pragma once
#include "AvdConfig.h"
#include <QObject>
#include <QUrl>
#include <functional>
class QNetworkAccessManager;

// Первоначальная настройка Android официальными средствами Google:
// Java (Eclipse Temurin) -> Android cmdline-tools -> sdkmanager (лицензии принимает пользователь)
// -> platform-tools, emulator, system image Google Play -> avdmanager.
// NEZER EMU не распространяет эти компоненты: они скачиваются напрямую с серверов правообладателей.
class SdkInstaller : public QObject {
    Q_OBJECT
public:
    explicit SdkInstaller(QObject* parent = nullptr);
    static bool isInstalled();
    static QString systemImagePackage(int api, const QString& flavor);
    void run(const AvdParams& p, int api, const QString& flavor);
    void cancel();
    void checkAcceleration(std::function<void(bool, QString)> cb);
signals:
    void progress(int percent, const QString& stage);
    void log(const QString& line);
    void finished(bool ok, const QString& message);
private:
    void step();
    void download(const QUrl& url, const QString& file, std::function<void(bool)> next);
    void extract(const QString& zip, const QString& dest, std::function<void(bool)> next);
    void tool(const QString& bat, const QStringList& args, const QByteArray& stdinData, int timeoutMin, std::function<void(bool, QString)> next);
    void fail(const QString& m);
    QNetworkAccessManager* m_net;
    AvdParams m_params; int m_api = 34; QString m_flavor; int m_stage = 0; bool m_cancel = false;
    class QProcess* m_current = nullptr;
};
