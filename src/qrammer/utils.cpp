#include "utils.h"

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QScreen>
#include <QString>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>
#ifdef WIN32
#include <windows.h>
#endif

#include <thread>

using namespace std;

void execExternalProgramAsync(const string cmd)
{
    thread th_exec([cmd]() {
        SPDLOG_INFO("Calling external program: [{}] in a separate child process.", cmd);
        int retval = 0;
        try {
#ifdef WIN32
            retval = WinExec(cmd.c_str(), SW_HIDE);
#else
            retval = system(cmd.c_str());
#endif
            SPDLOG_INFO("External program: [{}] returned {}", cmd, retval);
        } catch (const exception &e) {
            SPDLOG_ERROR("Failed calling {}: {}", cmd, e.what());
        }
    });
    th_exec.detach();
}

QPixmap selectImageFromFileSystem()
{
    QString fileName = QFileDialog::getOpenFileName(nullptr,
                                                    "Select an image",
                                                    nullptr,
                                                    "Images (*.png *.bmp *.jpg *.jpeg *.webp)");
    QPixmap image;
    if (fileName.isEmpty()) {
        SPDLOG_INFO("No file is selected");
        return QPixmap();
    }
    QByteArray byteArray;
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        return QPixmap();
    }
    byteArray = file.readAll();
    file.close();
    SPDLOG_INFO("fileName: {},byteArray.size(): {} bytes", fileName.toStdString(), byteArray.size());
    if (!image.loadFromData(byteArray)) {
        auto errMsg = QString(
                          "Failed loading file content from %1 into QPixmap, filesize: %2 bytes")
                          .arg(fileName)
                          .arg(byteArray.size());
        SPDLOG_ERROR(errMsg.toStdString());
        return QPixmap();
    }
    auto w = min(image.width(), ANSWER_IMAGE_DIMENSION);
    auto h = min(image.height(), QApplication::primaryScreen()->availableGeometry().height() / 3);
    image = image.scaled(QSize(w, h), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    return image;
}

void init_logger()
{
    auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    stdout_sink->set_level(spdlog::level::trace);
    size_t max_size_bytes = 1 * 1024 * 1024;
    size_t max_files = 3;
    auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/qrammer.log",
                                                                                max_size_bytes,
                                                                                max_files);
    std::vector<spdlog::sink_ptr> sinks{stdout_sink, rotating_sink};
    spdlog::logger lgr("multi_sink", {stdout_sink, rotating_sink});

    spdlog::set_default_logger(std::make_shared<spdlog::logger>(lgr));
    // Long function name column is needed as namespaces will be prepended to it...
    spdlog::set_pattern("%Y-%m-%dT%T.%f%z | %6t | %50! | %8l | %v");
}
