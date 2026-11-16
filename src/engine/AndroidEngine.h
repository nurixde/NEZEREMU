#pragma once
#include "AvdConfig.h"
#include <QObject>
#include <QSize>
#include <QStringList>
#include <QTimer>
class SettingsStore; class AdbClient; class EmuConsole; class EmulatorProcess; class FpsMonitor;

// Фасад Android-движка: запуск, загрузка, готовность, список приложений.
class AndroidEngine : public QObject {
    Q_OBJECT
public:
    enum class State { NotInstalled, Stopped, Starting, Booting, Ready, Error };
    Q_ENUM(State)
    explicit AndroidEngine(SettingsStore* s, QObject* parent = nullptr);
    ~AndroidEngine() override;
    AdbClient* adb() const { return m_adb; }
    EmuConsole* console() const { return m_console; }
    FpsMonitor* fps() const { return m_fps; }
    State state() const { return m_state; }
    QString message() const { return m_msg; }
    QStringList apps() const { return m_apps; }
    QString deviceAbi() const { return m_abi; }
    QSize screenSize() const;
    AvdParams params() const;
    static void modeDefaults(const QString& mode, int* cores, int* ramMb, int* priority);
    void refreshInstalledState();
    void start();
    void stop();
    void restart();
    void refreshApps();
    void applyLiveDisplay();
    void applyFrameRate();
signals:
    void stateChanged(AndroidEngine::State st, const QString& msg);
    void appsChanged(const QStringList& packages);
    void windowReady(quintptr hwnd);
    void logLine(const QString& line);
private:
    void setState(State s, const QString& m);
    void pollBoot();
    SettingsStore* m_s; AdbClient* m_adb; EmuConsole* m_console; EmulatorProcess* m_proc; FpsMonitor* m_fps;
    State m_state = State::Stopped; QString m_msg; QStringList m_apps; QString m_abi = "x86_64";
    QTimer m_boot; int m_bootTries = 0;
};
