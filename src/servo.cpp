//
// Created by noodles on 25-2-19.
// 舵机控制
//
#include "servo.h"
#include <iostream>
#include <unistd.h>
#include <algorithm>
#include <iomanip>

Servo::Servo(const std::string &serial_device, int baudrate)
        : serial(serial_device, baudrate, serial::Timeout::simpleTimeout(1000)),
          gpio(0, 0), gpio_enabled(false) {}

Servo::Servo(const std::string &serial_device, int baudrate, int gpio_chip, int gpio_line)
        : serial(serial_device, baudrate, serial::Timeout::simpleTimeout(1000)),
          gpio(gpio_chip, gpio_line), gpio_enabled(true) {}


Servo::~Servo() {
    close();
}

/**
 * @brief 初始化舵机
 */
void Servo::init() {
    if (gpio_enabled)
        gpio.init();

    if (!serial.isOpen())
        serial.open();

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

    if (serial.isOpen())
        serial.close();

    if (gpio_enabled)
        gpio.release();
}

/**
 * @brief 使能舵机总线
 */
void Servo::enableBus() {
    if (gpio_enabled)
        gpio.setValue(1);
}

/**
 * @brief 失能舵机总线
 */
void Servo::disableBus() {
    if (gpio_enabled)
        gpio.setValue(0);
}

const serial::Serial &Servo::getSerial() const {
    return serial;
}

/**
 * @brief 发送命令给舵机
 */
bool Servo::sendCommand(const std::vector<uint8_t> &frame) {
    if (!serial.isOpen()) {
        std::cerr << "❌ 串口未打开，无法发送数据！" << std::endl;
        return false;
    }

    enableBus();
    serial.flushInput();

    // ✅ 传递正确的参数给 `write()`
    size_t bytes_written = serial.write(frame.data(), frame.size());
    disableBus();

    if (bytes_written != frame.size()) {
        std::cerr << "sendCommand: Failed to write full frame. Expected: "
                  << frame.size() << ", Written: " << bytes_written << std::endl;
        return false;
    }

    return serial.waitReadable();
}

void printHexData(const std::string &tag, const std::vector<uint8_t> &data) {
    std::cout << "📩 " << tag << "（HEX）：";
    for (uint8_t byte: data) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int) byte << " ";
    }
    std::cout << std::dec << std::endl;
}

void Servo::processSerialData() {
    std::vector<uint8_t> buffer;

    while (running) {
        if (!serial.isOpen()) {
            std::cerr << "❌ 串口未打开，无法读取数据！" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // 读取数据
        size_t available_bytes = serial.available();
        if (available_bytes == 0) {
            std::cerr << "❌ 串口未读取到数据！" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        std::cout << "📌 串口已打开，尝试读取 " << available_bytes << " 字节数据" << std::endl;

        std::vector<uint8_t> temp_buffer(available_bytes);
        size_t bytes_read = serial.read(temp_buffer, available_bytes);

        if (bytes_read == 0) {
            std::cerr << "❌ 读取失败或超时！" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        buffer.insert(buffer.end(), temp_buffer.begin(), temp_buffer.end());

        printHexData("接收到的数据", buffer);

        if (buffer.size() < 6) {
            std::cerr << "❌ 数据包长度不足，丢弃数据" << std::endl;
            buffer.clear();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // 解析数据包
        static const std::vector<uint8_t> start_flag = {0xFF, 0xFF}; // 预定义搜索模式

        auto it = std::search(buffer.begin(), buffer.end(), start_flag.begin(), start_flag.end());

        if (it == buffer.end()) {
            std::cerr << "❌ 未找到数据包起始标志，丢弃数据" << std::endl;
            buffer.clear();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        size_t start_index = std::distance(buffer.begin(), it);
        std::cout << "✅ 找到起始标志，位置：" << start_index << std::endl;

        std::vector<uint8_t> packet;
        // 复制从 FF FF 开始的数据到 packet
        packet.assign(it, buffer.end());

        printHexData("起始数据", packet);

        // 计算校验和
        uint8_t checksum = 0;
        for (size_t i = 2; i < packet.size() - 1; i++) {
            checksum += packet[i];
        }
        checksum = ~checksum;

        // 校验失败，丢弃数据包
        if (checksum != packet.back()) {
            std::cerr << "❌ 校验失败，丢弃数据包" << std::endl;
            continue;
        }

        // 解析应答包
        uint8_t id = packet[2];
        uint8_t error = packet[4];
        std::vector<uint8_t> payload(packet.begin() + 5, packet.end() - 1);

        // 显示接收数据
        std::cout << "✅ 解析到数据包: ";
        for (uint8_t byte: packet) {
            printf("%02X ", byte);
        }
        std::cout << std::endl;

        // 解析错误码
        if (error != 0) {
            std::cerr << "⚠️ 舵机 " << static_cast<int>(id) << " 返回错误: " << std::hex << static_cast<int>(error)
                      << std::dec << std::endl;
        }

        // 这里可以回调处理接收到的数据，例如存储 payload 供其他线程访问


        buffer.clear();
        // 避免 CPU 过载
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}