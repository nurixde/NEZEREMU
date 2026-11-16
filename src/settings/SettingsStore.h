#pragma once
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVariant>

// Настройки в JSON: %LOCALAPPDATA%/NezerEmu/settings.json. Атомарная запись.
class SettingsStore {
public:
    explicit SettingsStore(const QString& path = defaultPath());
    static QString defaultPath();
    static const QJsonObject& defaults();
    bool load();
    bool save() const;
    QVariant value(const QString& key, const QVariant& def = {}) const;
    void setValue(const QString& key, const QVariant& v);
    bool contains(const QString& key) const { return m_obj.contains(key); }
    void resetToDefaults(const QStringList& keepPrefixes);
private:
    QString m_path;
    QJsonObject m_obj;
};
