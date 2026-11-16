#pragma once
#include <QString>
#include <QStringList>

namespace AdbParsers {
QStringList packagesFromQueryActivities(const QString& out);   // cmd package query-activities --brief
qint64 surfaceFlingerFrameCount(const QString& out);            // service call SurfaceFlinger 1013, -1 если недоступно
QString displayLabel(const QString& package);                    // известные имена, иначе из имени пакета
}
