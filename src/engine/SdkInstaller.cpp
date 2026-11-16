#include "SdkInstaller.h"
#include "ProcUtil.h"
#include "core/Paths.h"
#include <QDir>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProcess>
#include <QRegularExpression>
#include <QTimer>
#include <memory>

static const char* kJreUrl = "https://api.adoptium.net/v3/binary/latest/17/ga/windows/x64/jre/hotspot/normal/eclipse";
static const char* kToolsUrl = "https://dl.google.com/android/repository/commandlinetools-win-11076708_latest.zip";

static QString jreHome() { return Paths::sdkRoot() + "/jre"; }
static QString toolsBin() { return Paths::sdkRoot() + "/cmdline-tools/latest/bin"; }

SdkInstaller::SdkInstaller(QObject* parent) : QObject(parent), m_net(new QNetworkAccessManager(this)) {
    m_net->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
}

bool SdkInstaller::isInstalled() {
    return QFile::exists(Paths::emulatorExe()) && QFile::exists(Paths::adbExe()) && AvdConfig::exists();
}

QString SdkInstaller::systemImagePackage(int api, const QString& flavor) {
    return QString("system-images;android-%1;%2;x86_64").arg(api).arg(flavor);
}

void SdkInstaller::run(const AvdParams& p, int api, const QString& flavor) {
    m_params = p; m_api = api; m_flavor = flavor; m_stage = 0; m_cancel = false;
    step();
}

void SdkInstaller::cancel() { m_cancel = true; if (m_current) m_current->kill(); }
void SdkInstaller::fail(const QString& m) { emit finished(false, m); }

void SdkInstaller::step() {
    if (m_cancel) { fail("Установка отменена"); return; }
    const QString sdk = Paths::sdkRoot(), tmp = Paths::tempDir();
    auto next = [this](bool ok) { if (ok) { ++m_stage; step(); } };
    switch (m_stage) {
    case 0:
        if (QFile::exists(jreHome() + "/bin/java.exe")) { ++m_stage; step(); return; }
        emit progress(0, "Загрузка Java (Eclipse Temurin 17)");
        download(QUrl(kJreUrl), tmp + "/jre.zip", [=](bool ok) {
            if (!ok) return;
            extract(tmp + "/jre.zip", tmp + "/jre_x", [=](bool ok2) {
                if (!ok2) return;
                const QStringList d = QDir(tmp + "/jre_x").entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                QDir(jreHome()).removeRecursively();
                if (d.isEmpty() || !QDir().rename(tmp + "/jre_x/" + d.first(), jreHome())) { fail("Не удалось распаковать Java"); return; }
                QDir(tmp + "/jre_x").removeRecursively(); QFile::remove(tmp + "/jre.zip");
                next(true);
            });
        });
        return;
    case 1:
        if (QFile::exists(toolsBin() + "/sdkmanager.bat")) { ++m_stage; step(); return; }
        emit progress(0, "Загрузка Android SDK Command-line Tools");
        download(QUrl(kToolsUrl), tmp + "/tools.zip", [=](bool ok) {
            if (!ok) return;
            extract(tmp + "/tools.zip", tmp + "/tools_x", [=](bool ok2) {
                if (!ok2) return;
                QDir().mkpath(sdk + "/cmdline-tools");
                QDir(sdk + "/cmdline-tools/latest").removeRecursively();
                if (!QDir().rename(tmp + "/tools_x/cmdline-tools", sdk + "/cmdline-tools/latest")) { fail("Не удалось распаковать cmdline-tools"); return; }
                QDir(tmp + "/tools_x").removeRecursively(); QFile::remove(tmp + "/tools.zip");
                next(true);
            });
        });
        return;
    case 2:
        emit progress(0, "Принятие лицензий Android SDK");
        tool("sdkmanager.bat", {"--sdk_root=" + QDir::toNativeSeparators(sdk), "--licenses"}, QByteArray("y\n").repeated(30), 10,
             [=](bool, QString) { next(true); });
        return;
    case 3: {
        emit progress(0, "Загрузка Android Emulator и образа Android " + QString::number(m_api) + " (несколько ГБ)");
        const QString list = tmp + "/packages.txt";
        QFile f(list);
        if (f.open(QIODevice::WriteOnly)) { f.write(("platform-tools\nemulator\n" + systemImagePackage(m_api, m_flavor) + "\n").toUtf8()); f.close(); }
        tool("sdkmanager.bat", {"--sdk_root=" + QDir::toNativeSeparators(sdk), "--package_file=" + QDir::toNativeSeparators(list)}, QByteArray("y\n").repeated(10), 120,
             [=](bool ok, QString out) {
                 if (!ok || !QFile::exists(Paths::emulatorExe())) { fail("sdkmanager не смог скачать компоненты. Проверьте интернет.\n" + out.right(400)); return; }
                 next(true);
             });
        return; }
    case 4:
        emit progress(0, "Создание виртуального устройства");
        tool("avdmanager.bat", {"create", "avd", "-n", AvdConfig::kAvdName, "-k", systemImagePackage(m_api, m_flavor), "-d", "pixel_6", "--force"},
             "no\n", 5, [=](bool ok, QString out) {
                 if (!ok || !AvdConfig::exists()) { fail("Не удалось создать устройство:\n" + out.right(400)); return; }
                 QString e;
                 if (!AvdConfig::write(m_params, &e)) { fail(e); return; }
                 next(true);
             });
        return;
    default:
        emit progress(100, "Готово");
        emit finished(true, "Android установлен");
    }
}

