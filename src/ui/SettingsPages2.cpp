// Разделы «Управление и чувствительность», «Оформление», «Дополнительно».
#include "SettingsDialog.h"
#include "Theme.h"
#include "Widgets.h"
#include "core/Paths.h"
#include "core/UpdateChecker.h"
#include "engine/AndroidEngine.h"
#include "engine/AdbClient.h"
#include "input/GameInput.h"
#include "input/Profile.h"
#include "settings/SettingsStore.h"
#include <QColorDialog>
#include <QComboBox>
#include <QDesktopServices>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QTableWidget>
#include <QUrl>
#include <QVBoxLayout>

static QDoubleSpinBox* dspin(double lo, double hi, double v, double step, int dec = 2) {
    auto* s = new QDoubleSpinBox; s->setRange(lo, hi); s->setSingleStep(step); s->setDecimals(dec); s->setValue(v); s->setFixedWidth(110); return s;
}

QWidget* SettingsDialog::pageInput() {
    auto* c = new Card("mouse", "PC Mouse Engine");
    auto* sx = dspin(0.05, 20, m_s->value("input.sensX").toDouble(), 0.05);
    auto* sy = dspin(0.05, 20, m_s->value("input.sensY").toDouble(), 0.05);
    auto* ads = dspin(0.05, 3, m_s->value("input.adsMul").toDouble(), 0.05);
    auto* dpi = new QSpinBox; dpi->setRange(100, 32000); dpi->setSingleStep(100); dpi->setValue(m_s->value("input.dpi").toInt()); dpi->setFixedWidth(110);
    c->addRow("Чувствительность X", "Линейная, без ускорения", sx);
    c->addRow("Чувствительность Y", {}, sy);
    c->addRow("Чувствительность ADS", "Множитель при прицеливании (ПКМ)", ads);
    c->addRow("DPI мыши", "Укажите DPI вашей мыши: ощущения не изменятся при смене DPI", dpi);
    auto* sm = new ToggleSwitch; sm->setChecked(m_s->value("input.smoothing").toBool());
    c->addRow("Сглаживание", "По умолчанию выключено (как в CS2)", sm);
    auto* ss = new QSlider(Qt::Horizontal); ss->setRange(0, 90); ss->setValue(m_s->value("input.smoothStrength").toInt()); ss->setFixedWidth(150);
    c->addRow("Сила сглаживания", {}, ss);
    auto* cap = new QComboBox; for (const char* k : {"`", "F1", "F2", "CapsLock", "Mouse5"}) cap->addItem(k);
    cap->setCurrentText(m_s->value("input.captureKey").toString());
    c->addRow("Клавиша захвата курсора", "Захват / освобождение в игровом режиме. Esc тоже освобождает", cap);
    auto save = [=] {
        set("input.sensX", sx->value()); set("input.sensY", sy->value()); set("input.adsMul", ads->value()); set("input.dpi", dpi->value());
        set("input.smoothing", sm->isChecked()); set("input.smoothStrength", ss->value()); set("input.captureKey", cap->currentText());
        if (m_g) m_g->reloadSettings();
    };
    for (auto* w : {sx, sy, ads}) connect(w, &QDoubleSpinBox::valueChanged, this, save);
    connect(dpi, &QSpinBox::valueChanged, this, save); connect(ss, &QSlider::valueChanged, this, save);
    connect(sm, &QAbstractButton::toggled, this, save); connect(cap, &QComboBox::currentTextChanged, this, save);

    auto* t = new Card("monitor", "Задержка и точность");
    m_latency = t->addNote({}); m_accuracy = t->addNote({});
    t->addNote("Замер включает обработку в NEZER EMU и доставку события в эмулятор. Время отрисовки кадра в игре сюда не входит.");
    auto* tr = new QHBoxLayout; auto* reset = new QPushButton("Сбросить статистику"); auto* calib = new QPushButton("Проверить касание");
    tr->addWidget(reset); tr->addWidget(calib); t->body()->addLayout(tr);
    auto* abs = new QComboBox; abs->addItem("Пиксели экрана", 0); abs->addItem("0–32767", 32767);
    abs->setCurrentIndex(qMax(0, abs->findData(m_s->value("input.absMax", 0).toInt())));
    t->addRow("Координаты касаний", "Если касание попадает не в центр экрана, смените режим", abs);
    connect(abs, &QComboBox::activated, this, [this, abs] { set("input.absMax", abs->currentData()); if (m_g) m_g->reloadSettings(); });
    connect(reset, &QPushButton::clicked, this, [this] { if (m_g) { m_g->latency().clear(); m_g->resetTest(); } });
    connect(calib, &QPushButton::clicked, this, [this] {
        if (m_e->state() != AndroidEngine::State::Ready) { QMessageBox::information(this, "NEZER EMU", "Сначала запустите Android."); return; }
        m_e->adb()->shell("settings put system pointer_location 1", nullptr);
        if (m_g) m_g->tapCenterForCalibration();
        QMessageBox::information(this, "NEZER EMU", "На экране Android включено отображение касаний, и отправлено касание в центр. Если отметка не в центре — смените «Координаты касаний» и повторите.\nОтключить отображение: Настройки Android → Для разработчиков → Отображать касания.");
    });

    auto* p = new Card("keyboard", "Профили и клавиши");
    m_profile = new QComboBox; m_profile->addItems(Profiles::names());
    m_profile->setCurrentText(m_s->value("input.profile").toString());
    p->addRow("Профиль", "При запуске игры из Dock выбирается профиль с её пакетом", m_profile);
    m_pkg = new QLineEdit; m_pkg->setPlaceholderText("например com.axlebolt.standoff2");
    p->addRow("Пакет игры", {}, m_pkg);
    m_table = new QTableWidget(0, 5); m_table->setHorizontalHeaderLabels({"Действие", "Клавиша", "X", "Y", "Край L/C/R"});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); m_table->verticalHeader()->hide(); m_table->setMinimumHeight(300);
    p->body()->addWidget(m_table);
    p->addNote("Координаты задаются для экрана 1920×1080 и привязываются к левому (L), центральному (C) или правому (R) краю: так раскладка точно адаптируется к 4:3, 16:10, 16:9 и 21:9. "
               "Позиции по умолчанию — шаблон: сверьте их с кнопками в вашей игре. WASD — джойстик, ЛКМ — огонь, ПКМ — прицел.");
    auto* pr = new QHBoxLayout;
    auto* add = new QPushButton("+ Клавиша"); auto* del = new QPushButton("Удалить строку"); auto* np = new QPushButton("Новый профиль");
    auto* sv = new QPushButton("Сохранить"); sv->setObjectName("primary");
    pr->addWidget(add); pr->addWidget(del); pr->addWidget(np); pr->addStretch(); pr->addWidget(sv); p->body()->addLayout(pr);
    connect(m_profile, &QComboBox::currentTextChanged, this, [this](const QString& n) { loadProfileToTable(n); set("input.profile", n); if (m_g) m_g->setProfile(Profiles::load(n)); });
    connect(add, &QPushButton::clicked, this, [this] { const int r = m_table->rowCount(); m_table->insertRow(r);
        const QStringList v{"Кнопка", "F", "960", "540", "C"}; for (int i = 0; i < 5; ++i) m_table->setItem(r, i, new QTableWidgetItem(v[i])); });
    connect(del, &QPushButton::clicked, this, [this] { const int r = m_table->currentRow(); if (r >= 4) m_table->removeRow(r); });
    connect(np, &QPushButton::clicked, this, [this] {
        bool ok = false; const QString n = QInputDialog::getText(this, "Новый профиль", "Название (например, Standoff 2):", QLineEdit::Normal, {}, &ok).trimmed();
        if (!ok || n.isEmpty() || n.contains('/') || n.contains('\\')) return;
        Profile pf = Profile::builtinShooter(); pf.name = n; Profiles::save(pf);
        m_profile->addItem(n); m_profile->setCurrentText(n);
    });
    connect(sv, &QPushButton::clicked, this, &SettingsDialog::saveProfileFromTable);
    loadProfileToTable(m_profile->currentText());
    return page({c, t, p});
}

