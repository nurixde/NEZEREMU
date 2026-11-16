#pragma once
#include <QMap>
#include <QString>

// Работа с config.ini виртуального устройства (AVD) официального Android Emulator.
struct AvdParams {
    int cores = 4; int ramMb = 4096;
    int width = 1920; int height = 1080; int dpi = 320;
    int refreshHz = 60;            // hw.lcd.vsync
    QString gpuMode = "host";      // host | auto | angle_indirect | swiftshader_indirect
    bool landscape = true;
};

namespace AvdConfig {
const QString kAvdName = "NezerAVD";
QString avdDir();
QString configPath();
QMap<QString, QString> parse(const QString& text);
QString apply(const QString& text, const QMap<QString, QString>& values);   // сохраняет порядок и неизвестные ключи
QMap<QString, QString> toConfig(const AvdParams& p);
bool write(const AvdParams& p, QString* error);
bool exists();
}
