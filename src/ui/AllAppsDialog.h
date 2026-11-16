#pragma once
#include <QDialog>
class AppManager; class QListWidget; class QLineEdit;

class AllAppsDialog : public QDialog {
    Q_OBJECT
public:
    AllAppsDialog(AppManager* apps, bool pickForDock, QWidget* parent = nullptr);
signals:
    void launchRequested(const QString& pkg);
    void installApkRequested();
private:
    void fill();
    AppManager* m_apps; bool m_pick; QListWidget* m_list; QLineEdit* m_search;
};
