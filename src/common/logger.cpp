#include <QDebug>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

using namespace spdlog;

void init_logger()
{
    auto stdout_sink = std::make_shared<sinks::stdout_color_sink_mt>();
    stdout_sink->set_level(spdlog::level::trace);
    size_t max_size_bytes = 1 * 1024 * 1024;
    size_t max_files = 3;
    auto rotating_sink = std::make_shared<sinks::rotating_file_sink_mt>("logs/qrammer.log",
                                                                        max_size_bytes,
                                                                        max_files);
    std::vector<sink_ptr> sinks{stdout_sink, rotating_sink};
    spdlog::logger lgr("multi_sink", {stdout_sink, rotating_sink});

    spdlog::set_default_logger(std::make_shared<logger>(lgr));
    spdlog::set_pattern("%Y-%m-%dT%T.%f%z | %6t | %45! | %8l | %v");
}
