#include "Theme.h"

namespace Theme {
const QList<Style>& styles() {
    static const QList<Style> list = {
        //  id         название                 bg         card       border     input      side       hover      muted      accent
        {"violet",  "Неон (фиолетовый)",   "#0f0a1e", "#1b1333", "#2e2152", "#140e28", "#120c25", "#22183f", "#a59ec4", "#8b5cf6"},
        {"azure",   "Azure (бирюзовый)",  "#061311", "#0f201d", "#1d3a35", "#0a1a17", "#0c1011", "#13302b", "#7e9e98", "#00e5b3"},
        {"crimson", "Crimson (красный)",   "#140809", "#22100f", "#3d1c1c", "#1a0b0c", "#170a0b", "#2e1414", "#c49e9e", "#ef4444"},
        {"ocean",   "Ocean (синий)",       "#070d1a", "#101b30", "#1e2f52", "#0b1426", "#0a1222", "#16264a", "#9eaec4", "#3b82f6"},
        {"forest",  "Forest (зелёный)",    "#07120a", "#0f2214", "#1d3d24", "#0b1a0f", "#0a160d", "#153020", "#9ec4a6", "#22c55e"},
        {"graphite","Graphite (янтарный)", "#0d0d10", "#18181c", "#2a2a31", "#121215", "#111114", "#222228", "#a1a1aa", "#f59e0b"},
    };
    return list;
}

const Style& style(const QString& id) {
    for (const Style& s : styles()) if (s.id == id) return s;
    return styles().first();
}

void applyStyle(const QString& id) {
    const Style& s = style(id);
    kBg = s.bg; kCard = s.card; kBorder = s.border; kInput = s.input; kSide = s.side; kHover = s.hover; kMuted = s.muted;
}

QColor accent(const QString& s) { QColor c(s); return c.isValid() ? c : QColor(kDefaultAccent); }

QString globalStyle(const QString& a) {
    const QColor c = accent(a);
    const QString ac = c.name(), ach = c.lighter(118).name();
    return QString(R"(
* { color:%1; font-family:"Segoe UI"; }
QDialog, QMessageBox { background:%2; }
QToolTip { background:%3; color:%1; border:1px solid %4; padding:5px; border-radius:6px; }
QPushButton { background:%3; border:1px solid %4; border-radius:9px; padding:8px 16px; }
QPushButton:hover { border-color:%6; }
QPushButton#primary { background:%6; border:none; font-weight:600; }
QPushButton#primary:hover { background:%7; }
QPushButton:disabled { color:%5; }
QComboBox, QSpinBox, QDoubleSpinBox, QLineEdit { background:%8; border:1px solid %4; border-radius:8px; padding:6px 10px; min-height:20px; }
QComboBox:hover, QSpinBox:hover, QDoubleSpinBox:hover, QLineEdit:focus { border-color:%6; }
QComboBox::drop-down { border:none; width:22px; }
QComboBox QAbstractItemView { background:%8; border:1px solid %4; selection-background-color:%6; outline:0; }
QSlider::groove:horizontal { height:6px; background:%4; border-radius:3px; }
QSlider::sub-page:horizontal { background:%6; border-radius:3px; }
QSlider::handle:horizontal { background:white; width:16px; height:16px; margin:-5px 0; border-radius:8px; }
QScrollArea, QScrollArea > QWidget > QWidget { background:transparent; border:none; }
QScrollBar:vertical { background:transparent; width:8px; }
QScrollBar::handle:vertical { background:%4; border-radius:4px; min-height:30px; }
QScrollBar::add-line, QScrollBar::sub-line { height:0; }
QListWidget { background:transparent; border:none; outline:0; }
QListWidget::item { padding:10px 12px; border-radius:9px; margin:2px 6px; }
QListWidget::item:selected { background:%6; }
QListWidget::item:hover:!selected { background:%9; }
QTableWidget { background:%8; gridline-color:%4; border:1px solid %4; border-radius:8px; }
QHeaderView::section { background:%3; border:none; padding:6px; color:%5; }
QProgressBar { background:%8; border:1px solid %4; border-radius:8px; text-align:center; height:18px; }
QProgressBar::chunk { background:%6; border-radius:7px; }
QPlainTextEdit { background:%2; border:1px solid %4; border-radius:8px; font-family:Consolas; font-size:12px; }
QMenu { background:%3; border:1px solid %4; padding:6px; border-radius:10px; }
QMenu::item { padding:8px 22px; border-radius:6px; }
QMenu::item:selected { background:%6; }
QCheckBox::indicator { width:18px; height:18px; border:1px solid %4; border-radius:5px; background:%8; }
QCheckBox::indicator:checked { background:%6; border-color:%6; }
)").arg(kText, kBg, kCard, kBorder, kMuted, ac, ach, kInput, kHover);
}
}
