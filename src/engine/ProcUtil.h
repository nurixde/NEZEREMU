#pragma once
#include <QProcess>
#include <QStringList>
#include <functional>

namespace ProcUtil {
// Запуск .bat/.exe с точными кавычками (в именах пакетов SDK есть ';').
void setCommand(QProcess* p, const QString& program, const QStringList& args);
// Асинхронный запуск: cb(exitCode, stdout+stderr). exitCode -1 = тайм-аут/не запустилось.
QProcess* run(QObject* owner, const QString& program, const QStringList& args, int timeoutMs,
              std::function<void(int, QString)> cb, const QByteArray& stdinData = {});
}
