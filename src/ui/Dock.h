#pragma once
#include "apps/AppManager.h"
#include <QWidget>
class QHBoxLayout; class QLabel;

// Полупрозрачный Dock: «Добавить», реально установленные приложения, «Все приложения».
class Dock : public QWidget {
    Q_OBJECT
public:
    explicit Dock(QWidget* parent = nullptr);
    void setOpacityPercent(int p) { m_alpha = qBound(0, p, 100) * 255 / 100; update(); }
    void setAccent(const QColor& c) { m_accent = c; update(); }
    void setApps(const QList<AppEntry>& apps, bool androidReady);
    QSize sizeHint() const override;
signals:
    void launchRequested(const QString& pkg);
    void removeRequested(const QString& pkg);
    void addRequested();
    void allAppsRequested();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    QWidget* makeTile(const QIcon& icon, const QString& label, bool dashed);
    QHBoxLayout* m_lay; QList<QWidget*> m_tiles; int m_alpha = 224; QColor m_accent{"#8b5cf6"};
};
