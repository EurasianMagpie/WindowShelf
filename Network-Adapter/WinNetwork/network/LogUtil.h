// log_util.h
#ifndef _____X_COMMON_UTIL_LOG_HEADER_____
#define _____X_COMMON_UTIL_LOG_HEADER_____

#define USE_SPD_LOG     1

#if USE_SPD_LOG

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

#define LOGI spdlog::info
#define LOGW spdlog::warn
#define LOGC spdlog::critical
#define LOGE spdlog::error
#define LOGD spdlog::debug

static void init_log(const char* filename)
{
    // Create a file rotating logger with 2 MB size max and 2 rotated files
    auto max_size = 1048576 * 2;
    auto max_files = 2;
    auto logger = spdlog::rotating_logger_mt("default_logger", filename, max_size, max_files);
    spdlog::set_default_logger(logger);
}

#else 

#define LOGI sizeof
#define LOGW sizeof
#define LOGC sizeof
#define LOGE sizeof
#define LOGD sizeof

static void init_log(const char* filename) {}

#endif

namespace LogUtil {
    void Init(const char* filename) {
        init_log(filename);
    }
}

#endif//_____X_COMMON_UTIL_LOG_HEADER_____