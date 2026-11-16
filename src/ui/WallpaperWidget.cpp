#include "WallpaperWidget.h"
#include <QPainter>

WallpaperWidget::WallpaperWidget(QWidget* parent) : QWidget(parent) { setAttribute(Qt::WA_OpaquePaintEvent); }

bool WallpaperWidget::setImage(const QString& path) {
    QPixmap p(path);
    if (p.isNull()) return false;
    m_pix = p; m_cache = QPixmap(); update();
    return true;
}

void WallpaperWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    if (m_pix.isNull()) { QLinearGradient g(0, 0, 0, height()); g.setColorAt(0, QColor("#1a0f33")); g.setColorAt(1, QColor("#3b1f73")); p.fillRect(rect(), g); return; }
    const qreal dpr = devicePixelRatioF();
    const QSize target = size() * dpr;
    if (m_cache.size() != target) {
        const QPixmap scaled = m_pix.scaled(target, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        m_cache = scaled.copy((scaled.width() - target.width()) / 2, (scaled.height() - target.height()) / 2, target.width(), target.height());
        m_cache.setDevicePixelRatio(dpr);
    }
    p.drawPixmap(0, 0, m_cache);
    if (m_dim > 0) p.fillRect(rect(), QColor(8, 4, 20, m_dim));
}
