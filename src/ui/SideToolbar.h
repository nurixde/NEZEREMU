#pragma once
#include <QHash>
#include <QWidget>
class QToolButton;

// Узкая панель справа (порядок как в референсе).
class SideToolbar : public QWidget {
    Q_OBJECT
public:
    explicit SideToolbar(QWidget* parent = nullptr);
    void setAndroidReady(bool on);
    void setChecked(const QString& id, bool on);
    QToolButton* button(const QString& id) const { return m_btn.value(id); }
    static constexpr int kWidth = 50;
signals:
    void action(const QString& id);
protected:
    void paintEvent(QPaintEvent*) override;
private:
    QHash<QString, QToolButton*> m_btn;
};
