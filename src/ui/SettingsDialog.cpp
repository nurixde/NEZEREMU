#include "SettingsDialog.h"
#include "Icons.h"
#include "Theme.h"
#include "Widgets.h"
#include "core/WinUtil.h"
#include "engine/AndroidEngine.h"
#include "input/GameInput.h"
#include "settings/SettingsStore.h"
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QStackedWidget>
#include <QThread>
#include <QVBoxLayout>
#include "core/SystemCheck.h"

void SettingsDialog::set(const QString& k, const QVariant& v) { m_s->setValue(k, v); m_s->save(); }

SettingsDialog::SettingsDialog(SettingsStore* s, AndroidEngine* e, GameInput* g, QWidget* parent)
    : QDialog(parent), m_s(s), m_e(e), m_g(g) {
    setWindowTitle("NEZER EMU — Настройки"); resize(1080, 700); setMinimumSize(900, 560);
    ToggleSwitch::accent = Theme::accent(m_s->value("appearance.accent").toString());
    auto* root = new QHBoxLayout(this); root->setContentsMargins(0, 0, 0, 0); root->setSpacing(0);
    auto* side = new QWidget; side->setFixedWidth(250); side->setStyleSheet(QString("background:%1;").arg("#120c25"));
    auto* sv = new QVBoxLayout(side); sv->setContentsMargins(10, 18, 10, 14);
    auto* head = new QLabel("  Настройки"); head->setStyleSheet("font-size:18px;font-weight:700;padding:4px 0 12px 4px;");
    m_menu = new QListWidget; m_menu->setIconSize(QSize(18, 18));
    const QList<QPair<QString, QString>> sections = {{"general", "Основные"}, {"chip", "Производительность"}, {"monitor", "Экран и разрешение"},
        {"graphics", "Графика"}, {"mouse", "Управление и чувствительность"}, {"android", "Система Android"},
        {"brush", "Оформление"}, {"extra", "Дополнительно"}};
    for (const auto& sc : sections) m_menu->addItem(new QListWidgetItem(Icons::get(sc.first, QColor("#d9d2f5")), sc.second));
    sv->addWidget(head); sv->addWidget(m_menu, 1);
    auto* ver = new QLabel("  Версия " NEZER_VERSION); ver->setStyleSheet("color:#a59ec4;font-size:11px;"); sv->addWidget(ver);
    m_pages = new QStackedWidget;
    m_pages->addWidget(pageGeneral()); m_pages->addWidget(pagePerf()); m_pages->addWidget(pageScreen()); m_pages->addWidget(pageGraphics());
    m_pages->addWidget(pageInput()); m_pages->addWidget(pageAndroid()); m_pages->addWidget(pageAppearance()); m_pages->addWidget(pageExtra());
    root->addWidget(side); root->addWidget(m_pages, 1);
    connect(m_menu, &QListWidget::currentRowChanged, m_pages, &QStackedWidget::setCurrentIndex);
    m_menu->setCurrentRow(0);
    startTimer(500);
}

void SettingsDialog::openSection(int i) { m_menu->setCurrentRow(i); }

QWidget* SettingsDialog::page(const QList<Card*>& cards) {
    auto* area = new QScrollArea; area->setWidgetResizable(true);
    auto* w = new QWidget; auto* grid = new QHBoxLayout(w); grid->setContentsMargins(22, 22, 22, 22); grid->setSpacing(16);
    auto* c1 = new QVBoxLayout; auto* c2 = new QVBoxLayout; c1->setSpacing(16); c2->setSpacing(16);
    for (int i = 0; i < cards.size(); ++i) (i % 2 ? c2 : c1)->addWidget(cards[i]);
    c1->addStretch(); c2->addStretch();
    grid->addLayout(c1, 1); grid->addLayout(c2, 1);
    area->setWidget(w);
    return area;
}

static ToggleSwitch* toggle(bool on) { auto* t = new ToggleSwitch; t->setChecked(on); return t; }

