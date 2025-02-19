//
// Created by noodles on 25-2-19.
// 舵机控制
//
#include "servo.h"
#include <iostream>
#include <unistd.h>

/**
 * @brief 舵机构造函数
 */
Servo::Servo(const std::string &serial_device, int baudrate, int gpio_chip, int gpio_line)
        : serial(serial_device, baudrate), gpio(gpio_chip, gpio_line) {}

Servo::~Servo() {
    close();
}

/**
 * @brief 初始化舵机
 */
void Servo::init() {
    gpio.init();
    serial.open();
}

/**
 * @brief 关闭舵机
 */
void Servo::close() {
    serial.close();
    gpio.release();
}

/**
 * @brief 使能舵机总线
 */
void Servo::enableBus() {
    gpio.setValue(1);
}

/**
 * @brief 失能舵机总线
 */
void Servo::disableBus() {
    gpio.setValue(0);
}

/**
 * @brief 发送命令给舵机
 */
bool Servo::sendCommand(uint8_t id, uint8_t cmd, const std::vector<uint8_t> &data) {
    std::vector<uint8_t> frame = ServoProtocol::createFrame(id, cmd, data);

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

/**
 * @brief 读取舵机返回数据
 */
bool Servo::readResponse(uint8_t id, uint8_t &status, std::vector<uint8_t> &payload) {
    // **等待可读数据，避免 available() 直接返回 0**
    if (!serial.waitReadable()) {
        return false;
    }

    // **检查是否有足够数据可读**
    size_t available_bytes = serial.available();
    if (available_bytes < 6) {
        return false;
    }

    // **正确读取数据**
    std::vector<uint8_t> response;
    response.resize(available_bytes);  // 这里确保 vector 长度正确
    size_t bytes_read = serial.read(response, available_bytes);

    // **检查是否读取完整**
    if (bytes_read == 0 || bytes_read < 6) {
        return false;
    }

    return ServoProtocol::parseResponse(response, id, status, payload);
}

/**
 * @brief 设置舵机模式
 */
void Servo::setMode(uint8_t id, uint16_t mode) {
    std::vector<uint8_t> data = {0x06, (uint8_t) (mode ? 0 : 0xff), (uint8_t) (mode ? 0 : 0x03)};
    sendCommand(id, 0x03, data);
}

/**
 * @brief 设置舵机目标角度
 */
void Servo::setAngle(uint8_t id, uint16_t angle, uint16_t speed) {
    std::vector<uint8_t> data = {0x1E, (uint8_t) angle, (uint8_t) (angle >> 8), (uint8_t) speed,
                                 (uint8_t) (speed >> 8)};
    sendCommand(id, 0x03, data);
}

/**
 * @brief 设置舵机转速
 */
void Servo::setSpeed(uint8_t id, int16_t speed) {
    uint16_t temp = static_cast<uint16_t>(abs(speed));
    uint8_t high_byte = static_cast<uint8_t>(temp >> 8);

    if (speed < 0) {
        high_byte |= 0x04;  // 符号位
    }

    std::vector<uint8_t> data = {
            0x20,
            static_cast<uint8_t>(temp),  // 低字节
            high_byte                     // 高字节（带符号位）
    };

    sendCommand(id, 0x03, data);
}

/**
 * @brief 获取舵机当前位置
 */
int Servo::getPosition(uint8_t id) {
    std::vector<uint8_t> payload;
    uint8_t status;
    sendCommand(id, 0x02, {0x24, 2});
    if (readResponse(id, status, payload) && payload.size() == 2) {
        return payload[0] | (payload[1] << 8);
    }
    return -1;
}
