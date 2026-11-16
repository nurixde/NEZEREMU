#pragma once
#include <QList>
#include <QString>

struct CheckResult { QString name; bool ok; bool critical; QString detail; };

// Проверка системных требований (реальные значения системы).
namespace SystemCheck {
QList<CheckResult> run();
quint64 totalRamMb();
bool virtualizationAvailable(QString* how);
}
