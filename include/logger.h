//
// Created by noodles on 25-2-21.
//

#ifndef UP_CORE_LOGGER_H
#define UP_CORE_LOGGER_H

#include <iostream>
#include <string>
#include <mutex>
#include <vector>
#include "servo_protocol.h"

class Logger {
public:
    enum LogLevel {
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        OFF // 关闭所有日志
    };

    // 设置全局日志级别
    static void setLogLevel(LogLevel level);

    // 获取级别
    static LogLevel getLogLevel();

    static void debug(const std::string &message);

    static void info(const std::string &message);

    static void warning(const std::string &message);

    static void error(const std::string &message);

private:
    static LogLevel logLevel_;
    static std::mutex mutex_;

    // 日志输出
    static void log(LogLevel level, const std::string &message);

    static const char *logLevelToString(LogLevel level);
};

#endif //UP_CORE_LOGGER_H
