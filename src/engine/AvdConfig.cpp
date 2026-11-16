#include "AvdConfig.h"
#include "core/Paths.h"
#include <QFile>
#include <QSaveFile>
#include <QStringList>

namespace AvdConfig {
QString avdDir() { return Paths::avdHome() + "/" + kAvdName + ".avd"; }
QString configPath() { return avdDir() + "/config.ini"; }
bool exists() { return QFile::exists(configPath()); }

QMap<QString, QString> parse(const QString& text) {
    QMap<QString, QString> m;
    for (const QString& raw : text.split('\n')) {
        const QString line = raw.trimmed();
        const int eq = line.indexOf('=');
        if (line.isEmpty() || line.startsWith('#') || eq < 1) continue;
        m.insert(line.left(eq).trimmed(), line.mid(eq + 1).trimmed());
    }
    return m;
}

QString apply(const QString& text, const QMap<QString, QString>& values) {
    QStringList out; QMap<QString, QString> left = values;
    for (const QString& raw : text.split('\n')) {
        QString line = raw; line.remove('\r');
        const int eq = line.indexOf('=');
        if (eq > 0) {
            const QString k = line.left(eq).trimmed();
            if (left.contains(k)) { line = k + "=" + left.take(k); }
        }
        if (!line.isEmpty()) out << line;
    }
    for (auto it = left.cbegin(); it != left.cend(); ++it) out << it.key() + "=" + it.value();
    return out.join('\n') + '\n';
}

QMap<QString, QString> toConfig(const AvdParams& p) {
    const int w = p.landscape ? qMax(p.width, p.height) : qMin(p.width, p.height);
    const int h = p.landscape ? qMin(p.width, p.height) : qMax(p.width, p.height);
    return {
        {"hw.cpu.ncore", QString::number(p.cores)}, {"hw.ramSize", QString::number(p.ramMb)},
        {"hw.lcd.width", QString::number(w)}, {"hw.lcd.height", QString::number(h)},
        {"hw.lcd.density", QString::number(p.dpi)}, {"hw.lcd.vsync", QString::number(p.refreshHz)},
        {"hw.gpu.enabled", "yes"}, {"hw.gpu.mode", p.gpuMode}, {"hw.keyboard", "yes"},
        {"hw.audioOutput", "yes"}, {"hw.initialOrientation", p.landscape ? "landscape" : "portrait"},
        {"showDeviceFrame", "no"}, {"skin.dynamic", "yes"}, {"skin.name", QString("%1x%2").arg(w).arg(h)},
        {"skin.path", "_no_skin"}, {"disk.dataPartition.size", "16G"}, {"vm.heapSize", "512"},
        {"PlayStore.enabled", "true"}};
}

bool write(const AvdParams& p, QString* error) {
    QFile in(configPath());
    if (!in.open(QIODevice::ReadOnly)) { if (error) *error = "Виртуальное устройство не создано"; return false; }
    const QString text = QString::fromUtf8(in.readAll()); in.close();
    QSaveFile out(configPath());
    if (!out.open(QIODevice::WriteOnly)) { if (error) *error = "Нет доступа к config.ini"; return false; }
    out.write(apply(text, toConfig(p)).toUtf8());
    return out.commit();
}
}
