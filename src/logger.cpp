//
// Created by noodles on 25-2-21.
//

#include <sstream>
#include <iomanip>
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
    std::string timestamp = getCurrentTime();
    if (level >= logLevel_) {
        if (level == ERROR) {
            std::cerr << timestamp << " [" << logLevelToString(level) << "] " << message << std::endl;
        } else if (level == INFO) {
            std::clog << timestamp << " [" << logLevelToString(level) << "] " << message << std::endl;
        } else {
            std::cout << timestamp << " [" << logLevelToString(level) << "] " << message << std::endl;
        }
    }
}

std::string Logger::getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto time_point = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&time_point);

    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

    // 获取毫秒
    auto duration = now.time_since_epoch();
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;

    ss << '.' << std::setw(3) << std::setfill('0') << milliseconds; // 补齐毫秒为三位数

    return ss.str();
}

const char *Logger::logLevelToString(Logger::LogLevel level) {
    switch (level) {
        case Logger::DEBUG:
            return " DEBUG ";
        case Logger::INFO:
            return " INFO  ";
        case Logger::WARNING:
            return "WARNING";
        case Logger::ERROR:
            return " ERROR ";
        case Logger::OFF:
            return " OFF   ";
        default:
            return "UNKNOWN";
    }
}