void SdkInstaller::download(const QUrl& url, const QString& file, std::function<void(bool)> next) {
    auto* out = new QFile(file, this);
    if (!out->open(QIODevice::WriteOnly)) { delete out; fail("Нет доступа к папке " + file); next(false); return; }
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, "NezerEmu-Setup");
    QNetworkReply* r = m_net->get(req);
    connect(r, &QNetworkReply::readyRead, this, [r, out] { out->write(r->readAll()); });
    connect(r, &QNetworkReply::downloadProgress, this, [this](qint64 got, qint64 total) {
        if (total > 0) emit progress(int(got * 100 / total), {});
    });
    connect(r, &QNetworkReply::finished, this, [this, r, out, next, url] {
        out->write(r->readAll()); out->close(); out->deleteLater(); r->deleteLater();
        if (m_cancel) { fail("Установка отменена"); next(false); return; }
        if (r->error() != QNetworkReply::NoError) { fail("Ошибка загрузки " + url.host() + ": " + r->errorString()); next(false); return; }
        next(true);
    });
}

void SdkInstaller::extract(const QString& zip, const QString& dest, std::function<void(bool)> next) {
    emit progress(-1, "Распаковка");
    QDir(dest).removeRecursively();
    const QString cmd = QString("Expand-Archive -LiteralPath '%1' -DestinationPath '%2' -Force")
                            .arg(QDir::toNativeSeparators(zip), QDir::toNativeSeparators(dest));
    m_current = ProcUtil::run(this, "powershell.exe", {"-NoProfile", "-ExecutionPolicy", "Bypass", "-Command", cmd}, 20 * 60000,
        [this, next](int code, QString out) {
            m_current = nullptr;
            if (code != 0) { fail("Ошибка распаковки: " + out.right(300)); next(false); return; }
            next(true);
        });
}

void SdkInstaller::tool(const QString& bat, const QStringList& args, const QByteArray& in, int timeoutMin, std::function<void(bool, QString)> next) {
    auto* p = new QProcess(this);
    ProcUtil::setCommand(p, toolsBin() + "/" + bat, args);
    auto env = Paths::sdkEnv();
    env.insert("JAVA_HOME", QDir::toNativeSeparators(jreHome()));
    env.insert("PATH", QDir::toNativeSeparators(jreHome() + "/bin") + ";" + env.value("PATH"));
    p->setProcessEnvironment(env);
    p->setProcessChannelMode(QProcess::MergedChannels);
    auto all = std::make_shared<QString>();
    connect(p, &QProcess::readyRead, this, [this, p, all] {
        const QString s = QString::fromLocal8Bit(p->readAll());
        *all += s;
        static const QRegularExpression pct("(\\d{1,3})%");
        auto it = pct.globalMatch(s); int last = -1;
        while (it.hasNext()) last = it.next().captured(1).toInt();
        if (last >= 0) emit progress(last, {});
        for (const QString& l : s.split(QRegularExpression("[\\r\\n]"), Qt::SkipEmptyParts)) emit log(l.trimmed());
    });
    connect(p, &QProcess::finished, this, [this, p, all, next](int code, QProcess::ExitStatus st) {
        m_current = nullptr; p->deleteLater();
        next(st == QProcess::NormalExit && code == 0, *all);
    });
    QTimer::singleShot(timeoutMin * 60000, p, [p] { p->kill(); });
    m_current = p;
    p->start();
    if (!in.isEmpty()) p->write(in);
}

void SdkInstaller::checkAcceleration(std::function<void(bool, QString)> cb) {
    if (!QFile::exists(Paths::emulatorExe())) { cb(false, "Android Emulator ещё не установлен"); return; }
    ProcUtil::run(this, Paths::emulatorExe(), {"-accel-check"}, 30000, [cb](int code, QString out) { cb(code == 0, out.trimmed()); });
}