QWidget* SettingsDialog::pageGeneral() {
    auto* c = new Card("general", "Основные");
    auto* autostart = toggle(m_s->value("general.autostart").toBool());
    c->addRow("Автозапуск эмулятора", "Запускать при включении Windows", autostart);
    connect(autostart, &QAbstractButton::toggled, this, [this](bool on) { set("general.autostart", on); WinUtil::setAutostart(on); });
    auto* rem = toggle(m_s->value("general.rememberSize").toBool());
    c->addRow("Запоминать размер окна", "Восстанавливать последний размер", rem);
    connect(rem, &QAbstractButton::toggled, this, [this](bool on) { set("general.rememberSize", on); });
    auto* autoAndroid = toggle(m_s->value("general.autoAndroid", true).toBool());
    c->addRow("Запускать Android сразу", "Android стартует, пока показан экран блокировки", autoAndroid);
    connect(autoAndroid, &QAbstractButton::toggled, this, [this](bool on) { set("general.autoAndroid", on); });
    auto* lang = new QComboBox; lang->addItem("Русский"); lang->setEnabled(false); lang->setToolTip("Другие языки пока не переведены");
    c->addRow("Язык интерфейса", {}, lang);
    auto* keys = new Card("keyboard", "Горячие клавиши");
    keys->addNote("F11 — полный экран\n` (ё) — захват/освобождение курсора в игровом режиме\nEsc — освободить курсор\nCtrl+H — рабочий стол NEZER EMU");
    return page({c, keys});
}

QWidget* SettingsDialog::pagePerf() {
    auto* c = new Card("chip", "Производительность");
    auto* mode = new Segmented({"Баланс", "Производительность", "Максимум"});
    const QStringList modes{"balance", "performance", "max"};
    mode->setCurrent(qMax(0, modes.indexOf(m_s->value("perf.mode").toString())));
    c->addRow("Режим работы", {}, nullptr); c->body()->addWidget(mode);
    m_cores = new QComboBox; const int n = QThread::idealThreadCount();
    for (int i = 1; i <= n; ++i) m_cores->addItem(QString("%1 ядер").arg(i), i);
    m_cores->setCurrentIndex(qBound(0, m_s->value("perf.cores").toInt() - 1, n - 1));
    c->addRow("Количество ядер CPU", QString("Доступно на ПК: %1").arg(n), m_cores);
    m_ram = new QComboBox; const quint64 total = SystemCheck::totalRamMb();
    for (int gb : {2, 3, 4, 6, 8, 12, 16}) if (total == 0 || quint64(gb) * 1024 <= total * 3 / 4) m_ram->addItem(QString("%1 ГБ").arg(gb), gb * 1024);
    m_ram->setCurrentIndex(qMax(0, m_ram->findData(m_s->value("perf.ramMb").toInt())));
    c->addRow("Объём RAM", total ? QString("В ПК: %1 ГБ (максимум 75%)").arg(total / 1024) : QString(), m_ram);
    connect(m_cores, &QComboBox::activated, this, [this] { set("perf.cores", m_cores->currentData()); m_restartNote->show(); });
    connect(m_ram, &QComboBox::activated, this, [this] { set("perf.ramMb", m_ram->currentData()); m_restartNote->show(); });
    connect(mode, &Segmented::changed, this, [this, modes](int i) {
        int cores, ram, prio; AndroidEngine::modeDefaults(modes[i], &cores, &ram, &prio);
        set("perf.mode", modes[i]); set("perf.cores", cores);
        m_cores->setCurrentIndex(cores - 1);
        const int ri = m_ram->findData(ram); m_ram->setCurrentIndex(ri >= 0 ? ri : m_ram->count() - 1);
        set("perf.ramMb", m_ram->currentData()); m_restartNote->show();
    });
    auto* prio = toggle(m_s->value("perf.highPriority").toBool());
    c->addRow("Высокий приоритет процесса", "Меньше рывков при фоновой нагрузке", prio);
    connect(prio, &QAbstractButton::toggled, this, [this](bool on) { set("perf.highPriority", on); });
    m_restartNote = c->addNote("Изменения CPU и RAM применятся после перезапуска Android."); m_restartNote->hide();
    auto* restart = new QPushButton("Перезапустить Android"); c->body()->addWidget(restart);
    connect(restart, &QPushButton::clicked, m_e, &AndroidEngine::restart);

    auto* f = new Card("monitor", "Частота кадров");
    auto* fps = new QComboBox;
    for (int v : {30, 60, 90, 120, 144, 165, 240}) fps->addItem(QString("%1 FPS").arg(v), v);
    fps->addItem("Без ограничения", 0);
    fps->setCurrentIndex(qMax(0, fps->findData(m_s->value("perf.fps").toInt())));
    f->addRow("Лимит FPS", "Частота дисплея Android. Итоговый FPS также зависит от самой игры и видеокарты", fps);
    connect(fps, &QComboBox::activated, this, [this, fps] { set("perf.fps", fps->currentData()); m_e->applyFrameRate(); m_restartNote->show(); });
    auto* show = toggle(m_s->value("perf.showFps").toBool());
    f->addRow("Показывать реальный FPS", "Измеряется по счётчику кадров Android, в верхней панели", show);
    connect(show, &QAbstractButton::toggled, this, [this](bool on) { set("perf.showFps", on); emit appearanceChanged(); });
    f->addNote("«Без ограничения» выставляет максимальную частоту дисплея, которую сообщает эмулятору NEZER EMU (240). Настоящего «безлимита» в Android Emulator нет: число в панели FPS покажет реальный результат.");
    return page({c, f});
}

