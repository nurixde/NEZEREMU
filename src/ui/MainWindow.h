#pragma once
#include "engine/AndroidEngine.h"
#include <QStringList>
#include <QWidget>
class SettingsStore; class AppManager; class GameInput; class TitleBar; class SideToolbar; class QStackedWidget;
class LockScreen; class DesktopScreen; class AndroidView;

// Главное окно: блокировка -> рабочий стол -> настоящий Android; все модули связаны здесь.
class MainWindow : public QWidget {
    Q_OBJECT
public:
    explicit MainWindow(SettingsStore* s, QWidget* parent = nullptr);
protected:
    void resizeEvent(QResizeEvent*) override;
    void closeEvent(QCloseEvent*) override;
    void dragEnterEvent(QDragEnterEvent*) override;
    void dropEvent(QDropEvent*) override;
    bool nativeEvent(const QByteArray& type, void* message, qintptr* result) override;
private:
    void applyAppearance();
    void unlock();
    void showDesktop();
    void showAndroid();
    void onState(AndroidEngine::State st, const QString& msg);
    void onToolbar(const QString& id);
    void launch(const QString& pkg);
    void installApk(const QString& path = {});
    void openSettings(int section = 0);
    void openSetup();
    void showLog();
    void updateTitle();
    void refreshDock();
    SettingsStore* m_s; AndroidEngine* m_engine; AppManager* m_apps; GameInput* m_game;
    TitleBar* m_title; SideToolbar* m_tool; QWidget* m_content; QStackedWidget* m_stack;
    LockScreen* m_lock; DesktopScreen* m_desktop; AndroidView* m_android;
    QStringList m_log; QString m_fpsText, m_latText, m_flash; bool m_locked = true; bool m_setupShown = false;
};
