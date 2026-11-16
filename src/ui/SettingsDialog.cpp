#include "SettingsDialog.h"
#include "Icons.h"
#include "Theme.h"
#include "Widgets.h"
#include "core/WinUtil.h"
#include "engine/AndroidEngine.h"
#include "engine/BlueStacks.h"
#include "input/GameInput.h"
#include "settings/SettingsStore.h"
#include <QComboBox>
#include <QDesktopServices>
#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QStackedWidget>
#include <QThread>
#include <QUrl>
#include <QVBoxLayout>
#include "core/SystemCheck.h"

void SettingsDialog::set(const QString& k, const QVariant& v) { m_s->setValue(k, v); m_s->save(); }

SettingsDialog::SettingsDialog(SettingsStore* s, AndroidEngine* e, GameInput* g, QWidget* parent)
    : QDialog(parent), m_s(s), m_e(e), m_g(g) {
    setWindowTitle("NEZER EMU — Настройки"); resize(1080, 700); setMinimumSize(900, 560);
    ToggleSwitch::accent = Theme::accent(m_s->value("appearance.accent").toString());
    auto* root = new QHBoxLayout(this); root->setContentsMargins(0, 0, 0, 0); root->setSpacing(0);
    auto* side = new QWidget; side->setFixedWidth(250); side->setStyleSheet(QString("background:%1;").arg(Theme::kSide));
    auto* sv = new QVBoxLayout(side); sv->setContentsMargins(10, 18, 10, 14);
    auto* head = new QLabel("  Настройки"); head->setStyleSheet("font-size:18px;font-weight:700;padding:4px 0 12px 4px;");
    m_menu = new QListWidget; m_menu->setIconSize(QSize(18, 18));
    const QList<QPair<QString, QString>> sections = {{"general", "Основные"}, {"chip", "Производительность"}, {"monitor", "Экран и разрешение"},
        {"graphics", "Графика"}, {"mouse", "Управление и чувствительность"}, {"android", "BlueStacks и Android"},
        {"brush", "Оформление"}, {"extra", "Дополнительно"}};
    for (const auto& sc : sections) m_menu->addItem(new QListWidgetItem(Icons::get(sc.first, QColor("#d9d2f5")), sc.second));
    sv->addWidget(head); sv->addWidget(m_menu, 1);
    auto* ver = new QLabel("  Версия " NEZER_VERSION); ver->setStyleSheet(QString("color:%1;font-size:11px;").arg(Theme::kMuted)); sv->addWidget(ver);
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
    c->addRow("Запускать Android сразу", "BlueStacks стартует, пока показан экран блокировки", autoAndroid);
    connect(autoAndroid, &QAbstractButton::toggled, this, [this](bool on) { set("general.autoAndroid", on); });
    auto* lang = new QComboBox; lang->addItem("Русский"); lang->setEnabled(false); lang->setToolTip("Другие языки пока не переведены");
    c->addRow("Язык интерфейса", {}, lang);
    auto* keys = new Card("keyboard", "Горячие клавиши");
    keys->addNote("F11 — полный экран\nCtrl+H — рабочий стол NEZER EMU\nCtrl+Shift+A — редактор игрового управления BlueStacks");
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
    m_restartNote = c->addNote("CPU и RAM записываются в BlueStacks и применятся после перезапуска Android."); m_restartNote->hide();
    auto* restart = new QPushButton("Перезапустить Android"); c->body()->addWidget(restart);
    connect(restart, &QPushButton::clicked, m_e, &AndroidEngine::restart);

    auto* f = new Card("monitor", "Частота кадров");
    auto* fps = new QComboBox;
    for (int v : {30, 60, 90, 120, 144, 165, 240}) fps->addItem(QString("%1 FPS").arg(v), v);
    fps->addItem("Без ограничения", 0);
    fps->setCurrentIndex(qMax(0, fps->findData(m_s->value("perf.fps").toInt())));
    f->addRow("Лимит FPS", "Лимит кадров BlueStacks. Итоговый FPS также зависит от самой игры и видеокарты", fps);
    connect(fps, &QComboBox::activated, this, [this, fps] { set("perf.fps", fps->currentData()); m_e->applyFrameRate(); m_restartNote->show(); });
    auto* show = toggle(m_s->value("perf.showFps").toBool());
    f->addRow("Показывать реальный FPS", "Измеряется по счётчику кадров Android, в верхней панели", show);
    connect(show, &QAbstractButton::toggled, this, [this](bool on) { set("perf.showFps", on); emit appearanceChanged(); });
    f->addNote("«Без ограничения» выставляет 240 FPS в BlueStacks. При лимите выше 60 автоматически включается «Высокая частота кадров». Применяется при следующем запуске BlueStacks.");
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
    auto* note = c->addNote("Разрешение и DPI записываются в настройки BlueStacks и полностью применяются после перезапуска Android (сразу — через wm size/density).");
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
    r->addItem("DirectX (рекомендуется)", "dx"); r->addItem("OpenGL", "gl"); r->addItem("Vulkan", "vlcn");
    r->setCurrentIndex(qMax(0, r->findData(m_s->value("gfx.bsRenderer").toString())));
    c->addRow("Рендеринг", "Графический движок BlueStacks", r);
    connect(r, &QComboBox::activated, this, [this, r] { set("gfx.bsRenderer", r->currentData()); });
    c->addNote("DirectX обычно самый стабильный. Если чёрный экран или вылеты, обновите драйвер видеокарты или попробуйте OpenGL. Применяется после перезапуска Android.");
    auto* restart = new QPushButton("Перезапустить Android"); c->body()->addWidget(restart);
    connect(restart, &QPushButton::clicked, m_e, &AndroidEngine::restart);
    return page({c});
}

