#include "audiomanager.h"
#include "mainwindow.h"
#include "theme.h"
#include "settingsmanager.h"

#include <QApplication>
#include <QLoggingCategory>
#include <QLibrary>

int main(int argc, char *argv[])
{
    // Отключаем избыточный технический вывод FFmpeg и Qt Multimedia в консоль
    qputenv("QT_LOGGING_RULES", "qt.multimedia*=false;qt.multimedia.ffmpeg*=false");
    QLoggingCategory::setFilterRules(QStringLiteral("qt.multimedia.*=false\nqt.multimedia.ffmpeg.*=false"));

    // Подавляем прямой вывод внутренних предупреждений декодера FFmpeg в stderr
    using AvLogSetLevelFn = void (*)(int);
    AvLogSetLevelFn avLogSetLevel = reinterpret_cast<AvLogSetLevelFn>(
        QLibrary::resolve(QStringLiteral("avutil-59"), "av_log_set_level")
    );
    if (!avLogSetLevel) {
        avLogSetLevel = reinterpret_cast<AvLogSetLevelFn>(
            QLibrary::resolve(QStringLiteral("avutil"), "av_log_set_level")
        );
    }
    if (avLogSetLevel) {
        avLogSetLevel(-8); // AV_LOG_QUIET: полное отключение спама FFmpeg в консоль
    }

    QApplication a(argc, argv);

    // Применяем централизованную тему и стиль приложения
    Theme::applyApplicationStyle(a);

    MainWindow w;
    if (SettingsManager::instance().isFullscreen()) {
        w.showFullScreen();
    } else {
        w.showNormal();
    }

    return a.exec();
}
