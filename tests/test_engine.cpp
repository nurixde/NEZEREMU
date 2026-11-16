#include <QtTest>
#include "core/UpdateChecker.h"
#include "display/FpsMonitor.h"
#include "engine/AdbParsers.h"
#include "engine/ApkInspector.h"
#include "engine/AvdConfig.h"
#include "input/TouchInjector.h"

class TestEngine : public QObject {
    Q_OBJECT
private slots:
    void apkAbis() {
        const ApkInfo i = ApkInspector::inspect(QString(TEST_DATA_DIR) + "/arm_only.apk");
        QVERIFY(i.valid); QCOMPARE(i.abis, (QStringList{"arm64-v8a", "armeabi-v7a"}));
        QVERIFY(ApkInspector::compatibilityNote(i, "x86_64").contains("ARM"));
        const ApkInfo j = ApkInspector::inspect(QString(TEST_DATA_DIR) + "/java_only.apk");
        QVERIFY(j.valid); QVERIFY(j.abis.isEmpty());
        QVERIFY(!ApkInspector::inspect(QString(TEST_DATA_DIR) + "/not_apk.txt").valid);
    }
    void installErrors() {
        QVERIFY(ApkInspector::explainInstallError("Failure [INSTALL_FAILED_NO_MATCHING_ABIS: x]").contains("архитектуры"));
        QVERIFY(ApkInspector::explainInstallError("Failure [INSTALL_FAILED_FOO]").contains("INSTALL_FAILED_FOO"));
    }
    void parsers() {
        const QString q = "com.android.vending/com.google.android.finsky.activities.MainActivity\r\n  com.tencent.ig/com.epicgames.ue4.SplashActivity\nNo activities found\ncom.tencent.ig/.Other\n";
        QCOMPARE(AdbParsers::packagesFromQueryActivities(q), (QStringList{"com.android.vending", "com.tencent.ig"}));
        QCOMPARE(AdbParsers::surfaceFlingerFrameCount("Result: Parcel(0000a3b1    '....')"), qint64(0xa3b1));
        QCOMPARE(AdbParsers::surfaceFlingerFrameCount("service not found"), qint64(-1));
        QCOMPARE(AdbParsers::displayLabel("com.axlebolt.standoff2"), QString("Standoff 2"));
        QCOMPARE(FpsMonitor::fpsFrom(100, 160, 1000000000), 60.0);
        QCOMPARE(FpsMonitor::fpsFrom(-1, 160, 1000000000), -1.0);
    }
    void avdConfig() {
        const QString in = "hw.cpu.ncore=2\r\nfoo=bar\nhw.ramSize=2048\n";
        const QString out = AvdConfig::apply(in, {{"hw.cpu.ncore", "6"}, {"hw.lcd.width", "2560"}});
        const auto m = AvdConfig::parse(out);
        QCOMPARE(m.value("hw.cpu.ncore"), QString("6")); QCOMPARE(m.value("foo"), QString("bar"));
        QCOMPARE(m.value("hw.ramSize"), QString("2048")); QCOMPARE(m.value("hw.lcd.width"), QString("2560"));
        AvdParams p; p.width = 1080; p.height = 1920; p.landscape = true;
        const auto c = AvdConfig::toConfig(p);
        QCOMPARE(c.value("hw.lcd.width"), QString("1920")); QCOMPARE(c.value("hw.lcd.height"), QString("1080"));
    }
    void touchCommands() {
        TouchInjector t(nullptr); t.setScreen({1920, 1080});
        QCOMPARE(t.buildDown(0, 7, {100.4, 200.6}, true), QByteArray("event send EV_ABS:ABS_MT_SLOT:0 EV_ABS:ABS_MT_TRACKING_ID:7 EV_ABS:ABS_MT_POSITION_X:100 EV_ABS:ABS_MT_POSITION_Y:201 EV_ABS:ABS_MT_PRESSURE:100 EV_KEY:BTN_TOUCH:1 EV_SYN:SYN_REPORT:0"));
        QCOMPARE(t.buildUp(2, false), QByteArray("event send EV_ABS:ABS_MT_SLOT:2 EV_ABS:ABS_MT_TRACKING_ID:-1 EV_SYN:SYN_REPORT:0"));
        QCOMPARE(t.toDevice({5000, -10}), QPoint(1919, 0));
        t.setAbsMax(32767); QCOMPARE(t.toDevice({1919, 1079}), QPoint(32767, 32767));
    }
    void versions() {
        QCOMPARE(UpdateChecker::compareVersions("1.0.0", "v1.0.1"), -1);
        QCOMPARE(UpdateChecker::compareVersions("1.2", "1.2.0"), 0);
        QCOMPARE(UpdateChecker::compareVersions("2.0.0", "1.9.9"), 1);
    }
};
QTEST_GUILESS_MAIN(TestEngine)
#include "test_engine.moc"
