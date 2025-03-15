//
// Created by noodles on 25-2-21.
//

#include <sstream>
#include "logger.h"
#include "servo_protocol.h"

// 初始化静态成员
Logger::LogLevel Logger::logLevel_ = Logger::INFO;
std::mutex Logger::mutex_;

// 设置全局日志级别
void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    logLevel_ = level;
}

// 获取级别
Logger::LogLevel Logger::getLogLevel() {
    return logLevel_;
}

void Logger::debug(const std::string &message) {
    log(DEBUG, message);
}

void Logger::info(const std::string &message) {
    log(INFO, message);
}

void Logger::warning(const std::string &message) {
    log(WARNING, message);
}

void Logger::error(const std::string &message) {
    log(ERROR, message);
}

// 日志输出
void Logger::log(Logger::LogLevel level, const std::string &message) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (level >= logLevel_) {
        if (level == ERROR) {
            std::cerr << "[" << logLevelToString(level) << "] " << message << std::endl;
        } else {
            std::cout << "[" << logLevelToString(level) << "] " << message << std::endl;
        }
    }
}

const char *Logger::logLevelToString(Logger::LogLevel level) {
    switch (level) {
        case Logger::DEBUG:
            return "DEBUG";
        case Logger::INFO:
            return "INFO";
        case Logger::WARNING:
            return "WARNING";
        case Logger::ERROR:
            return "ERROR";
        case Logger::OFF:
            return "OFF";
        default:
            return "UNKNOWN";
    }
}
