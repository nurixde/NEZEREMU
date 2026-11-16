#pragma once
#include <QWidget>

// Встраивает окно Android Emulator внутрь NEZER EMU (Win32 SetParent).
class EmbeddedHost : public QWidget {
    Q_OBJECT
public:
    explicit EmbeddedHost(QWidget* parent = nullptr);
    void attach(quintptr hwnd);
    void detach();
    bool hasWindow() const { return m_child != 0; }
    void setChildInputEnabled(bool on);   // в игровом режиме мышь и клавиатура идут через Mouse Engine
    void focusChild();
protected:
    void resizeEvent(QResizeEvent*) override;
    void showEvent(QShowEvent*) override;
private:
    void fit();
    quintptr m_child = 0;
};
