#pragma once
#include "AvdConfig.h"
#include <QObject>
#include <QProcess>
#include <QTimer>

// Запуск настоящего Android Emulator (Google, AOSP) и поиск его окна для встраивания.
class EmulatorProcess : public QObject {
    Q_OBJECT
public:
    explicit EmulatorProcess(QObject* parent = nullptr);
    ~EmulatorProcess() override;
    void start(const AvdParams& p, bool coldBoot, bool wipeData);
    void stop();
    bool isRunning() const { return m_proc.state() != QProcess::NotRunning; }
    quint32 pid() const { return quint32(m_proc.processId()); }
    static QString diagnose(const QString& log);
signals:
    void logLine(const QString& line);
    void windowFound(quintptr hwnd);
    void failed(const QString& reason);
    void exited();
private:
    void scanWindows();
    QProcess m_proc;
    QTimer m_scan;
    int m_scanTries = 0;
    quintptr m_hwnd = 0;
    QString m_log;
};
