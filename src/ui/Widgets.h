#pragma once
#include <QAbstractButton>
#include <QButtonGroup>
#include <QFrame>
#include <QPropertyAnimation>
#include <QWidget>
class QLabel; class QVBoxLayout;

// Переключатель-тумблер как в референсе.
class ToggleSwitch : public QAbstractButton {
    Q_OBJECT
    Q_PROPERTY(qreal knob READ knob WRITE setKnob)
public:
    explicit ToggleSwitch(QWidget* parent = nullptr);
    QSize sizeHint() const override { return {44, 24}; }
    qreal knob() const { return m_pos; }
    void setKnob(qreal p) { m_pos = p; update(); }
    static QColor accent;
protected:
    void paintEvent(QPaintEvent*) override;
    void checkStateSet() override;
private:
    qreal m_pos = 0; QPropertyAnimation* m_anim;
};

// Карточка настроек с иконкой и заголовком.
class Card : public QFrame {
    Q_OBJECT
public:
    Card(const QString& icon, const QString& title, QWidget* parent = nullptr);
    QVBoxLayout* body() const { return m_body; }
    // строка: название + подпись слева, элемент справа
    QWidget* addRow(const QString& title, const QString& hint, QWidget* control);
    QLabel* addNote(const QString& text);
private:
    QVBoxLayout* m_body;
};

// Сегментированный переключатель (Баланс / Производительность / Максимум).
class Segmented : public QWidget {
    Q_OBJECT
public:
    Segmented(const QStringList& labels, QWidget* parent = nullptr);
    int current() const { return m_group->checkedId(); }
    void setCurrent(int i);
signals:
    void changed(int index);
private:
    QButtonGroup* m_group;
};
