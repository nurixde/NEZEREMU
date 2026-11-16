#pragma once
#include <QElapsedTimer>
#include <QObject>
#include <QQueue>
#include <QTcpSocket>

// Постоянное TCP-соединение с консолью эмулятора (порт 5554).
// События касаний идут сюда напрямую (без запуска adb на каждое движение).
class EmuConsole : public QObject {
    Q_OBJECT
public:
    explicit EmuConsole(QObject* parent = nullptr);
    void connectTo(int port = 5554);
    bool isReady() const { return m_ready; }
    void send(const QByteArray& line, qint64 inputTimestampNs = 0);   // inputTimestampNs: для замера задержки
    static QElapsedTimer& clock();
signals:
    void ready();
    void disconnected();
    void ackLatency(double ms);   // от события мыши до подтверждения эмулятора
private:
    void onRead();
    QTcpSocket m_sock;
    bool m_ready = false;
    bool m_authSent = false;
    QByteArray m_buf;
    QQueue<qint64> m_pending;
};
