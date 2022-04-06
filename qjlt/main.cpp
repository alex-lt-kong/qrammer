#include "mainwindow.h"
#include <QApplication>
#include <QGuiApplication>
#include <QFontDatabase>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.showMaximized();

    QDir tmpDir = QApplication::applicationFilePath();
    tmpDir.cdUp();
    QFontDatabase::addApplicationFont(tmpDir.path() +  "/NotoSansMonoCJKsc-Regular.otf");
    QFontDatabase::addApplicationFont(tmpDir.path() + "/NotoSansCJKsc-Medium.otf");


    if (QGuiApplication::platformName() == "windows") {        
        QFont font = w.font();
        font.setFamily("Microsoft Yahei");
   //     font.setPixelSize(15);
        QApplication::setFont(font);
    } else if (QGuiApplication::platformName() == "xcb") {
        QFont font = w.font();
        font.setFamily("Noto Sans CJK SC Medium");
  //      font.setPixelSize(14);
        QApplication::setFont(font);
    }
//    qDebug() << "About to output env";
//    qDebug() << QString::fromLatin1(qgetenv("QT_IM_MODULE"));

    a.setQuitOnLastWindowClosed(false);     // It appears that this line is needed so that the program would not quit if users select NO at the break reminder.
    return a.exec();
}