QWidget* SettingsDialog::pageScreen() {
    auto* c = new Card("monitor", "Экран и разрешение");
    auto* preset = new QComboBox;
    const QList<QPair<QString, QSize>> list = {{"4:3", {1280, 960}}, {"4:3", {1440, 1080}}, {"16:10", {1680, 1050}}, {"16:10", {1920, 1200}},
        {"16:9", {1280, 720}}, {"16:9", {1600, 900}}, {"16:9", {1920, 1080}}, {"16:9", {2560, 1440}}, {"21:9", {2560, 1080}}, {"21:9", {3440, 1440}}};
    for (const auto& p : list) preset->addItem(QString("%1 × %2  (%3)").arg(p.second.width()).arg(p.second.height()).arg(p.first), p.second);
    preset->addItem("Пользовательское…", QSize());
    const QSize cur(m_s->value("screen.width").toInt(), m_s->value("screen.height").toInt());
    const int idx = preset->findData(cur); preset->setCurrentIndex(idx >= 0 ? idx : preset->count() - 1);
    c->addRow("Разрешение", {}, preset);
    auto* w = new QSpinBox; w->setRange(480, 7680); w->setValue(cur.width());
    auto* h = new QSpinBox; h->setRange(320, 4320); h->setValue(cur.height());
    auto* dpi = new QSpinBox; dpi->setRange(120, 640); dpi->setValue(m_s->value("screen.dpi").toInt());
    c->addRow("Ширина", "пикселей", w); c->addRow("Высота", "пикселей", h); c->addRow("DPI (плотность)", "Размер элементов интерфейса Android", dpi);
    connect(preset, &QComboBox::activated, this, [preset, w, h] { const QSize s = preset->currentData().toSize(); if (s.isValid()) { w->setValue(s.width()); h->setValue(s.height()); } });
    auto* orient = new Segmented({"Альбомная", "Портретная"});
    orient->setCurrent(m_s->value("screen.orientation").toString() == "portrait" ? 1 : 0);
    c->addRow("Ориентация экрана", {}, nullptr); c->body()->addWidget(orient);
    auto* apply = new QPushButton("Применить"); apply->setObjectName("primary"); c->body()->addWidget(apply);
    auto* note = c->addNote("Разрешение передаётся в Android (конфигурация устройства + wm size/density), а не просто растягивает картинку. Размер дисплея устройства полностью применяется после перезапуска Android.");
    connect(apply, &QPushButton::clicked, this, [this, w, h, dpi, orient, note] {
        set("screen.width", w->value()); set("screen.height", h->value()); set("screen.dpi", dpi->value());
        set("screen.orientation", orient->current() == 1 ? "portrait" : "landscape");
        m_e->applyLiveDisplay(); emit inputChanged();
        note->setText(note->text().section("\n", 0, 0) + "\nСохранено: " + QString("%1×%2, %3 dpi").arg(w->value()).arg(h->value()).arg(dpi->value()));
        if (m_e->state() == AndroidEngine::State::Ready &&
            QMessageBox::question(this, "NEZER EMU", "Перезапустить Android сейчас, чтобы полностью применить разрешение?") == QMessageBox::Yes) m_e->restart();
    });
    return page({c});
}

QWidget* SettingsDialog::pageGraphics() {
    auto* c = new Card("graphics", "Графика");
    auto* r = new QComboBox;
    r->addItem("Аппаратный (видеокарта ПК)", "host"); r->addItem("Авто", "auto");
    r->addItem("ANGLE (DirectX)", "angle_indirect"); r->addItem("Программный (SwiftShader, медленно)", "swiftshader_indirect");
    r->setCurrentIndex(qMax(0, r->findData(m_s->value("gfx.mode").toString())));
    c->addRow("Рендеринг", "Режим GPU Android Emulator", r);
    connect(r, &QComboBox::activated, this, [this, r] { set("gfx.mode", r->currentData()); });
    c->addNote("Аппаратный режим даёт максимальный FPS. Если есть чёрный экран или вылеты, обновите драйвер видеокарты или попробуйте ANGLE. Применяется после перезапуска Android.");
    auto* restart = new QPushButton("Перезапустить Android"); c->body()->addWidget(restart);
    connect(restart, &QPushButton::clicked, m_e, &AndroidEngine::restart);
    return page({c});
}

