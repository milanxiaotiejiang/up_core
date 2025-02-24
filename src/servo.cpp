//
// Created by noodles on 25-2-19.
// 舵机控制
//
#include "servo.h"
#include "logger.h"
#include <iostream>
#include <unistd.h>
#include <algorithm>
#include <iomanip>

//Servo::Servo(const gpio::GPIO &gpio, bool gpio_enabled,
//             const std::string &port,
//             uint32_t baudrate,
//             serial::Timeout timeout,
//             serial::bytesize_t bytesize,
//             serial::parity_t parity,
//             serial::stopbits_t stopbits,
//             serial::flowcontrol_t flowcontrol
//)
//        : gpio(gpio), gpio_enabled(gpio_enabled),
//          serial(port, baudrate, timeout, bytesize, parity, stopbits, flowcontrol) {}

Servo::~Servo() {
    close();
}

/**
 * @brief 初始化舵机
 */
void Servo::init() {
    if (gpio != nullptr)
        gpio->init();

    if (!serial->isOpen())
        serial->open();

    // 启动监听线程
    running = true;
    serialThread = std::thread(&Servo::processSerialData, this);
    serialThread.detach();
}

/**
 * @brief 关闭舵机
 */
void Servo::close() {
    running = false;
    if (serialThread.joinable())
        serialThread.join();

    if (serial->isOpen())
        serial->close();

    if (gpio_enabled)
        gpio->release();
}

/**
 * @brief 使能舵机总线
 */
void Servo::enableBus() {
    if (gpio_enabled)
        gpio->setValue(1);
}

/**
 * @brief 失能舵机总线
 */
void Servo::disableBus() {
    if (gpio_enabled)
        gpio->setValue(0);
}

/**
 * @brief 发送命令给舵机
 */
bool Servo::sendCommand(const std::vector<uint8_t> &frame) {
    if (!serial->isOpen()) {
        Logger::error("❌ 串口未打开，无法发送数据！");
        return false;
    }

    enableBus();
    serial->flushInput();

    // ✅ 传递正确的参数给 `write()`
    size_t bytes_written = serial->write(frame.data(), frame.size());
    disableBus();

    if (bytes_written != frame.size()) {
        Logger::error("sendCommand: Failed to write full frame. Expected: "
                      + std::to_string(frame.size()) + ", Written: " + std::to_string(bytes_written));
        return false;
    }

    return serial->waitReadable();
}

void Servo::performSerialData(const std::vector<uint8_t> &packet) {
    // 解析应答包
    uint8_t id = packet[2];
    uint8_t error = packet[4];
    std::vector<uint8_t> payload(packet.begin() + 5, packet.end() - 1);

    servo::ServoErrorInfo errorInfo = servo::getServoErrorInfo(error);
    if (errorInfo.error != servo::ServoError::NO_ERROR) {
        Logger::error("⚠️ 舵机 " + std::to_string(id) + " 返回错误: " + std::to_string(errorInfo.error)
                      + " (" + errorInfo.description + ")");
    }

    Logger::info("✅ 接收到数据包: " + bytesToHex(packet));

    // 这里可以回调处理接收到的数据，例如存储 payload 供其他线程访问

}

void Servo::processSerialData() {
    std::vector<uint8_t> buffer;

    while (running) {
        if (!serial->isOpen()) {
            Logger::error("❌ 串口未打开，无法读取数据！");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // 读取数据
        size_t available_bytes = serial->available();
        if (available_bytes == 0) {
            Logger::debug("❌ 串口未读取到数据！");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        Logger::debug("📌 串口已打开，尝试读取 " + std::to_string(available_bytes) + " 字节数据");

        std::vector<uint8_t> temp_buffer;
        size_t bytes_read = serial->read(temp_buffer, available_bytes);

        if (bytes_read == 0) {
            Logger::error("❌ 读取失败或超时！");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        buffer.insert(buffer.end(), temp_buffer.begin(), temp_buffer.end());

        Logger::debug("接收到的数据 " + bytesToHex(buffer));

        if (buffer.size() < 6) {
            Logger::debug("❌ 数据包长度不足，丢弃数据");
            buffer.clear();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // 解析数据包
        static const std::vector<uint8_t> start_flag = {0xFF, 0xFF}; // 预定义搜索模式

        auto it = std::search(buffer.begin(), buffer.end(), start_flag.begin(), start_flag.end());

        if (it == buffer.end()) {
            Logger::debug("❌ 未找到数据包起始标志，丢弃数据");
            buffer.clear();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        size_t start_index = std::distance(buffer.begin(), it);
        Logger::debug("✅ 找到起始标志，位置：" + std::to_string(start_index));

        std::vector<uint8_t> packet;
        // 复制从 FF FF 开始的数据到 packet
        packet.assign(it, buffer.end());

        // 清空缓冲区，因为数据已经复制到 packet
        buffer.clear();

        Logger::debug("起始数据 " + bytesToHex(packet));

        // 检查数据包长度是否足够
        if (packet.size() < 6) {
            Logger::debug("❌ 数据包长度不足，丢弃数据");
            continue;
        }

        // 计算校验和
        uint8_t checksum = 0;
        for (size_t i = 2; i < packet.size() - 1; i++) {
            checksum += packet[i];
        }
        checksum = ~checksum;

        // 校验失败，丢弃数据包
        if (checksum != packet.back()) {
            Logger::debug("❌ 校验失败，丢弃数据包");
            continue;
        }

        processDataPacket(packet);

        // 避免 CPU 过载
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}