#-------------------------------------------------
#
# Project created by QtCreator 2017-10-07T18:03:41
#
#-------------------------------------------------

QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qjlt-db-utility
TEMPLATE = app

VERSION = 7.3.19.0918
QMAKE_TARGET_COMPANY = Mamsds Studio
QMAKE_TARGET_PRODUCT = Qt-based Joint Learning Tool - DB Utility
QMAKE_TARGET_DESCRIPTION = A qt-based cross-platform learning tool by Mamsds Studio
QMAKE_TARGET_COPYRIGHT = @COPYLEFT ALL WRONGS RESERVED

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
        main.cpp \
        mainwindow.cpp

HEADERS += \
        mainwindow.h

FORMS += \
        mainwindow.ui

RESOURCES += \
    res.qrc

RC_ICONS = Utility.ico
