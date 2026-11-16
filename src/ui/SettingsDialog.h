#pragma once
#include <QDialog>
class SettingsStore; class AndroidEngine; class GameInput; class Card; class QListWidget; class QStackedWidget;
class QLabel; class QComboBox; class QSpinBox; class QTableWidget; class QLineEdit;

// Меню настроек: 8 разделов в стиле референса (меню слева, карточки справа). Всё сохраняется сразу.
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    SettingsDialog(SettingsStore* s, AndroidEngine* e, GameInput* g, QWidget* parent = nullptr);
    void openSection(int i);
signals:
    void appearanceChanged();
    void inputChanged();
    void setupRequested();
    void logRequested();
protected:
    void timerEvent(QTimerEvent*) override;
private:
    QWidget* page(const QList<Card*>& cards);
    QWidget* pageGeneral(); QWidget* pagePerf(); QWidget* pageScreen(); QWidget* pageGraphics();
    QWidget* pageInput(); QWidget* pageAndroid(); QWidget* pageAppearance(); QWidget* pageExtra();
    void loadProfileToTable(const QString& name);
    void saveProfileFromTable();
    void set(const QString& k, const QVariant& v);
    SettingsStore* m_s; AndroidEngine* m_e; GameInput* m_g;
    QListWidget* m_menu; QStackedWidget* m_pages;
    QLabel *m_latency = nullptr, *m_accuracy = nullptr, *m_androidState = nullptr, *m_restartNote = nullptr;
    QComboBox *m_cores = nullptr, *m_ram = nullptr, *m_profile = nullptr;
    QTableWidget* m_table = nullptr; QLineEdit* m_pkg = nullptr;
    QLabel *m_thumbMain = nullptr, *m_thumbLock = nullptr;
};
