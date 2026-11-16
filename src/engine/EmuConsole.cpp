#include "EmuConsole.h"
#include <QDir>
#include <QFile>

QElapsedTimer& EmuConsole::clock() { static QElapsedTimer t = [] { QElapsedTimer e; e.start(); return e; }(); return t; }

EmuConsole::EmuConsole(QObject* parent) : QObject(parent) {
    connect(&m_sock, &QTcpSocket::readyRead, this, &EmuConsole::onRead);
    connect(&m_sock, &QTcpSocket::disconnected, this, [this] { m_ready = false; m_authSent = false; m_pending.clear(); emit disconnected(); });
}

void EmuConsole::connectTo(int port) {
    m_sock.abort(); m_ready = false; m_authSent = false; m_buf.clear(); m_pending.clear();
    m_sock.connectToHost("127.0.0.1", quint16(port));
    m_sock.setSocketOption(QAbstractSocket::LowDelayOption, 1);
}

void EmuConsole::send(const QByteArray& line, qint64 ts) {
    if (!m_ready) return;
    m_pending.enqueue(ts);
    m_sock.write(line + "\n");
    m_sock.flush();
}

void EmuConsole::onRead() {
    m_buf += m_sock.readAll();
    int nl;
    while ((nl = m_buf.indexOf('\n')) >= 0) {
        const QByteArray line = m_buf.left(nl).trimmed();
        m_buf.remove(0, nl + 1);
        if (!m_authSent) {
            if (line.startsWith("OK")) {
                QFile f(QDir::homePath() + "/.emulator_console_auth_token");
                const QByteArray tok = f.open(QIODevice::ReadOnly) ? f.readAll().trimmed() : QByteArray();
                m_authSent = true;
                m_sock.write("auth " + tok + "\n");
                m_pending.enqueue(-1);
            }
            continue;
        }
        if (line.startsWith("OK") || line.startsWith("KO")) {
            const qint64 ts = m_pending.isEmpty() ? 0 : m_pending.dequeue();
            if (ts == -1) { if (line.startsWith("OK")) { m_ready = true; emit ready(); } continue; }
            if (ts > 0) emit ackLatency((clock().nsecsElapsed() - ts) / 1e6);
        }
    }
}
