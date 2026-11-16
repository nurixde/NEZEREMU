#include "ProcUtil.h"
#include "core/Paths.h"
#include <QDir>
#include <QTimer>
#include <memory>

namespace ProcUtil {
void setCommand(QProcess* p, const QString& program, const QStringList& args) {
    p->setProcessEnvironment(Paths::sdkEnv());
#ifdef Q_OS_WIN
    if (program.endsWith(".bat", Qt::CaseInsensitive)) {
        QString line = "\"" + QDir::toNativeSeparators(program) + "\"";
        for (const QString& a : args) line += " \"" + a + "\"";
        p->setProgram("cmd.exe");
        p->setNativeArguments("/s /c \"" + line + "\"");
        return;
    }
#endif
    p->setProgram(program);
    p->setArguments(args);
}

QProcess* run(QObject* owner, const QString& program, const QStringList& args, int timeoutMs,
              std::function<void(int, QString)> cb, const QByteArray& stdinData) {
    auto* p = new QProcess(owner);
    setCommand(p, program, args);
    p->setProcessChannelMode(QProcess::MergedChannels);
    auto done = std::make_shared<bool>(false);
    auto finish = [p, cb, done](int code) {
        if (*done) return; *done = true;
        const QString out = QString::fromUtf8(p->readAll());
        if (cb) cb(code, out);
        p->deleteLater();
    };
    QObject::connect(p, &QProcess::finished, p, [finish](int code, QProcess::ExitStatus st) { finish(st == QProcess::NormalExit ? code : -1); });
    QObject::connect(p, &QProcess::errorOccurred, p, [finish](QProcess::ProcessError e) { if (e == QProcess::FailedToStart) finish(-1); });
    if (timeoutMs > 0) QTimer::singleShot(timeoutMs, p, [p, finish] { p->kill(); finish(-1); });
    p->start();
    if (!stdinData.isEmpty()) { p->write(stdinData); p->closeWriteChannel(); }
    return p;
}
}
