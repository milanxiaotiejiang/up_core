//
// Created by noodles on 25-3-15.
//

#include <iostream>
#include <sstream>
#include <iomanip>
#include "servo_protocol_parse.h"
#include "logger.h"

std::string bytesToHex(const std::vector<uint8_t> &data) {
    std::ostringstream oss;
    oss << "📩 ";
    for (uint8_t byte: data) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int) byte << " ";
    }
    return oss.str();
}

int singleByteToInt(uint8_t byte) {
    return static_cast<int>(byte);
}

int doubleByteToInt(uint8_t lowByte, uint8_t highByte) {
    // 将高字节左移8位，再与低字节相加，恢复原始值
    return (static_cast<int>(highByte) << 8) | static_cast<int>(lowByte);
}

float combineSpeed(uint8_t lowByte, uint8_t highByte) {
    // 将高字节左移8位，再与低字节相加，恢复原始速度值
    int speed = doubleByteToInt(lowByte, highByte);
    return speed * 62.0f / 1023.0f;
}

float combinePosition(uint8_t lowByte, uint8_t highByte) {
    // 将高字节左移8位，再与低字节相加，恢复原始速度值
    int position = doubleByteToInt(lowByte, highByte);
    return position * 300.0f / 1023.0f;
}

bool previewSerialData(const std::vector<uint8_t> &packet) {

    // 解析应答包
    // 检查数据包长度是否足够
    if (packet.size() < 6) {
        Logger::debug("❌ 数据包长度不足，丢弃数据");
        return false;
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
        return false;
    }

    uint8_t id = packet[2];
    uint8_t length = packet[3];
    uint8_t error = packet[4];
    std::vector<uint8_t> payload(packet.begin() + 5, packet.end() - 1);

    servo::ServoErrorInfo errorInfo = servo::getServoErrorInfo(error);
    if (errorInfo.error != servo::ServoError::NO_ERROR) {
        Logger::warning("⚠️ 舵机 " + std::to_string(id) + " 返回错误: " + std::to_string(errorInfo.error)
                        + " (" + errorInfo.description + ")");
    }

    Logger::info("✅ 接收到数据包: " + bytesToHex(payload));

    // 这里可以回调处理接收到的数据，例如存储 payload 供其他线程访问
    return errorInfo.error == servo::ServoError::NO_ERROR;
}

std::pair<bool, std::pair<int, int>> performExtractID(const std::vector<uint8_t> &packet) {

    // 解析应答包
    // 检查数据包长度是否足够
    if (packet.size() < 6) {
        Logger::debug("❌ 数据包长度不足，丢弃数据");
        return std::make_pair(false, std::make_pair(0, 0));
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
        return std::make_pair(false, std::make_pair(0, 0));
    }

    uint8_t id = packet[2];
    uint8_t length = packet[3];
    uint8_t error = packet[4];

    servo::ServoErrorInfo errorInfo = servo::getServoErrorInfo(error);
    if (errorInfo.error != servo::ServoError::NO_ERROR) {
        Logger::warning("⚠️ 舵机 " + std::to_string(id) + " 返回错误: " + std::to_string(errorInfo.error)
                        + " (" + errorInfo.description + ")");
        return std::make_pair(true, std::make_pair(id, error));
    }

    return std::make_pair(true, std::make_pair(id, error));
}

