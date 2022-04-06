#include "mainwindow.h"
#include <QApplication>
#include <QFontDatabase>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    QDir tmpDir = QApplication::applicationFilePath();
    tmpDir.cdUp();
    QFontDatabase::addApplicationFont(tmpDir.path() +  "/NotoSansMonoCJKsc-Regular.otf");
    QFontDatabase::addApplicationFont(tmpDir.path() + "/NotoSansCJKsc-Medium.otf");

    if (QGuiApplication::platformName() == "windows") {
        QFont font = w.font();          // As such (instead of initialize the object from scratch), the default font size could be kept.
        font.setFamily("Microsoft Yahei");
        QApplication::setFont(font);
    } else if (QGuiApplication::platformName() == "xcb") {
        QFont font = w.font();          // As such (instead of initialize the object from scratch), the default font size could be kept.
        font.setFamily("Noto Sans CJK SC Medium");
        QApplication::setFont(font);
    }

    return a.exec();
}
