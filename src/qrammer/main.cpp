#include "../common/logger.h"
#include "global_variables.h"

#include "window_overview.h"

#include <QApplication>
#include <QFontDatabase>
#include <QGuiApplication>

int main(int argc, char *argv[])
{
    init_logger();
    SPDLOG_INFO("Qrammer started (git commit: {})", GIT_COMMIT_HASH);
    SPDLOG_INFO("Settings are stored at: {}", settings.fileName().toStdString());
    QApplication a(argc, argv);
    MainWindow w;
    w.showMaximized();

    QDir tmpDir = QApplication::applicationFilePath();
    tmpDir.cdUp();
    // QFontDatabase::addApplicationFont(tmpDir.path() + "/NotoSansMonoCJKsc-Regular.otf");
    // QFontDatabase::addApplicationFont(tmpDir.path() + "/NotoSansCJKsc-Medium.otf");

    /*
    if (QGuiApplication::platformName() == "windows") {
        QFont font = w.font();
        font.setFamily("Microsoft Yahei");
        // font.setPixelSize(15);
        QApplication::setFont(font);
    } else if (QGuiApplication::platformName() == "xcb") {
        QFont font = w.font();
        font.setFamily("Noto Sans CJK SC Medium");
        // font.setPixelSize(14);
        QApplication::setFont(font);
    }
    */

    // It appears that this line is needed so that the program would not quit if users select NO at the break reminder.
    a.setQuitOnLastWindowClosed(false);

    return a.exec();
}