namespace servo {
// 打印对应枚举的名称和值
    std::string eePROMValue(EEPROM eeprom, uint8_t value) {
        std::ostringstream oss;
        switch (eeprom) {
            case EEPROM::MODEL_NUMBER_L:
                oss << "低位型号号码: ";
                break;
            case EEPROM::MODEL_NUMBER_H:
                oss << "高位型号号码: ";
                break;
            case EEPROM::VERSION:
                oss << "软件版本: ";
                break;
            case EEPROM::ID:
                oss << "ID: ";
                break;
            case EEPROM::BAUDRATE:
                oss << "波特率: ";
                break;
            case EEPROM::RETURN_DELAY_TIME:
                oss << "返回延迟时间: ";
                break;
            case EEPROM::CW_ANGLE_LIMIT_L:
                oss << "顺时针角度限制（L）: ";
                break;
            case EEPROM::CW_ANGLE_LIMIT_H:
                oss << "顺时针角度限制（H）: ";
                break;
            case EEPROM::CCW_ANGLE_LIMIT_L:
                oss << "逆时针角度限制（L）: ";
                break;
            case EEPROM::CCW_ANGLE_LIMIT_H:
                oss << "逆时针角度限制（H）: ";
                break;
            case EEPROM::MAX_TEMPERATURE:
                oss << "最高温度上限: ";
                break;
            case EEPROM::MIN_VOLTAGE:
                oss << "最低输入电压: ";
                break;
            case EEPROM::MAX_VOLTAGE:
                oss << "最高输入电压: ";
                break;
            case EEPROM::MAX_TORQUE_L:
                oss << "最大扭矩（L）: ";
                break;
            case EEPROM::MAX_TORQUE_H:
                oss << "最大扭矩（H）: ";
                break;
            case EEPROM::STATUS_RETURN_LEVEL:
                oss << "应答状态级别: ";
                break;
            case EEPROM::ALARM_LED:
                oss << "LED闪烁: ";
                break;
            case EEPROM::ALARM_SHUTDOWN:
                oss << "卸载条件: ";
                break;
            default:
                oss << "Unknown: ";
                break;
        }
        // 输出十六进制
        oss << "Hex: 0x" << std::hex << static_cast<int>(value) << " ";  // 十六进制格式
        // 输出十进制
        oss << "Decimal: " << std::dec << static_cast<int>(value);  // 恢复为十进制格式
        return oss.str();  // 返回格式化后的字符串
    }

    std::unordered_map<EEPROM, uint8_t> parseEEPROMData(const std::vector<uint8_t> &data, EEPROM start) {
        EEPROM eepromFields[] = {
                EEPROM::MIN_VOLTAGE, EEPROM::MODEL_NUMBER_L, EEPROM::MODEL_NUMBER_H,
                EEPROM::VERSION, EEPROM::ID, EEPROM::BAUDRATE, EEPROM::RETURN_DELAY_TIME,
                EEPROM::CW_ANGLE_LIMIT_L, EEPROM::CW_ANGLE_LIMIT_H, EEPROM::CCW_ANGLE_LIMIT_L,
                EEPROM::CCW_ANGLE_LIMIT_H, EEPROM::MAX_TEMPERATURE, EEPROM::MIN_VOLTAGE,
                EEPROM::MAX_VOLTAGE, EEPROM::MAX_TORQUE_L, EEPROM::MAX_TORQUE_H,
                EEPROM::STATUS_RETURN_LEVEL, EEPROM::ALARM_LED, EEPROM::ALARM_SHUTDOWN
        };

        std::unordered_map<EEPROM, uint8_t> unorderedMap;
        bool startPrinting = false;
        size_t dataIndex = 0;

        for (const EEPROM &eepromField: eepromFields) {
            if (eepromField == start) {
                startPrinting = true;
            }
            if (startPrinting && dataIndex < data.size()) {
                Logger::debug(eePROMValue(eepromField, data[dataIndex]));
                unorderedMap[eepromField] = data[dataIndex];
                dataIndex++;
            }
        }

        return unorderedMap;
    }

