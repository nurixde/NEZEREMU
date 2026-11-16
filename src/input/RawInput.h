#pragma once
#include <QAbstractNativeEventFilter>
#include <QObject>

// Windows Raw Input: относительные отсчёты мыши напрямую от драйвера (без ускорения Windows).
class RawInput : public QObject, public QAbstractNativeEventFilter {
    Q_OBJECT
public:
    enum class Mode { Off, KeyboardSink, Exclusive };
    explicit RawInput(QObject* parent = nullptr);
    ~RawInput() override;
    void setMode(Mode m, quintptr hwnd);
    Mode mode() const { return m_mode; }
    bool nativeEventFilter(const QByteArray& type, void* message, qintptr* result) override;
signals:
    void mouseMoved(int dx, int dy, qint64 tsNs);
    void mouseButton(int button, bool down, qint64 tsNs);   // 0 ЛКМ, 1 ПКМ, 2 СКМ, 3 Mouse4, 4 Mouse5
    void key(int vk, bool down, qint64 tsNs);
private:
    Mode m_mode = Mode::Off;
    quintptr m_hwnd = 0;
};
