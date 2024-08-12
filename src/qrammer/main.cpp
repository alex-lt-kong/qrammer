#include "./src/qrammer/window/overview.h"
#include "global_variables.h"
#include "utils.h"

#include <QApplication>
#include <QFontDatabase>
#include <QGuiApplication>
#include <spdlog/spdlog.h>

#include <filesystem>

using namespace Qrammer;

int main(int argc, char *argv[])
{
    init_logger();
    SPDLOG_INFO("");
    SPDLOG_INFO("");
    SPDLOG_INFO("Qrammer started (git commit: {})", GIT_COMMIT_HASH);
    SPDLOG_INFO("Settings are stored at: {}", settings.fileName().toStdString());

    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);

    QDir binPath = QApplication::applicationFilePath();
    binPath.cdUp();
    binPath.cdUp();
    auto dbPath = binPath.filesystemAbsolutePath() / "db/database.sqlite";
    SPDLOG_INFO("Database path: {}", dbPath.string());
    spdlog::default_logger()->flush();
    db = DB(dbPath);

    Window::Overview w;
    int retval = -1;
    if (w.init()) {
        w.showMaximized();
        retval = a.exec();
    }
    SPDLOG_INFO("Qrammer exited, retval: {}", retval);
    return retval;
}