    // 打印对应的枚举名称和值
    std::string ramValue(RAM ram, uint8_t value) {
        std::ostringstream oss;
        switch (ram) {
            case RAM::TORQUE_ENABLE:
                oss << "扭矩开关: ";
                break;
            case RAM::LED:
                oss << "LED开关: ";
                break;
            case RAM::CW_COMPLIANCE_MARGIN:
                oss << "顺时针不灵敏区: ";
                break;
            case RAM::CCW_COMPLIANCE_MARGIN:
                oss << "逆时针不灵敏区: ";
                break;
            case RAM::CW_COMPLIANCE_SLOPE:
                oss << "顺时针比例系数: ";
                break;
            case RAM::CCW_COMPLIANCE_SLOPE:
                oss << "逆时针比例系数: ";
                break;
            case RAM::GOAL_POSITION_L:
                oss << "目标位置（L）: ";
                break;
            case RAM::GOAL_POSITION_H:
                oss << "目标位置（H）: ";
                break;
            case RAM::MOVING_SPEED_L:
                oss << "运行速度（L）: ";
                break;
            case RAM::MOVING_SPEED_H:
                oss << "运行速度（H）: ";
                break;
            case RAM::ACCELERATION:
                oss << "加速度: ";
                break;
            case RAM::DECELERATION:
                oss << "减速度: ";
                break;
            case RAM::PRESENT_POSITION_L:
                oss << "当前位置（L）: ";
                break;
            case RAM::PRESENT_POSITION_H:
                oss << "当前位置（H）: ";
                break;
            case RAM::PRESENT_SPEED_L:
                oss << "当前速度（L）: ";
                break;
            case RAM::PRESENT_SPEED_H:
                oss << "当前速度（H）: ";
                break;
            case RAM::PRESENT_LOAD_L:
                oss << "当前负载（L）: ";
                break;
            case RAM::PRESENT_LOAD_H:
                oss << "当前负载（H）: ";
                break;
            case RAM::PRESENT_VOLTAGE:
                oss << "当前电压: ";
                break;
            case RAM::TEMPERATURE:
                oss << "当前温度: ";
                break;
            case RAM::REG_WRITE:
                oss << "REG WRITE标志: ";
                break;
            case RAM::MOVING:
                oss << "运行中标志: ";
                break;
            case RAM::LOCK:
                oss << "锁标志: ";
                break;
            case RAM::MIN_PWM_L:
                oss << "最小PWM（L）: ";
                break;
            case RAM::MIN_PWM_H:
                oss << "最小PWM（H）: ";
                break;
            default:
                oss << "Unknown: ";
                break;
        }
        // 输出十六进制
        oss << "Hex: 0x" << std::hex << static_cast<int>(value) << " ";  // 十六进制格式
        // 输出十进制
        oss << "Decimal: " << std::dec << static_cast<int>(value);  // 恢复为十进制格式
        return oss.str();  // 返回格式化后的字符串
    }

    std::unordered_map<RAM, uint8_t> parseRAMData(const std::vector<uint8_t> &data, RAM start) {
        // 假设输入数据是按照 RAM 枚举的顺序排列
        RAM ramFields[] = {
                RAM::TORQUE_ENABLE, RAM::LED, RAM::CW_COMPLIANCE_MARGIN, RAM::CCW_COMPLIANCE_MARGIN,
                RAM::CW_COMPLIANCE_SLOPE, RAM::CCW_COMPLIANCE_SLOPE, RAM::GOAL_POSITION_L, RAM::GOAL_POSITION_H,
                RAM::MOVING_SPEED_L, RAM::MOVING_SPEED_H, RAM::ACCELERATION, RAM::DECELERATION,
                RAM::PRESENT_POSITION_L, RAM::PRESENT_POSITION_H, RAM::PRESENT_SPEED_L, RAM::PRESENT_SPEED_H,
                RAM::PRESENT_LOAD_L, RAM::PRESENT_LOAD_H, RAM::PRESENT_VOLTAGE, RAM::TEMPERATURE,
                RAM::REG_WRITE, RAM::MOVING, RAM::LOCK, RAM::MIN_PWM_L, RAM::MIN_PWM_H
        };

        std::unordered_map<RAM, uint8_t> unorderedMap;
        bool startPrinting = false;
        size_t dataIndex = 0;

        for (const RAM &ramField: ramFields) {
            if (ramField == start) {
                startPrinting = true;
            }
            if (startPrinting && dataIndex < data.size()) {
                Logger::debug(ramValue(ramField, data[dataIndex]));
                unorderedMap[ramField] = data[dataIndex];
                dataIndex++;
            }
        }

        return unorderedMap;
    }
}