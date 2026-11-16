#include "AdbParsers.h"
#include <QHash>
#include <QRegularExpression>
#include <QSet>

namespace AdbParsers {
QStringList packagesFromQueryActivities(const QString& out) {
    QStringList res; QSet<QString> seen;
    static const QRegularExpression re("^\\s*([A-Za-z][\\w.]+)/[\\w.$]+\\s*$");
    for (const QString& line : out.split('\n')) {
        const auto m = re.match(line.trimmed());
        if (m.hasMatch() && !seen.contains(m.captured(1))) { seen.insert(m.captured(1)); res << m.captured(1); }
    }
    return res;
}

qint64 surfaceFlingerFrameCount(const QString& out) {
    static const QRegularExpression re("Parcel\\(([0-9a-fA-F]{8})");
    const auto m = re.match(out);
    if (!m.hasMatch()) return -1;
    bool ok = false; const qint64 v = m.captured(1).toLongLong(&ok, 16);
    return ok ? v : -1;
}

QString displayLabel(const QString& pkg) {
    static const QHash<QString, QString> known = {
        {"com.android.vending", "Play Маркет"}, {"com.android.chrome", "Chrome"},
        {"com.google.android.documentsui", "Файлы"}, {"com.android.settings", "Настройки Android"},
        {"com.dts.freefireth", "Free Fire"}, {"com.dts.freefiremax", "Free Fire MAX"},
        {"com.tencent.ig", "PUBG MOBILE"}, {"com.pubg.krmobile", "PUBG MOBILE KR"}, {"com.vng.pubgmobile", "PUBG MOBILE VN"},
        {"com.axlebolt.standoff2", "Standoff 2"}, {"com.roblox.client", "Roblox"},
        {"com.google.android.youtube", "YouTube"}, {"com.google.android.gm", "Gmail"},
        {"com.google.android.apps.maps", "Карты"}, {"com.google.android.apps.photos", "Фото"},
        {"com.mojang.minecraftpe", "Minecraft"}, {"com.supercell.brawlstars", "Brawl Stars"},
        {"com.activision.callofduty.shooter", "Call of Duty"}, {"com.miHoYo.GenshinImpact", "Genshin Impact"}};
    if (known.contains(pkg)) return known.value(pkg);
    QString last = pkg.section('.', -1);
    if (last.isEmpty()) return pkg;
    last[0] = last[0].toUpper();
    return last;
}
}
