#include "SettingsStore.h"
#include "core/Paths.h"
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSaveFile>
#include <QThread>

SettingsStore::SettingsStore(const QString& path) : m_path(path) {}
QString SettingsStore::defaultPath() { return Paths::dataDir() + "/settings.json"; }

const QJsonObject& SettingsStore::defaults() {
    static const QJsonObject d = [] {
        const int n = qMax(2, QThread::idealThreadCount());
        QJsonObject o;
        o["general.showLock"] = true; o["general.rememberSize"] = true; o["general.autostart"] = false;
        o["perf.mode"] = "performance"; o["perf.cores"] = qBound(2, n - 2, 8); o["perf.ramMb"] = 6144;
        o["perf.fps"] = 60; o["perf.highPriority"] = true; o["perf.showFps"] = true;
        o["screen.width"] = 1920; o["screen.height"] = 1080; o["screen.dpi"] = 320; o["screen.orientation"] = "landscape";
        o["gfx.mode"] = "host";
        o["input.sensX"] = 1.0; o["input.sensY"] = 1.0; o["input.adsMul"] = 0.6; o["input.dpi"] = 800;
        o["input.smoothing"] = false; o["input.smoothStrength"] = 30; o["input.captureKey"] = "`"; o["input.profile"] = "Default";
        o["android.image"] = "google_apis_playstore"; o["android.api"] = 34; o["android.coldBoot"] = false; o["android.wipeNext"] = false;
        o["appearance.mainWallpaper"] = ""; o["appearance.lockWallpaper"] = ""; o["appearance.accent"] = "#8b5cf6"; o["appearance.dockOpacity"] = 88;
        o["update.repo"] = "";
        return o;
    }();
    return d;
}

bool SettingsStore::load() {
    QFile f(m_path);
    if (!f.open(QIODevice::ReadOnly)) return false;
    QJsonParseError err;
    const auto doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return false;
    m_obj = doc.object();
    return true;
}

bool SettingsStore::save() const {
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    QSaveFile f(m_path);
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(QJsonDocument(m_obj).toJson(QJsonDocument::Indented));
    return f.commit();
}

QVariant SettingsStore::value(const QString& key, const QVariant& def) const {
    if (m_obj.contains(key)) return m_obj.value(key).toVariant();
    if (def.isValid()) return def;
    return defaults().value(key).toVariant();
}

void SettingsStore::setValue(const QString& key, const QVariant& v) { m_obj.insert(key, QJsonValue::fromVariant(v)); }

void SettingsStore::resetToDefaults(const QStringList& keep) {
    for (const QString& k : m_obj.keys()) {
        bool kept = false;
        for (const QString& p : keep) if (k.startsWith(p)) kept = true;
        if (!kept) m_obj.remove(k);
    }
}
