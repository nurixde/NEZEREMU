#pragma once
#include <QObject>
#include <functional>

// Служебные команды Android (установка, список приложений, скриншот).
// В игровом вводе ADB НЕ используется: см. EmuConsole.
class AdbClient : public QObject {
    Q_OBJECT
public:
    using Cb = std::function<void(int, QString)>;
    explicit AdbClient(QObject* parent = nullptr) : QObject(parent) {}
    void setSerial(const QString& s) { m_serial = s; }
    void run(const QStringList& args, int timeoutMs, Cb cb);
    void shell(const QString& cmd, Cb cb, int timeoutMs = 15000);
    void bootCompleted(std::function<void(bool)> cb);
    void launchablePackages(std::function<void(QStringList)> cb);
    void launch(const QString& pkg);
    void install(const QString& apk, Cb cb);
    void keyEvent(int code);
    void screenshot(const QString& outPng, std::function<void(bool)> cb);
    void setDisplay(int w, int h, int dpi);
    void setRefreshRate(int hz);
    void deviceAbi(std::function<void(QString)> cb);
private:
    QString m_serial = "emulator-5554";
};