void SettingsDialog::loadProfileToTable(const QString& name) {
    const Profile p = Profiles::load(name);
    m_pkg->setText(p.package);
    m_table->setRowCount(0);
    auto row = [this](const QString& a, const QString& k, QPointF pos, Anchor an, bool keyEditable) {
        const int r = m_table->rowCount(); m_table->insertRow(r);
        const QStringList v{a, k, QString::number(pos.x()), QString::number(pos.y()), MouseMath::anchorToString(an)};
        for (int i = 0; i < 5; ++i) { auto* it = new QTableWidgetItem(v[i]); if ((i == 0 || i == 1) && !keyEditable) it->setFlags(it->flags() & ~Qt::ItemIsEditable); m_table->setItem(r, i, it); }
    };
    row("Джойстик (центр)", "WASD", p.joyCenter, p.joyAnchor, false);
    row("Камера (центр зоны)", "Мышь", p.camAnchor, p.camAnchor_a, false);
    row("Огонь", "ЛКМ", p.fire, p.fireAnchor, false);
    row("Прицел", "ПКМ", p.scope, p.scopeAnchor, false);
    for (const KeyBind& k : p.keys) row(k.label, k.key, k.pos, k.anchor, true);
}

void SettingsDialog::saveProfileFromTable() {
    Profile p = Profiles::load(m_profile->currentText());
    p.name = m_profile->currentText(); p.package = m_pkg->text().trimmed();
    auto pt = [this](int r) { return QPointF(m_table->item(r, 2)->text().toDouble(), m_table->item(r, 3)->text().toDouble()); };
    auto an = [this](int r) { return MouseMath::anchorFromString(m_table->item(r, 4)->text().trimmed().toUpper()); };
    p.joyCenter = pt(0); p.joyAnchor = an(0); p.camAnchor = pt(1); p.camAnchor_a = an(1);
    p.fire = pt(2); p.fireAnchor = an(2); p.scope = pt(3); p.scopeAnchor = an(3);
    p.keys.clear();
    QStringList bad;
    for (int r = 4; r < m_table->rowCount(); ++r) {
        const QString key = m_table->item(r, 1)->text().trimmed();
        if (!Profiles::keyFromName(key)) { bad << key; continue; }
        p.keys.append({key, pt(r), an(r), m_table->item(r, 0)->text()});
    }
    Profiles::save(p);
    if (m_g) m_g->setProfile(p);
    QMessageBox::information(this, "NEZER EMU", bad.isEmpty() ? "Профиль сохранён." : "Сохранено. Неизвестные клавиши пропущены: " + bad.join(", ") + "\nДоступны: A–Z, 0–9, F1–F12, Space, Tab, Shift, Ctrl, Alt, Enter, Mouse4, Mouse5.");
}

