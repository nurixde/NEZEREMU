#pragma once
#include "WallpaperWidget.h"
#include "apps/AppManager.h"
#include "engine/AndroidEngine.h"
class Dock; class QLabel; class QPushButton; class QProgressBar; class QFrame; class QHBoxLayout;

// Рабочий стол NEZER EMU: фон, ярлыки слева сверху, Dock внизу, статус Android по центру.
class DesktopScreen : public WallpaperWidget {
    Q_OBJECT
public:
    explicit DesktopScreen(QWidget* parent = nullptr);
    Dock* dock() const { return m_dock; }
    void setShortcuts(const QList<AppEntry>& apps);
    void setAndroidState(AndroidEngine::State st, const QString& msg);
public slots:
    void relayout();
signals:
    void launchRequested(const QString& pkg);
    void setupRequested();
    void startRequested();
    void logRequested();
protected:
    void resizeEvent(QResizeEvent*) override;
private:
    Dock* m_dock; QWidget* m_shortcuts; QHBoxLayout* m_scLay; QLabel* m_clock;
    QFrame* m_status; QLabel* m_statusText; QProgressBar* m_busy; QPushButton* m_primary; QPushButton* m_log;
    AndroidEngine::State m_state = AndroidEngine::State::Stopped;
};
