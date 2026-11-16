#include "SetupWizard.h"
#include "core/SystemCheck.h"
#include "engine/AndroidEngine.h"
#include "engine/SdkInstaller.h"
#include "settings/SettingsStore.h"
#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

static QLabel* title(const QString& t) { auto* l = new QLabel(t); l->setStyleSheet("font-size:20px;font-weight:600;"); return l; }
static QLabel* text(const QString& t) { auto* l = new QLabel(t); l->setWordWrap(true); l->setOpenExternalLinks(true); l->setStyleSheet("color:#cfc8ea;"); return l; }

SetupWizard::SetupWizard(SettingsStore* s, QWidget* parent) : QDialog(parent), m_s(s), m_inst(new SdkInstaller(this)) {
    setWindowTitle("NEZER EMU — первоначальная настройка Android"); resize(720, 540);
    m_pages = new QStackedWidget;
    m_pages->addWidget(pageCheck()); m_pages->addWidget(pageLicense()); m_pages->addWidget(pageInstall()); m_pages->addWidget(pageDone());
    auto* v = new QVBoxLayout(this); v->setContentsMargins(26, 22, 26, 22); v->addWidget(m_pages);
    connect(m_inst, &SdkInstaller::progress, this, [this](int p, const QString& st) {
        if (!st.isEmpty()) m_stage->setText(st);
        if (p < 0) m_bar->setRange(0, 0); else { m_bar->setRange(0, 100); m_bar->setValue(p); }
    });
    connect(m_inst, &SdkInstaller::log, this, [this](const QString& l) { if (!l.isEmpty()) m_log->appendPlainText(l); });
    connect(m_inst, &SdkInstaller::finished, this, [this](bool ok, const QString& msg) {
        if (!ok) {
            m_stage->setText("Ошибка: " + msg); m_stage->setStyleSheet("color:#ff8a8a;"); m_log->appendPlainText(msg);
            m_cancel->setText("Повторить"); m_bar->setRange(0, 100); m_bar->setValue(0);
            return;
        }
        m_pages->setCurrentIndex(3);
        m_inst->checkAcceleration([this](bool accel, QString out) {
            m_doneText->setText(accel ? "Android установлен. Аппаратное ускорение работает:\n" + out
                                      : "Android установлен, но аппаратное ускорение НЕДОСТУПНО:\n" + out +
                                        "\n\nВключите: Панель управления → Программы → Включение компонентов Windows → «Платформа гипервизора Windows», затем перезагрузите ПК.");
        });
    });
}

QWidget* SetupWizard::pageCheck() {
    auto* w = new QWidget; auto* v = new QVBoxLayout(w);
    v->addWidget(title("Проверка системы"));
    v->addWidget(text("NEZER EMU запускает настоящий Android через официальный Android Emulator. Ниже — фактические параметры вашего ПК."));
    m_checkList = new QVBoxLayout; v->addLayout(m_checkList); v->addStretch();
    bool blocked = false;
    for (const CheckResult& r : SystemCheck::run()) {
        auto* l = new QLabel(QString("<b style='color:%1'>%2</b>&nbsp;&nbsp;%3<br><span style='color:#a59ec4'>%4</span>")
                                 .arg(r.ok ? "#4ade80" : (r.critical ? "#f87171" : "#fbbf24"), r.ok ? "✓" : (r.critical ? "✕" : "!"), r.name.toHtmlEscaped(), r.detail.toHtmlEscaped()));
        m_checkList->addWidget(l);
        if (!r.ok && r.critical) blocked = true;
    }
    auto* h = new QHBoxLayout; m_checkNext = new QPushButton(blocked ? "Продолжить всё равно" : "Далее"); m_checkNext->setObjectName("primary");
    auto* later = new QPushButton("Позже");
    h->addStretch(); h->addWidget(later); h->addWidget(m_checkNext); v->addLayout(h);
    if (blocked) v->insertWidget(v->count() - 1, text("<span style='color:#f87171'>Есть критические проблемы: Android, скорее всего, не запустится, пока их не исправить.</span>"));
    connect(later, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_checkNext, &QPushButton::clicked, this, [this] { m_pages->setCurrentIndex(1); });
    return w;
}

