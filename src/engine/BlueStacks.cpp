#include "BlueStacks.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#ifdef Q_OS_WIN
#include <windows.h>
#include <tlhelp32.h>
#include <wchar.h>
#endif

namespace BlueStacks {

static bool fill(Install& r, QString inst, QString data) {
    inst = QDir::cleanPath(QDir::fromNativeSeparators(inst.trimmed()));
    data = QDir::cleanPath(QDir::fromNativeSeparators(data.trimmed()));
    const QString player = inst + "/HD-Player.exe";
    if (inst.isEmpty() || !QFileInfo::exists(player)) return false;
    r.found = true; r.installDir = inst; r.dataDir = data; r.player = player;
    r.adb = inst + "/HD-Adb.exe"; r.multiInstance = inst + "/HD-MultiInstanceManager.exe";
    r.conf = data.isEmpty() ? QString() : data + "/bluestacks.conf";
    return true;
}

Install detect() {
    Install r;
    for (const char* key : {"BlueStacks_nxt", "BlueStacks_msi5"}) {
        QSettings reg(QString("HKEY_LOCAL_MACHINE\\SOFTWARE\\") + key, QSettings::Registry64Format);
        if (fill(r, reg.value("InstallDir").toString(), reg.value("UserDefinedDir").toString())) return r;
    }
    fill(r, "C:/Program Files/BlueStacks_nxt", "C:/ProgramData/BlueStacks_nxt");
    return r;
}

bool isInstalled() { return detect().found; }

QString adbExe() {
    const Install i = detect();
    return i.found && QFileInfo::exists(i.adb) ? i.adb : QString("adb");
}

static QStringList confLines(const QString& path, bool* crlf) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return {};
    const QByteArray raw = f.readAll();
    if (crlf) *crlf = raw.contains("\r\n");
    QStringList lines = QString::fromUtf8(raw).split('\n');
    for (QString& l : lines) if (l.endsWith('\r')) l.chop(1);
    return lines;
}

QMap<QString, QString> readConf() {
    QMap<QString, QString> m;
    const Install i = detect();
    if (!i.found || i.conf.isEmpty()) return m;
    for (const QString& line : confLines(i.conf, nullptr)) {
        const int eq = line.indexOf('=');
        if (eq <= 0) continue;
        QString v = line.mid(eq + 1).trimmed();
        if (v.size() >= 2 && v.startsWith('"') && v.endsWith('"')) v = v.mid(1, v.size() - 2);
        m.insert(line.left(eq).trimmed(), v);
    }
    return m;
}

QStringList instances() {
    QStringList out;
    const QMap<QString, QString> m = readConf();
    for (auto it = m.cbegin(); it != m.cend(); ++it) {
        if (!it.key().startsWith("bst.instance.")) continue;
        const QString name = it.key().section('.', 2, 2);
        if (!name.isEmpty() && !out.contains(name)) out << name;
    }
    out.sort();
    if (out.removeAll("Pie64") > 0) out.prepend("Pie64");
    return out;
}

QString defaultInstance() {
    const QStringList l = instances();
    return l.isEmpty() ? QString("Pie64") : l.first();
}

int adbPort(const QString& inst) {
    const QMap<QString, QString> m = readConf();
    const QStringList keys{"bst.instance." + inst + ".status.adb_port", "bst.instance." + inst + ".adb_port"};
    for (const QString& k : keys) {
        const int p = m.value(k).toInt();
        if (p > 0) return p;
    }
    return 5555;
}

bool writeConf(const QMap<QString, QString>& changes, QString* error) {
    auto fail = [error](const QString& e) { if (error) *error = e; return false; };
    const Install i = detect();
    if (!i.found || i.conf.isEmpty() || !QFileInfo::exists(i.conf))
        return fail("файл bluestacks.conf не найден (запустите BlueStacks хотя бы один раз)");
    bool crlf = false;
    QStringList lines = confLines(i.conf, &crlf);
    if (lines.isEmpty()) return fail("не удалось прочитать bluestacks.conf");
    QMap<QString, QString> left = changes;
    for (QString& l : lines) {
        const int eq = l.indexOf('=');
        if (eq <= 0) continue;
        const QString k = l.left(eq).trimmed();
        auto it = left.find(k);
        if (it == left.end()) continue;
        l = k + "=\"" + it.value() + "\"";
        left.erase(it);
    }
    while (!lines.isEmpty() && lines.last().trimmed().isEmpty()) lines.removeLast();
    for (auto it = left.cbegin(); it != left.cend(); ++it) lines << it.key() + "=\"" + it.value() + "\"";
    const QString bak = i.conf + ".nezer.bak";   // одна резервная копия оригинала
    if (!QFileInfo::exists(bak)) QFile::copy(i.conf, bak);
    QFile f(i.conf);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return fail("нет прав на запись в " + QDir::toNativeSeparators(i.conf));
    const QString nl = crlf ? "\r\n" : "\n";
    f.write((lines.join(nl) + nl).toUtf8());
    return true;
}

QList<quint32> playerPids() {
    QList<quint32> r;
#ifdef Q_OS_WIN
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return r;
    PROCESSENTRY32W e{};
    e.dwSize = sizeof(e);
    if (Process32FirstW(snap, &e)) {
        do { if (_wcsicmp(e.szExeFile, L"HD-Player.exe") == 0) r << quint32(e.th32ProcessID); } while (Process32NextW(snap, &e));
    }
    CloseHandle(snap);
#endif
    return r;
}

}
