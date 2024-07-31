#include "../common/logger.h"
#include "global_variables.h"

#include "window_overview.h"

#include <QApplication>
#include <QFontDatabase>
#include <QGuiApplication>

#include <filesystem>

int main(int argc, char *argv[])
{
    init_logger();
    SPDLOG_INFO("");
    SPDLOG_INFO("");
    SPDLOG_INFO("Qrammer started (git commit: {})", GIT_COMMIT_HASH);
    SPDLOG_INFO("Settings are stored at: {}", settings.fileName().toStdString());

    QApplication a(argc, argv);

    QDir binPath = QApplication::applicationFilePath();
    binPath.cdUp();
    binPath.cdUp();
    auto dbPath = binPath.filesystemAbsolutePath() / "db/database.sqlite";
    SPDLOG_INFO("Database path: {}", dbPath.string());
    db = DB(dbPath);

    MainWindow w;
    w.showMaximized();
    // It appears that this line is needed so that the program would not quit if users select NO at the break reminder.
    a.setQuitOnLastWindowClosed(false);

    return a.exec();
}