QWidget* SetupWizard::pageLicense() {
    auto* w = new QWidget; auto* v = new QVBoxLayout(w);
    v->addWidget(title("Компоненты и лицензии"));
    v->addWidget(text("С официальных серверов будут скачаны (около 4 ГБ):<br>"
                      "• Eclipse Temurin JRE 17 (adoptium.net) — нужен для sdkmanager;<br>"
                      "• Android SDK Command-line Tools, Platform-Tools, Android Emulator (Google);<br>"
                      "• системный образ Android 14 x86_64 (Google).<br><br>"
                      "NEZER EMU не распространяет эти компоненты и Google Play: они загружаются напрямую от правообладателей. "
                      "Условия: <a href='https://developer.android.com/studio/terms' style='color:#c4a7ff'>Android SDK License Agreement</a>, "
                      "для образа с Google Play — условия Google."));
    v->addSpacing(10);
    m_image = new QComboBox;
    m_image->addItem("Android 14 с Google Play (рекомендуется)", "google_apis_playstore");
    m_image->addItem("Android 14 с Google APIs (без Play Маркета)", "google_apis");
    v->addWidget(m_image);
    m_accept = new QCheckBox("Я прочитал(а) и принимаю лицензионные условия Android SDK");
    v->addWidget(m_accept); v->addStretch();
    auto* h = new QHBoxLayout; auto* back = new QPushButton("Назад"); auto* go = new QPushButton("Принять и установить");
    go->setObjectName("primary"); go->setEnabled(false);
    h->addStretch(); h->addWidget(back); h->addWidget(go); v->addLayout(h);
    connect(m_accept, &QCheckBox::toggled, go, &QPushButton::setEnabled);
    connect(back, &QPushButton::clicked, this, [this] { m_pages->setCurrentIndex(0); });
    connect(go, &QPushButton::clicked, this, &SetupWizard::startInstall);
    return w;
}

QWidget* SetupWizard::pageInstall() {
    auto* w = new QWidget; auto* v = new QVBoxLayout(w);
    v->addWidget(title("Установка Android"));
    m_stage = new QLabel("Подготовка…"); m_bar = new QProgressBar; m_log = new QPlainTextEdit; m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(3000);
    v->addWidget(m_stage); v->addWidget(m_bar); v->addWidget(m_log, 1);
    auto* h = new QHBoxLayout; m_cancel = new QPushButton("Отмена"); h->addStretch(); h->addWidget(m_cancel); v->addLayout(h);
    connect(m_cancel, &QPushButton::clicked, this, [this] {
        if (m_cancel->text() == "Повторить") { m_stage->setStyleSheet({}); m_cancel->setText("Отмена"); startInstall(); }
        else { m_inst->cancel(); reject(); }
    });
    return w;
}

QWidget* SetupWizard::pageDone() {
    auto* w = new QWidget; auto* v = new QVBoxLayout(w);
    v->addWidget(title("Готово"));
    m_doneText = text("Проверка аппаратного ускорения…"); m_doneText->setTextInteractionFlags(Qt::TextSelectableByMouse);
    v->addWidget(m_doneText); v->addStretch();
    auto* h = new QHBoxLayout; auto* ok = new QPushButton("Запустить Android"); ok->setObjectName("primary");
    h->addStretch(); h->addWidget(ok); v->addLayout(h);
    connect(ok, &QPushButton::clicked, this, &QDialog::accept);
    return w;
}

void SetupWizard::startInstall() {
    m_pages->setCurrentIndex(2);
    const QString flavor = m_image->currentData().toString();
    m_s->setValue("android.image", flavor); m_s->save();
    AndroidEngine tmp(m_s);
    m_inst->run(tmp.params(), m_s->value("android.api").toInt(), flavor);
}
