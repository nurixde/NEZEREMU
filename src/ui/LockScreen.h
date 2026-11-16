#pragma once
#include "WallpaperWidget.h"
class QLabel; class QPushButton; class QTimer;

// Экран блокировки: реальные часы и дата, «Войти в систему», свайп вверх с анимацией.
class LockScreen : public WallpaperWidget {
    Q_OBJECT
public:
    explicit LockScreen(QWidget* parent = nullptr);
    void setAccent(const QColor& c);
    void resetPosition();
signals:
    void unlockRequested();
    void windowDragRequested();
protected:
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
private:
    void tick();
    void relayout();
    QLabel *m_clock, *m_date, *m_hint; QPushButton* m_enter; QTimer* m_timer;
    int m_pressY = -1; int m_offset = 0;
};
