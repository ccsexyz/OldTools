#ifndef POLLERTEST_LOGGER_H
#define POLLERTEST_LOGGER_H

#include "BlockingQueue.h"
#include <string>

#define debug(fmt, ...)                                                        \
    Liby::Logger::getLogger().log(Logger::LogLevel::DEBUG, __FILE__, __LINE__, \
                                  __func__, fmt, ##__VA_ARGS__)
#define info(fmt, ...)                                                         \
    Liby::Logger::getLogger().log(Logger::LogLevel::INFO, __FILE__, __LINE__,  \
                                  __func__, fmt, ##__VA_ARGS__)
#define warn(fmt, ...)                                                         \
    Liby::Logger::getLogger().log(Logger::LogLevel::WARN, __FILE__, __LINE__,  \
                                  __func__, fmt, ##__VA_ARGS__)
#define error(fmt, ...)                                                        \
    Liby::Logger::getLogger().log(Logger::LogLevel::ERROR, __FILE__, __LINE__, \
                                  __func__, fmt, ##__VA_ARGS__)
#define fatal(fmt, ...)                                                        \
    Liby::Logger::getLogger().log(Logger::LogLevel::FATAL, __FILE__, __LINE__, \
                                  __func__, fmt, ##__VA_ARGS__);               \
    ::exit(1)
#define debugif(condition, ...)                                                \
    do {                                                                       \
        if ((condition))                                                       \
            debug(__VA_ARGS__);                                                \
    } while (0)
#define infoif(condition, ...)                                                 \
    do {                                                                       \
        if ((condition))                                                       \
            info(__VA_ARGS__);                                                 \
    } while (0)
#define warnif(condition, ...)                                                 \
    do {                                                                       \
        if ((condition))                                                       \
            warn(__VA_ARGS__);                                                 \
    } while (0)
#define errorif(condition, ...)                                                \
    do {                                                                       \
        if ((condition))                                                       \
            error(__VA_ARGS__);                                                \
    } while (0)
#define fatalif(condition, ...)                                                \
    do {                                                                       \
        if ((condition))                                                       \
            fatal(__VA_ARGS__);                                                \
    } while (0)

namespace Liby {
class Logger final {
public:
    enum class LogLevel { DEBUG, INFO, WARN, ERROR, FATAL };

    Logger(const Logger &) = delete;
    Logger(Logger &&) = delete;
    Logger &operator=(const Logger &) = delete;

    static Logger &getLogger();

    LogLevel getLevel() const { return level_; }
    void setLevel_(LogLevel level) { level_ = level; }
    static const char *getLevelString(LogLevel level);
    static void setLevel(LogLevel level) {
        Logger::getLogger().setLevel_(level);
    }

    void log(LogLevel level, const char *file, const int line, const char *func,
             const char *fmt...);

    static std::string getTimeStr();

private:
    Logger(LogLevel level = LogLevel::INFO);

    void logger_thread();

private:
    LogLevel level_;
    BlockingQueue<std::string> queue_;
};
}

#endif // POLLERTEST_LOGGER_H
