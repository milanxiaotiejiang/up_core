//
// Created by noodles on 25-2-19.
// 舵机控制
//
#include "servo.h"
#include "logger.h"
#include "servo_protocol_parse.h"
#include <iostream>
#ifdef __linux__
#include <unistd.h>
#endif
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
#ifdef __linux__
    if (gpio != nullptr)
        gpio->init();
#endif


    if (!serial->isOpen())
        serial->open();

    // 启动监听线程
    running = true;
    receive_thread = std::thread(&Servo::processSerialData, this);
    receive_thread.detach();
}

/**
 * @brief 关闭舵机
 */
void Servo::close() {
    running = false;
    if (receive_thread.joinable())
        receive_thread.join();

    if (serial->isOpen())
        serial->close();

#ifdef __linux__
    if (gpio_enabled)
        gpio->release();
#endif
}

/**
 * @brief 使能舵机总线
 */
void Servo::enableBus() {
#ifdef __linux__
    if (gpio_enabled)
        gpio->setValue(1);
#endif
}

/**
 * @brief 失能舵机总线
 */
void Servo::disableBus() {
#ifdef __linux__
    if (gpio_enabled)
        gpio->setValue(0);
#endif
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

bool Servo::sendWaitCommand(const std::vector<uint8_t> &frame, std::vector<uint8_t> &response_data) {
    if (!serial->isOpen()) {
        Logger::error("❌ 串口未打开，无法发送数据！");
        return false;
    }

    // 使用锁来确保单线程执行
    std::lock_guard<std::mutex> lock(send_mutex);

    // 为每条命令生成一个唯一的消息 ID
    uint32_t message_id = generateMessageId();

    // 将消息 ID 添加到数据帧中，确保发送的每条消息都带有这个 ID
    std::vector<uint8_t> frame_with_id = frame;
    //    frame_with_id.push_back(static_cast<uint8_t>(message_id & 0xFF));  // 添加低字节
    //    frame_with_id.push_back(static_cast<uint8_t>((message_id >> 8) & 0xFF));  // 添加高字节

    // 创建一个条件变量和锁，用于等待此消息的响应
    std::unique_lock<std::mutex> wait_lock(mutex_);
    auto cv = std::make_unique<std::condition_variable>();
    message_conditions_[message_id] = std::move(cv);

    enableBus();
    serial->flushInput();

    // ✅ 传递正确的参数给 `write()`
    size_t bytes_written = serial->write(frame_with_id.data(), frame_with_id.size());
    disableBus();

    if (bytes_written != frame_with_id.size()) {
        Logger::error("sendCommand: Failed to write full frame. Expected: "
                      + std::to_string(frame.size()) + ", Written: " + std::to_string(bytes_written));
        return false;
    }

    // 设置最大等待时间为 500 毫秒
    auto timeout = std::chrono::steady_clock::now() + std::chrono::milliseconds(5000);

    // 使用 wait_for，等待指定时间或条件变量被通知
    if (message_conditions_[message_id]->wait_until(wait_lock, timeout, [this, message_id] {
        // 判断条件是等待接收到数据
        return received_data_.count(message_id) > 0; // 如果已接收到该 message_id 的数据
    })) {
        // 取出对应的响应数据
        auto data = received_data_[message_id];
        received_data_.erase(message_id); // 删除已处理的响应

        message_conditions_.erase(message_id); // 删除对应的条件变量


        // 将数据传递到外部传递的 response_data
        response_data = data;

        Logger::info("发送命令后收到数据：" + bytesToHex(data));

        return true;
    } else {
        // 超时，返回失败
        Logger::error("sendWaitCommand: Timeout waiting for response.");
        // 清理数据和条件变量
        message_conditions_.erase(message_id);
        return false; // 超时
    }
}

bool Servo::performSerialData(const std::vector<uint8_t> &packet) {
    return previewSerialData(packet);
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
            //            Logger::debug("❌ 未找到数据包起始标志，丢弃数据");
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

        //        Logger::debug("起始数据 " + bytesToHex(packet));

        // 假设数据帧最后两个字节是消息 ID
        //        uint32_t received_message_id = static_cast<uint32_t>(packet[packet.size() - 1]) |
        //                                       (static_cast<uint32_t>(packet[packet.size() - 2]) << 8);
        uint32_t received_message_id = message_counter; {
            std::lock_guard<std::mutex> lock(mutex_);

            // 将接收到的数据存入 map，使用消息 ID 作为键
            received_data_[received_message_id] = packet;

            // 通知对应的线程，数据已经接收完毕
            if (message_conditions_.count(received_message_id)) {
                message_conditions_[received_message_id]->notify_one();
            }
        }

        processDataPacket(packet);

        // 避免 CPU 过载
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    Logger::debug("❌ 串口监听线程已停止！");
}