QWidget* SettingsDialog::pageAndroid() {
    auto* c = new Card("android", "BlueStacks и Android");
    m_androidState = c->addNote({});
    const BlueStacks::Install bs = BlueStacks::detect();
    c->addNote(bs.found ? "BlueStacks: " + QDir::toNativeSeparators(bs.installDir) : QString("BlueStacks 5 не найден на этом ПК."));
    auto* inst = new QComboBox;
    const QStringList list = BlueStacks::instances();
    for (const QString& n : list) inst->addItem(n);
    if (list.isEmpty()) inst->addItem(m_e->instance());
    inst->setCurrentText(m_e->instance());
    c->addRow("Экземпляр Android", "Pie64 = Android 9, Rvc64 = Android 11, Tiramisu64 = Android 13. Новые создаются в «Менеджере окон» BlueStacks", inst);
    connect(inst, &QComboBox::activated, this, [this, inst] { set("bs.instance", inst->currentText()); });
    auto* row = new QHBoxLayout;
    auto* start = new QPushButton("Запустить"); auto* stop = new QPushButton("Остановить"); auto* rs = new QPushButton("Перезапустить");
    row->addWidget(start); row->addWidget(stop); row->addWidget(rs); c->body()->addLayout(row);
    connect(start, &QPushButton::clicked, m_e, &AndroidEngine::start); connect(stop, &QPushButton::clicked, m_e, &AndroidEngine::stop); connect(rs, &QPushButton::clicked, m_e, &AndroidEngine::restart);
    auto* apply = toggle(m_s->value("bs.applySettings").toBool());
    c->addRow("Передавать настройки в BlueStacks", "CPU, RAM, разрешение, DPI, FPS и рендер записываются в bluestacks.conf перед запуском", apply);
    connect(apply, &QAbstractButton::toggled, this, [this](bool on) { set("bs.applySettings", on); });
    auto* mim = new QPushButton("Менеджер окон BlueStacks");
    connect(mim, &QPushButton::clicked, this, [this] {
        const BlueStacks::Install b = BlueStacks::detect();
        if (!b.found || !QProcess::startDetached(b.multiInstance, {})) QMessageBox::information(this, "NEZER EMU", "Менеджер окон BlueStacks не найден.");
    });
    auto* dl = new QPushButton("Скачать BlueStacks 5");
    connect(dl, &QPushButton::clicked, this, [] { QDesktopServices::openUrl(QUrl(QString::fromLatin1(BlueStacks::kDownloadUrl))); });
    auto* check = new QPushButton("Проверить установку BlueStacks…");
    connect(check, &QPushButton::clicked, this, [this] { emit setupRequested(); });
    c->body()->addWidget(mim); c->body()->addWidget(dl); c->body()->addWidget(check);
    c->addNote("Если Android не определяется: в BlueStacks откройте Настройки → Дополнительно и включите «Android Debug Bridge (ADB)». Приложения и данные Android хранит сам BlueStacks.");
    auto* compat = new Card("general", "Совместимость игр");
    compat->addNote("Перед установкой APK NEZER EMU проверяет архитектуры нативных библиотек (ARM/x86) и объясняет ошибки установки.\n\n"
                    "Игровое управление (клавиши и мышь) настраивается встроенным редактором BlueStacks: кнопка «Управление» справа или Ctrl+Shift+A.");
    return page({c, compat});
}

void SettingsDialog::timerEvent(QTimerEvent*) {
    if (m_androidState) m_androidState->setText("Состояние: " + m_e->message() + (m_e->state() == AndroidEngine::State::Ready ? "  •  ABI: " + m_e->deviceAbi() : QString()));
    if (m_latency && m_g) {
        auto& L = m_g->latency();
        m_latency->setText(L.count() ? QString("Задержка ввода: средняя %1 мс, 95-й перцентиль %2 мс, макс %3 мс, событий %4")
                                                .arg(L.avg(), 0, 'f', 2).arg(L.percentile(95), 0, 'f', 2).arg(L.max(), 0, 'f', 2).arg(L.count())
                                          : QString("Задержка ввода: нет данных. С BlueStacks ввод идёт напрямую в его окно."));
        const QPointF r = m_g->testRaw(), o = m_g->testOut();
        const QString kx = r.x() != 0 ? QString::number(o.x() / r.x(), 'f', 4) : "—", ky = r.y() != 0 ? QString::number(o.y() / r.y(), 'f', 4) : "—";
        m_accuracy->setText(QString("Тест точности: сырые отсчёты X %1 / Y %2 → смещение пальца X %3 / Y %4 px. Коэффициент X %5, Y %6.")
                                .arg(r.x()).arg(r.y()).arg(o.x(), 0, 'f', 1).arg(o.y(), 0, 'f', 1).arg(kx, ky));
    }
}
