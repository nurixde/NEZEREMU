#include <QtTest>
#include <QTemporaryDir>
#include "settings/SettingsStore.h"

class TestSettings : public QObject {
    Q_OBJECT
private slots:
    void roundTrip() {
        QTemporaryDir d; const QString p = d.path() + "/s.json";
        { SettingsStore s(p); s.setValue("appearance.dockOpacity", 55); s.setValue("dock.pinned", QStringList{"a", "b"}); QVERIFY(s.save()); }
        SettingsStore s2(p); QVERIFY(s2.load());
        QCOMPARE(s2.value("appearance.dockOpacity").toInt(), 55);
        QCOMPARE(s2.value("dock.pinned").toStringList(), (QStringList{"a", "b"}));
    }
    void defaults() {
        SettingsStore s("Z:/nonexistent/x.json"); QVERIFY(!s.load());
        QCOMPARE(s.value("perf.fps").toInt(), 60);
        QCOMPARE(s.value("appearance.dockOpacity").toInt(), 88);
        QCOMPARE(s.value("input.smoothing").toBool(), false);
        QCOMPARE(s.value("nope", 7).toInt(), 7);
    }
    void corrupted() {
        QTemporaryDir d; QFile f(d.path() + "/bad.json"); QVERIFY(f.open(QIODevice::WriteOnly)); f.write("{oops"); f.close();
        SettingsStore s(d.path() + "/bad.json"); QVERIFY(!s.load()); QCOMPARE(s.value("perf.fps").toInt(), 60);
    }
    void resetKeepsDock() {
        SettingsStore s("x.json"); s.setValue("perf.fps", 144); s.setValue("dock.pinned", QStringList{"p"});
        s.resetToDefaults({"dock."});
        QCOMPARE(s.value("perf.fps").toInt(), 60); QCOMPARE(s.value("dock.pinned").toStringList(), QStringList{"p"});
    }
};
QTEST_GUILESS_MAIN(TestSettings)
#include "test_settings.moc"
