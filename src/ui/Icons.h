#pragma once
#include <QIcon>
#include <QString>

// Векторные линейные иконки интерфейса (рисуются кодом, без сторонних файлов).
namespace Icons {
QIcon get(const QString& name, const QColor& color = Qt::white, int size = 64);
}
