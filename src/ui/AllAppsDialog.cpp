#include "AllAppsDialog.h"
#include "apps/AppManager.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

AllAppsDialog::AllAppsDialog(AppManager* apps, bool pick, QWidget* parent) : QDialog(parent), m_apps(apps), m_pick(pick) {
    setWindowTitle(pick ? "Добавить в Dock" : "Все приложения"); resize(720, 520);
    auto* v = new QVBoxLayout(this);
    auto* hint = new QLabel(pick ? "Выберите приложение: оно появится в Dock. Показаны только реально установленные приложения Android."
                                 : "Приложения, установленные в Android. Двойной щелчок: запуск.");
    hint->setWordWrap(true); hint->setStyleSheet("color:#a59ec4;");
    m_search = new QLineEdit; m_search->setPlaceholderText("Поиск");
    m_list = new QListWidget; m_list->setViewMode(QListView::IconMode); m_list->setIconSize(QSize(56, 56));
    m_list->setGridSize(QSize(110, 104)); m_list->setResizeMode(QListView::Adjust); m_list->setMovement(QListView::Static); m_list->setWordWrap(true);
    auto* h = new QHBoxLayout;
    auto* apk = new QPushButton("Установить APK…"); auto* pin = new QPushButton(m_pick ? "Добавить" : "В Dock / убрать"); pin->setObjectName("primary");
    auto* close = new QPushButton("Закрыть");
    h->addWidget(apk); h->addStretch(); h->addWidget(pin); h->addWidget(close);
    v->addWidget(hint); v->addWidget(m_search); v->addWidget(m_list, 1); v->addLayout(h);
    connect(m_search, &QLineEdit::textChanged, this, &AllAppsDialog::fill);
    connect(close, &QPushButton::clicked, this, &QDialog::reject);
    connect(apk, &QPushButton::clicked, this, [this] { emit installApkRequested(); accept(); });
    connect(pin, &QPushButton::clicked, this, [this] {
        auto* it = m_list->currentItem(); if (!it) return;
        const QString p = it->data(Qt::UserRole).toString();
        if (m_pick) { m_apps->pin(p); accept(); return; }
        if (m_apps->isPinned(p)) m_apps->unpin(p); else m_apps->pin(p);
        fill();
    });
    connect(m_list, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* it) {
        const QString p = it->data(Qt::UserRole).toString();
        if (m_pick) { m_apps->pin(p); accept(); } else { emit launchRequested(p); accept(); }
    });
    fill();
}

void AllAppsDialog::fill() {
    m_list->clear();
    const QString q = m_search->text().trimmed();
    const auto all = m_apps->installed();
    for (const AppEntry& a : all) {
        if (!q.isEmpty() && !a.label.contains(q, Qt::CaseInsensitive) && !a.package.contains(q, Qt::CaseInsensitive)) continue;
        auto* it = new QListWidgetItem(a.icon, a.label + (m_apps->isPinned(a.package) ? "  ★" : ""));
        it->setData(Qt::UserRole, a.package); it->setToolTip(a.package);
        m_list->addItem(it);
    }
    if (all.isEmpty()) m_list->addItem(new QListWidgetItem("Нет данных: Android не запущен"));
}
