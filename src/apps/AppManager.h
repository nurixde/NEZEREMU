#pragma once
#include <QColor>
#include <QIcon>
#include <QObject>
#include <QStringList>
class SettingsStore;

struct AppEntry { QString package; QString label; QIcon icon; };

// Реально установленные приложения и ярлыки Dock.
class AppManager : public QObject {
    Q_OBJECT
public:
    explicit AppManager(SettingsStore* s, QObject* parent = nullptr);
    void setInstalled(const QStringList& packages);
    QList<AppEntry> installed() const;
    QList<AppEntry> dockApps() const;          // только установленные и закреплённые
    bool isPinned(const QString& pkg) const;
    void pin(const QString& pkg);
    void unpin(const QString& pkg);
    static QIcon iconFor(const QString& pkg, const QString& label);
    static QStringList autoPinCandidates();
signals:
    void dockChanged();
private:
    AppEntry entry(const QString& pkg) const;
    SettingsStore* m_s; QStringList m_installed;
};
