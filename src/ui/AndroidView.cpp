#include "AndroidView.h"
#include "display/EmbeddedHost.h"
#include <QLabel>
#include <QStackedLayout>

AndroidView::AndroidView(QWidget* parent) : QWidget(parent) {
    setStyleSheet("background:#000;");
    m_stack = new QStackedLayout(this);
    m_msg = new QLabel("Android ещё не запущен"); m_msg->setAlignment(Qt::AlignCenter); m_msg->setStyleSheet("color:#b9b2d6;font-size:14px;background:#07040f;");
    m_host = new EmbeddedHost;
    m_stack->addWidget(m_msg); m_stack->addWidget(m_host);
}
void AndroidView::attach(quintptr h) { m_stack->setCurrentWidget(m_host); m_host->attach(h); }
void AndroidView::detach() { m_host->detach(); m_stack->setCurrentWidget(m_msg); }
void AndroidView::setMessage(const QString& m) { m_msg->setText(m); }
