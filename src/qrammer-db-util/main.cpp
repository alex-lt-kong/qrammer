#include "./src/common/logger.h"
#include "mainwindow.h"

#include <QApplication>
#include <QFontDatabase>

int main(int argc, char *argv[])
{
    init_logger();
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