QWidget* SettingsDialog::pageAndroid() {
    auto* c = new Card("android", "Система Android");
    m_androidState = c->addNote({});
    c->addNote(QString("Образ: Android %1, %2, x86_64 (с трансляцией ARM-приложений)").arg(m_s->value("android.api").toInt() == 34 ? "14 (API 34)" : m_s->value("android.api").toString(), m_s->value("android.image").toString()));
    auto* row = new QHBoxLayout;
    auto* start = new QPushButton("Запустить"); auto* stop = new QPushButton("Остановить"); auto* rs = new QPushButton("Перезапустить");
    row->addWidget(start); row->addWidget(stop); row->addWidget(rs); c->body()->addLayout(row);
    connect(start, &QPushButton::clicked, m_e, &AndroidEngine::start); connect(stop, &QPushButton::clicked, m_e, &AndroidEngine::stop); connect(rs, &QPushButton::clicked, m_e, &AndroidEngine::restart);
    auto* cold = toggle(m_s->value("android.coldBoot").toBool());
    c->addRow("Холодный запуск", "Не использовать быстрый снимок (дольше, но надёжнее)", cold);
    connect(cold, &QAbstractButton::toggled, this, [this](bool on) { set("android.coldBoot", on); });
    auto* reinstall = new QPushButton("Проверка системы и переустановка компонентов…");
    connect(reinstall, &QPushButton::clicked, this, [this] { emit setupRequested(); });
    auto* wipe = new QPushButton("Сбросить данные Android…");
    connect(wipe, &QPushButton::clicked, this, [this] {
        if (QMessageBox::warning(this, "NEZER EMU", "Все приложения и данные внутри Android будут удалены при следующем запуске. Продолжить?",
                                 QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) { set("android.wipeNext", true); m_e->restart(); }
    });
    c->body()->addWidget(reinstall); c->body()->addWidget(wipe);
    c->addNote("Приложения и данные Android сохраняются между запусками и при обновлении NEZER EMU (%LOCALAPPDATA%\\NezerEmu\\avd).");
    auto* compat = new Card("general", "Совместимость игр");
    compat->addNote("Перед установкой APK NEZER EMU проверяет архитектуры нативных библиотек (ARM/x86) и объясняет ошибки установки.\n\n"
                    "Важно: часть игр (например, с античитом) может обнаружить эмулятор и отказаться запускаться или перенаправить в особые лобби. Это решает разработчик игры, и NEZER EMU не обходит такие защиты.");
    return page({c, compat});
}

void SettingsDialog::timerEvent(QTimerEvent*) {
    if (m_androidState) m_androidState->setText("Состояние: " + m_e->message() + (m_e->state() == AndroidEngine::State::Ready ? "  •  ABI: " + m_e->deviceAbi() : QString()));
    if (m_latency && m_g) {
        auto& L = m_g->latency();
        m_latency->setText(L.count() ? QString("Задержка ввода (мышь → подтверждение эмулятора): средняя %1 мс, 95-й перцентиль %2 мс, макс %3 мс, событий %4")
                                                .arg(L.avg(), 0, 'f', 2).arg(L.percentile(95), 0, 'f', 2).arg(L.max(), 0, 'f', 2).arg(L.count())
                                          : QString("Задержка ввода: нет данных. Включите игровой режим, захватите курсор и подвигайте мышью."));
        const QPointF r = m_g->testRaw(), o = m_g->testOut();
        const QString kx = r.x() != 0 ? QString::number(o.x() / r.x(), 'f', 4) : "—", ky = r.y() != 0 ? QString::number(o.y() / r.y(), 'f', 4) : "—";
        m_accuracy->setText(QString("Тест точности: сырые отсчёты X %1 / Y %2 → смещение пальца X %3 / Y %4 px. Коэффициент X %5, Y %6 (без сглаживания он постоянный = без ускорения).")
                                .arg(r.x()).arg(r.y()).arg(o.x(), 0, 'f', 1).arg(o.y(), 0, 'f', 1).arg(kx, ky));
    }
}
