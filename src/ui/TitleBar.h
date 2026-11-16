#pragma once
#include <QWidget>
class QLabel; class QToolButton;

// Верхняя панель: логотип, NEZER EMU, минималистичное управление окном.
class TitleBar : public QWidget {
    Q_OBJECT
public:
    explicit TitleBar(QWidget* parent = nullptr);
    void setStatus(const QString& text, const QColor& dot);
    static constexpr int kHeight = 40;
signals:
    void minimizeRequested(); void maximizeRequested(); void closeRequested();
protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
private:
    QLabel* m_status; QColor m_dot = Qt::gray;
};
