#pragma once
#include <QPixmap>
#include <QWidget>

// Фон в режиме cover: без искажений, кешируется под размер (дешёвая перерисовка).
class WallpaperWidget : public QWidget {
    Q_OBJECT
public:
    explicit WallpaperWidget(QWidget* parent = nullptr);
    bool setImage(const QString& path);
    void setDim(int alpha) { m_dim = alpha; update(); }
protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override { m_cache = QPixmap(); }
    QPixmap m_pix, m_cache; int m_dim = 0;
};
