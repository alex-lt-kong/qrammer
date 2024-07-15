#include "mainwindow.h"

#include <QApplication>
#include <QFontDatabase>
#include <spdlog/async.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

using namespace spdlog;

void init_logger()
{
    auto stdout_sink = std::make_shared<sinks::stdout_color_sink_mt>();

    size_t max_size_bytes = 10 * 1024 * 1024;
    size_t max_files = 3;
    auto rotating_sink = std::make_shared<sinks::rotating_file_sink_mt>("logs/qrammer-db-util.log",
                                                                        max_size_bytes,
                                                                        max_files);

    std::vector<sink_ptr> sinks{stdout_sink, rotating_sink};
    // q_size means the number of items in the queue, not the size in byte of the
    // queue
    init_thread_pool(1024 * 8, 1);
    auto logger = std::make_shared<async_logger>("qrammer_logger",
                                                 sinks.begin(),
                                                 sinks.end(),
                                                 thread_pool(),
                                                 async_overflow_policy::overrun_oldest);
    spdlog::set_default_logger(logger);
    spdlog::set_pattern("%Y-%m-%dT%T.%f%z|%5t|%10!|%8l| %v");
}

int main(int argc, char *argv[])
{
    init_logger();
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    /*
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
    */
    return a.exec();
}