QWidget* SettingsDialog::pageAppearance() {
    auto* c = new Card("brush", "Оформление");
    auto thumb = [](const QString& key, SettingsStore* s) {
        QString path = s->value(key).toString();
        if (path.isEmpty()) path = Paths::assetsDir() + (key.contains("lock") ? "/wallpaper_lock.jpg" : "/wallpaper_main.jpg");
        return QPixmap(path).scaled(220, 124, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation).copy(0, 0, 220, 124);
    };
    for (const auto& pair : QList<QPair<QString, QString>>{{"appearance.mainWallpaper", "Фон главного экрана"}, {"appearance.lockWallpaper", "Фон экрана блокировки"}}) {
        auto* lbl = new QLabel; lbl->setPixmap(thumb(pair.first, m_s)); lbl->setStyleSheet("border-radius:10px;");
        (pair.first.contains("lock") ? m_thumbLock : m_thumbMain) = lbl;
        auto* change = new QPushButton("Изменить фон"); auto* def = new QPushButton("По умолчанию");
        auto* col = new QVBoxLayout; col->addWidget(change); col->addWidget(def); col->addStretch();
        auto* h = new QHBoxLayout; h->addWidget(lbl); h->addLayout(col);
        c->addRow(pair.second, {}, nullptr); c->body()->addLayout(h);
        const QString key = pair.first;
        connect(change, &QPushButton::clicked, this, [this, key, lbl, thumb] {
            const QString f = QFileDialog::getOpenFileName(this, "Выберите фото", {}, "Изображения (*.png *.jpg *.jpeg *.webp *.bmp)");
            if (f.isEmpty()) return;
            const QString dst = Paths::ensure(Paths::dataDir() + "/wallpapers") + "/" + (key.contains("lock") ? "lock" : "main") + "." + QFileInfo(f).suffix();
            QFile::remove(dst); QFile::copy(f, dst);   // копия: фон не пропадёт, если оригинал удалят
            set(key, dst); lbl->setPixmap(thumb(key, m_s)); emit appearanceChanged();
        });
        connect(def, &QPushButton::clicked, this, [this, key, lbl, thumb] { set(key, ""); lbl->setPixmap(thumb(key, m_s)); emit appearanceChanged(); });
    }
    auto* sw = new QHBoxLayout;
    for (const char* hex : {"#8b5cf6", "#ef4444", "#3b82f6", "#22c55e"}) {
        auto* b = new QPushButton; b->setFixedSize(40, 40); b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet(QString("QPushButton{background:%1;border-radius:10px;border:2px solid %2;}").arg(hex, QColor(hex) == Theme::accent(m_s->value("appearance.accent").toString()) ? "white" : "transparent"));
        const QString h = hex;
        connect(b, &QPushButton::clicked, this, [this, h] { set("appearance.accent", h); emit appearanceChanged(); });
        sw->addWidget(b);
    }
    auto* custom = new QPushButton("Свой…");
    connect(custom, &QPushButton::clicked, this, [this] { const QColor c = QColorDialog::getColor(Theme::accent(m_s->value("appearance.accent").toString()), this); if (c.isValid()) { set("appearance.accent", c.name()); emit appearanceChanged(); } });
    sw->addWidget(custom); sw->addStretch();
    c->addRow("Цвет интерфейса", "Применяется сразу (цвет тумблеров — после переоткрытия настроек)", nullptr); c->body()->addLayout(sw);
    auto* op = new QSlider(Qt::Horizontal); op->setRange(10, 100); op->setValue(m_s->value("appearance.dockOpacity").toInt()); op->setFixedWidth(170);
    auto* opl = new QLabel(QString("%1%").arg(op->value()));
    auto* oh = new QWidget; auto* ohl = new QHBoxLayout(oh); ohl->setContentsMargins(0, 0, 0, 0); ohl->addWidget(op); ohl->addWidget(opl);
    c->addRow("Прозрачность Dock", {}, oh);
    connect(op, &QSlider::valueChanged, this, [this, opl](int v) { opl->setText(QString("%1%").arg(v)); set("appearance.dockOpacity", v); emit appearanceChanged(); });
    return page({c});
}

