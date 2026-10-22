QT       += core core-private gui network widgets multimedia

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    leaderboardmanager.cpp \
    settingsmanager.cpp \
    mainmenuwidget.cpp \
    settingswidget.cpp \
    exitdialog.cpp \
    theme.cpp \
    audiomanager.cpp \
    beatmap.cpp \
    beatmapmanager.cpp \
    songselectwidget.cpp

HEADERS += \
    mainwindow.h \
    scorerecord.h \
    firebaseconfig.h \
    leaderboardmanager.h \
    settingsmanager.h \
    mainmenuwidget.h \
    settingswidget.h \
    exitdialog.h \
    theme.h \
    audiomanager.h \
    beatmap.h \
    beatmapmanager.h \
    songselectwidget.h

RESOURCES += \
    resources.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
