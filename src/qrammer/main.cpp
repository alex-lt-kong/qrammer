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

    db = DB();
    SPDLOG_INFO("Database path: {}", db.getDatabasePath().toStdString());
    // It appears that this line is needed so that the program would not quit if users select NO at the break reminder.
    a.setQuitOnLastWindowClosed(false);
    return a.exec();
}