QWidget* SettingsDialog::pageExtra() {
    auto* c = new Card("extra", "Дополнительно");
    c->addNote("NEZER EMU " NEZER_VERSION ". Основа: официальный Android Emulator (AOSP). Интерфейс: Qt 6.");
    auto* upd = new QPushButton("Проверить обновления"); auto* updNote = c->addNote({});
    auto* checker = new UpdateChecker(m_s, this);
    connect(upd, &QPushButton::clicked, checker, &UpdateChecker::check);
    connect(checker, &UpdateChecker::result, this, [updNote](const QString& text, const QUrl& url) {
        updNote->setText(text);
        if (url.isValid() && QMessageBox::question(nullptr, "NEZER EMU", text + "\nОткрыть страницу загрузки?") == QMessageBox::Yes) QDesktopServices::openUrl(url);
    });
    auto* log = new QPushButton("Журнал Android Emulator"); connect(log, &QPushButton::clicked, this, &SettingsDialog::logRequested);
    auto* folder = new QPushButton("Открыть папку данных"); connect(folder, &QPushButton::clicked, this, [] { QDesktopServices::openUrl(QUrl::fromLocalFile(Paths::dataDir())); });
    auto* shots = new QPushButton("Открыть папку скриншотов"); connect(shots, &QPushButton::clicked, this, [] { QDesktopServices::openUrl(QUrl::fromLocalFile(Paths::screenshotsDir())); });
    auto* reset = new QPushButton("Сбросить настройки NEZER EMU");
    connect(reset, &QPushButton::clicked, this, [this] {
        if (QMessageBox::question(this, "NEZER EMU", "Сбросить все настройки? Приложения и данные Android не затрагиваются.") != QMessageBox::Yes) return;
        m_s->resetToDefaults({"dock."}); m_s->save(); emit appearanceChanged(); emit inputChanged(); close();
    });
    for (auto* b : {upd, log, folder, shots, reset}) c->body()->addWidget(b);
    return page({c});
}
