#pragma once
#include <QDialog>
class SettingsStore; class SdkInstaller; class QStackedWidget; class QLabel; class QProgressBar; class QPlainTextEdit; class QPushButton; class QCheckBox; class QVBoxLayout; class QComboBox;

// Первоначальная настройка: проверка системы -> лицензии -> загрузка Android -> проверка ускорения.
class SetupWizard : public QDialog {
    Q_OBJECT
public:
    SetupWizard(SettingsStore* s, QWidget* parent = nullptr);
private:
    QWidget* pageCheck(); QWidget* pageLicense(); QWidget* pageInstall(); QWidget* pageDone();
    void startInstall();
    SettingsStore* m_s; SdkInstaller* m_inst; QStackedWidget* m_pages;
    QVBoxLayout* m_checkList; QPushButton* m_checkNext; QCheckBox* m_accept; QComboBox* m_image;
    QLabel* m_stage; QProgressBar* m_bar; QPlainTextEdit* m_log; QPushButton* m_cancel; QLabel* m_doneText;
};
