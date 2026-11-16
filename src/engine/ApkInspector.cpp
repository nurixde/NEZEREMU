#include "ApkInspector.h"
#include <QFile>
#include <QRegularExpression>
#include <QSet>
#include <QtEndian>

namespace ApkInspector {
static quint32 rd32(const QByteArray& b, qsizetype o) { return qFromLittleEndian<quint32>(b.constData() + o); }
static quint16 rd16(const QByteArray& b, qsizetype o) { return qFromLittleEndian<quint16>(b.constData() + o); }

QStringList zipEntryNames(const QByteArray& d, QString* error) {
    QStringList names;
    const qsizetype minEocd = 22;
    if (d.size() < minEocd) { if (error) *error = "Файл слишком мал"; return names; }
    qsizetype eocd = -1;
    for (qsizetype i = d.size() - minEocd; i >= qMax<qsizetype>(0, d.size() - 65557); --i)
        if (rd32(d, i) == 0x06054b50) { eocd = i; break; }
    if (eocd < 0) { if (error) *error = "Это не APK (не ZIP-архив)"; return names; }
    const quint16 count = rd16(d, eocd + 10);
    qsizetype p = rd32(d, eocd + 16);
    for (int i = 0; i < count; ++i) {
        if (p + 46 > d.size() || rd32(d, p) != 0x02014b50) { if (error) *error = "Повреждённый APK"; break; }
        const quint16 n = rd16(d, p + 28), x = rd16(d, p + 30), c = rd16(d, p + 32);
        if (p + 46 + n > d.size()) break;
        names << QString::fromUtf8(d.mid(p + 46, n));
        p += 46 + n + x + c;
    }
    return names;
}

ApkInfo inspect(const QString& path) {
    ApkInfo info;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) { info.error = "Не удалось открыть файл"; return info; }
    QString err;
    const QStringList names = zipEntryNames(f.readAll(), &err);
    if (names.isEmpty()) { info.error = err.isEmpty() ? "Пустой архив" : err; return info; }
    QSet<QString> abis;
    for (const QString& n : names) {
        if (n == "AndroidManifest.xml") info.hasManifest = true;
        if (n.endsWith(".apk")) info.isSplitBundle = true;
        if (n.startsWith("lib/")) { const QString a = n.section('/', 1, 1); if (!a.isEmpty()) abis.insert(a); }
    }
    info.abis = QStringList(abis.begin(), abis.end());
    info.abis.sort();
    info.valid = info.hasManifest;
    if (!info.valid) info.error = info.isSplitBundle ? "Это набор APK (XAPK/APKS). Распакуйте его и установите через Play Маркет или по частям."
                                            : "В архиве нет AndroidManifest.xml, это не APK";
    return info;
}

QString compatibilityNote(const ApkInfo& i, const QString& dev) {
    if (!i.valid) return i.error;
    if (i.abis.isEmpty()) return "Без нативного кода: должно работать на любой архитектуре.";
    if (i.abis.contains(dev)) return "Есть сборка под " + dev + ": лучшая совместимость и скорость.";
    if (dev.startsWith("x86") && (i.abis.contains("arm64-v8a") || i.abis.contains("armeabi-v7a")))
        return "Только ARM (" + i.abis.join(", ") + "). Запуск идёт через трансляцию ARM образа Android: возможны снижение FPS или отказ запуска.";
    return "Архитектуры APK (" + i.abis.join(", ") + ") не совпадают с " + dev + ". Установка может не пройти.";
}

QString explainInstallError(const QString& out) {
    static const QList<QPair<QString, QString>> map = {
        {"INSTALL_FAILED_NO_MATCHING_ABIS", "Приложение собрано для другой архитектуры процессора, и образ Android не может его транслировать."},
        {"INSTALL_FAILED_OLDER_SDK", "Приложению нужна более новая версия Android."},
        {"INSTALL_FAILED_INSUFFICIENT_STORAGE", "Недостаточно места внутри Android."},
        {"INSTALL_FAILED_VERSION_DOWNGRADE", "Уже установлена более новая версия."},
        {"INSTALL_FAILED_UPDATE_INCOMPATIBLE", "Установлена версия с другой подписью. Удалите её и повторите."},
        {"INSTALL_PARSE_FAILED", "APK повреждён или загружен не полностью."},
        {"INSTALL_FAILED_MISSING_SPLIT", "Приложению нужны дополнительные части (split APK). Установите его из Play Маркета."},
        {"device offline", "Android ещё не готов. Подождите загрузки."}};
    for (const auto& p : map) if (out.contains(p.first)) return p.second;
    static const QRegularExpression re("(INSTALL_[A-Z_]+)");
    const auto m = re.match(out);
    return m.hasMatch() ? "Ошибка установки: " + m.captured(1) : "Неизвестная ошибка установки: " + out.trimmed().left(200);
}
}
