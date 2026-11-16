#include "UpdateChecker.h"
#include "settings/SettingsStore.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QStringList>

#ifndef NEZER_REPO
#define NEZER_REPO ""
#endif

UpdateChecker::UpdateChecker(SettingsStore* s, QObject* parent) : QObject(parent), m_s(s), m_net(new QNetworkAccessManager(this)) {}

int UpdateChecker::compareVersions(const QString& a, const QString& b) {
    const QStringList x = QString(a).remove('v').split('.'), y = QString(b).remove('v').split('.');
    for (int i = 0; i < qMax(x.size(), y.size()); ++i) {
        const int p = i < x.size() ? x[i].toInt() : 0, q = i < y.size() ? y[i].toInt() : 0;
        if (p != q) return p < q ? -1 : 1;
    }
    return 0;
}

void UpdateChecker::check() {
    QString repo = m_s->value("update.repo").toString();
    if (repo.isEmpty()) repo = NEZER_REPO;
    if (repo.isEmpty()) { emit result("Репозиторий обновлений не задан (сборка без GitHub).", {}); return; }
    QNetworkRequest req(QUrl("https://api.github.com/repos/" + repo + "/releases/latest"));
    req.setHeader(QNetworkRequest::UserAgentHeader, "NezerEmu");
    QNetworkReply* r = m_net->get(req);
    connect(r, &QNetworkReply::finished, this, [this, r] {
        r->deleteLater();
        if (r->error() != QNetworkReply::NoError) { emit result("Не удалось проверить: " + r->errorString(), {}); return; }
        const QJsonObject o = QJsonDocument::fromJson(r->readAll()).object();
        const QString tag = o.value("tag_name").toString();
        if (tag.isEmpty()) { emit result("Релизов пока нет.", {}); return; }
        if (compareVersions(NEZER_VERSION, tag) < 0) emit result("Доступна новая версия " + tag + ". Установите её поверх текущей: данные сохранятся.", QUrl(o.value("html_url").toString()));
        else emit result("У вас последняя версия (" NEZER_VERSION ").", {});
    });
}
