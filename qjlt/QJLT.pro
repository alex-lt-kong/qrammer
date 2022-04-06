#-------------------------------------------------
#
# Project created by QtCreator 2017-10-01T16:22:51
#
#-------------------------------------------------


#-------------------------------------------------
# Note regarding Chinese input:
# If a Qt program cannot input Chinese with Fcitx and following steps about the copy libfcitxplatforminputcontextplugin.so doesn't work, also pay attention to
# the Kit which is being used. As at 2019-03-31, the default kit (qt 5.7) works but the newer kit (qt 5.12.5 gcc 64) doesn't.
# 2019-04-07: only qt 5.7 works, 5.8+ don't work
# 2019-04-14: both qt 5.9 and 5.12 work as well, the 04-07 note should be a mistake. Also, using 5.7 has a negative effect: the MP3 module doesn't work.
# 2019-04-14: Okay abandoned fcitx, it just doesn't work with Qt.
# 2019-09-20: Tried a lot, ibus just doesn't work. Switched to fcitx...(but how about the note on 2019-04-14???)
# 2019-09-20: Regarding the Fcitx fix which is related to libfcitxplatforminputcontextplugin.so, I observed that this file is in the package named "libqt5gui5".
# This package's version pattern is similar to the version pattern of qt itself. I guess if the versions of Qt and libqt5gui5 are matching, then fcitx would work.
# 2019-09-21: the command "cp /usr/lib/x86_64-linux-gnu/qt5/plugins/platforminputcontexts/libfcitxplatforminputcontextplugin.so ./Development/Qt/Tools/QtCreator/lib/Qt/plugins/platforminputcontexts/" really works! Checked before and after the issuance of the command and now QtCreator supports Chinese. 如你所见。
# 2019-09-21: the command "cp /usr/lib/x86_64-linux-gnu/qt5/plugins/platforminputcontexts/libfcitxplatforminputcontextplugin.so ./Development/Qt/5.12.3/gcc_64/plugins/platforminputcontexts/" also works. Before the command, QJLT-5.12.3 cannot input Chinese and after the command QJLT-5.12.3 can input Chinese.
#-------------------------------------------------

VERSION = 7.3.19.0918
QMAKE_TARGET_COMPANY = Mamsds Studio
QMAKE_TARGET_PRODUCT = Qt-based Joint Learning Tool
QMAKE_TARGET_DESCRIPTION = A qt-based cross-platform learning tool by Mamsds Studio
QMAKE_TARGET_COPYRIGHT = @COPYLEFT ALL WRONGS RESERVED

QT += sql network
QT += core gui
QT += multimedia
QT += widgets

TARGET = qjlt
TEMPLATE = app

#QMAKE_LFLAGS = -no-pie

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
    bmbox.cpp \
        main.cpp \
        mainwindow.cpp \
    snapshot.cpp \
    practicewindow.cpp \
    downloader.cpp \
    msgbox.cpp

HEADERS += \
    bmbox.h \
        mainwindow.h \
    snapshot.h \
    practicewindow.h \
    downloader.h \
    msgbox.h

FORMS += \
    bmbox.ui \
        mainwindow.ui \
    practicewindow.ui \
    msgbox.ui

RC_ICONS = Main.ico

RESOURCES += \
    res.qrc

DISTFILES += \
    ChangeLog.txt \
    android/AndroidManifest.xml \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradlew \
    android/res/values/libs.xml \
    android/build.gradle \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew.bat

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
