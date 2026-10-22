#include "mainwindow.h"
#include "theme.h"
#include "settingsmanager.h"

#include <QApplication>

int main(int argc, char *argv[])
{
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
