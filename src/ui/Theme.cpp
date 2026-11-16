#include "Theme.h"

namespace Theme {
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
QComboBox, QSpinBox, QDoubleSpinBox, QLineEdit { background:#140e28; border:1px solid %4; border-radius:8px; padding:6px 10px; min-height:20px; }
QComboBox:hover, QSpinBox:hover, QDoubleSpinBox:hover, QLineEdit:focus { border-color:%6; }
QComboBox::drop-down { border:none; width:22px; }
QComboBox QAbstractItemView { background:#140e28; border:1px solid %4; selection-background-color:%6; outline:0; }
QSlider::groove:horizontal { height:6px; background:#2a2045; border-radius:3px; }
QSlider::sub-page:horizontal { background:%6; border-radius:3px; }
QSlider::handle:horizontal { background:white; width:16px; height:16px; margin:-5px 0; border-radius:8px; }
QScrollArea, QScrollArea > QWidget > QWidget { background:transparent; border:none; }
QScrollBar:vertical { background:transparent; width:8px; }
QScrollBar::handle:vertical { background:%4; border-radius:4px; min-height:30px; }
QScrollBar::add-line, QScrollBar::sub-line { height:0; }
QListWidget { background:transparent; border:none; outline:0; }
QListWidget::item { padding:10px 12px; border-radius:9px; margin:2px 6px; }
QListWidget::item:selected { background:%6; }
QListWidget::item:hover:!selected { background:#22183f; }
QTableWidget { background:#140e28; gridline-color:%4; border:1px solid %4; border-radius:8px; }
QHeaderView::section { background:%3; border:none; padding:6px; color:%5; }
QProgressBar { background:#140e28; border:1px solid %4; border-radius:8px; text-align:center; height:18px; }
QProgressBar::chunk { background:%6; border-radius:7px; }
QPlainTextEdit { background:#0b0717; border:1px solid %4; border-radius:8px; font-family:Consolas; font-size:12px; }
QMenu { background:%3; border:1px solid %4; padding:6px; border-radius:10px; }
QMenu::item { padding:8px 22px; border-radius:6px; }
QMenu::item:selected { background:%6; }
QCheckBox::indicator { width:18px; height:18px; border:1px solid %4; border-radius:5px; background:#140e28; }
QCheckBox::indicator:checked { background:%6; border-color:%6; }
)").arg(kText, kBg, kCard, kBorder, kMuted, ac, ach);
}
}
