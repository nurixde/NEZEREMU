#include "AdbClient.h"
#include "AdbParsers.h"
#include "ProcUtil.h"
#include "core/Paths.h"
#include <QFile>

void AdbClient::run(const QStringList& args, int timeoutMs, Cb cb) {
    ProcUtil::run(this, Paths::adbExe(), QStringList{"-s", m_serial} + args, timeoutMs, cb);
}
void AdbClient::shell(const QString& cmd, Cb cb, int t) { run({"shell", cmd}, t, cb); }

void AdbClient::bootCompleted(std::function<void(bool)> cb) {
    shell("getprop sys.boot_completed", [cb](int c, QString o) { cb(c == 0 && o.trimmed() == "1"); }, 5000);
}

void AdbClient::launchablePackages(std::function<void(QStringList)> cb) {
    shell("cmd package query-activities --brief -a android.intent.action.MAIN -c android.intent.category.LAUNCHER",
          [cb](int c, QString o) { cb(c == 0 ? AdbParsers::packagesFromQueryActivities(o) : QStringList{}); });
}

void AdbClient::launch(const QString& pkg) {
    shell("monkey -p " + pkg + " -c android.intent.category.LAUNCHER 1", nullptr);
}

void AdbClient::install(const QString& apk, Cb cb) { run({"install", "-r", "-g", apk}, 10 * 60 * 1000, cb); }
void AdbClient::keyEvent(int code) { shell("input keyevent " + QString::number(code), nullptr, 5000); }

void AdbClient::screenshot(const QString& out, std::function<void(bool)> cb) {
    const QString remote = "/sdcard/nezer_shot.png";
    shell("screencap -p " + remote, [this, out, remote, cb](int c, QString) {
        if (c != 0) { cb(false); return; }
        run({"pull", remote, out}, 20000, [cb, out](int c2, QString) { cb(c2 == 0 && QFile::exists(out)); });
    });
}

void AdbClient::setDisplay(int w, int h, int dpi) {
    shell(QString("wm size %1x%2 && wm density %3").arg(w).arg(h).arg(dpi), nullptr);
}

void AdbClient::setRefreshRate(int hz) {
    shell(QString("settings put system peak_refresh_rate %1.0 && settings put system min_refresh_rate %1.0").arg(hz), nullptr);
}

void AdbClient::deviceAbi(std::function<void(QString)> cb) {
    shell("getprop ro.product.cpu.abi", [cb](int, QString o) { cb(o.trimmed()); }, 5000);
}
