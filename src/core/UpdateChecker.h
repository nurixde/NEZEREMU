#pragma once
#include <QObject>
#include <QUrl>
class SettingsStore; class QNetworkAccessManager;

// Проверка обновлений через GitHub Releases репозитория, из которого собран установщик.
class UpdateChecker : public QObject {
    Q_OBJECT
public:
    UpdateChecker(SettingsStore* s, QObject* parent = nullptr);
    void check();
    static int compareVersions(const QString& a, const QString& b);
signals:
    void result(const QString& text, const QUrl& downloadPage);
private:
    SettingsStore* m_s; QNetworkAccessManager* m_net;
};
