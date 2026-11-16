#pragma once
#include <QWidget>
class EmbeddedHost; class QLabel; class QStackedLayout;

// Экран настоящего Android внутри окна NEZER EMU.
class AndroidView : public QWidget {
    Q_OBJECT
public:
    explicit AndroidView(QWidget* parent = nullptr);
    EmbeddedHost* host() const { return m_host; }
    void attach(quintptr hwnd);
    void detach();
    void setMessage(const QString& m);
private:
    EmbeddedHost* m_host; QLabel* m_msg; QStackedLayout* m_stack;
};
