//
// Created by noodles on 25-2-19.
//

#include "servo_protocol.h"

#include <numeric>
#include "unordered_map"
#include "logger.h"
#include <cmath>

#include "servo_protocol_parse.h"


namespace servo {
    Base::Base(uint8_t id) {
        id_ = id;
    }

    inline uint8_t toByte(bool value) {
        return static_cast<uint8_t>(value ? 1 : 0);
    }

    uint8_t baudrateToAddress4(uint32_t baud) {
        static const std::unordered_map<uint32_t, uint8_t> baudrate_map = {
                {1000000, 0x01},
                {500000,  0x03},
                {250000,  0x07},
                {115200,  0x10}, // 实际值是 117647.1
                {57600,   0x22}, // 实际值是 57142.9
                {19200,   0x67} // 实际值是 19230.8
        };

        auto it = baudrate_map.find(baud);
        if (it != baudrate_map.end()) {
            return it->second;
        }

        return 0x01; // 默认恢复 1M
    }

    // 计算校验和
    uint8_t calculateChecksum(const std::vector<uint8_t> &packet) {
        uint16_t sum = std::accumulate(packet.begin() + 2, packet.end(), 0);
        return static_cast<uint8_t>(~sum);
    }

    //    std::vector<uint8_t> Base::buildShortPacket(uint8_t address, const std::vector<uint8_t> &data) {
    //        std::vector<uint8_t> packet;
    //
    //        packet.push_back(id_); // ID
    //        packet.push_back(address);
    //        if (!data.empty()) {
    //            packet.insert(packet.end(), data.begin(), data.end());
    //        }
    //
    //        return packet;
    //    }

    std::vector<uint8_t> Base::buildShortPacket(int write_length, std::vector<uint8_t> commandData) {
        std::vector<uint8_t> payload(commandData.end() - 1 - write_length, commandData.end() - 1);
        return payload;
    }

    std::vector<uint8_t> Base::buildCommandPacket(ORDER command, uint8_t address, const std::vector<uint8_t> &data) {
        std::vector<uint8_t> packet;
        packet.push_back(0xFF); // 固定字头
        packet.push_back(0xFF);
        packet.push_back(id_); // ID

        // 数据长度 = 参数数量 + 2 + 指令(1)
        uint8_t length = static_cast<uint8_t>(data.size() + 2);
        if (!(command == ORDER::ACTION || command == ORDER::PING || command == ORDER::RESET ||
              command == ORDER::BOOTLOADER)) {
            // ACTION、PING 指令不需要地址
            length += 1;
        }
        packet.push_back(length);

        // 插入指令类型
        packet.push_back(static_cast<uint8_t>(command));

        // 仅 `WRITE DATA` 和 `REG WRITE` 需要地址字段
        if (command == ORDER::WRITE_DATA || command == ORDER::REG_WRITE || command == ORDER::READ_DATA) {
            packet.push_back(address);
        }

        // 插入参数（如果有）
        if (!data.empty()) {
            packet.insert(packet.end(), data.begin(), data.end());
        }

        // 计算校验和
        packet.push_back(calculateChecksum(packet)); // 校验和

        // Logger::debug("发送指令包: " + bytesToHex(packet));

        return packet;
    }

    std::vector<uint8_t> Base::buildPingPacket() {
        return buildCommandPacket(ORDER::PING, 0x00, {});
    }

    std::vector<uint8_t> Base::buildReadPacket(uint8_t address, uint8_t read_length) {
        return buildCommandPacket(ORDER::READ_DATA, address, {read_length});
    }

    std::vector<uint8_t> Base::buildWritePacket(uint8_t address, const std::vector<uint8_t> &data) {
        return buildCommandPacket(ORDER::WRITE_DATA, address, data);
    }

    std::vector<uint8_t> Base::buildRegWritePacket(uint8_t address, const std::vector<uint8_t> &data) {
        return buildCommandPacket(ORDER::REG_WRITE, address, data);
    }

    std::vector<uint8_t> Base::buildActionPacket() {
        return buildCommandPacket(ORDER::ACTION, 0x00, {});
    }

    std::vector<uint8_t> Base::buildResetPacket() {
        return buildCommandPacket(ORDER::RESET, 0x00, {});
    }

    std::vector<uint8_t> Base::buildResetBootLoader() {
        return buildCommandPacket(ORDER::BOOTLOADER, 0x00, {});
    }

    std::vector<uint8_t>
    Base::buildSyncWritePacket(uint8_t address, int write_length, std::vector<ServoProtocol> &protocols,
                               const std::function<std::vector<uint8_t>(ServoProtocol &data, int position)> &func) {
        std::vector<uint8_t> packet;

        packet.push_back(write_length);

        for (size_t i = 0; i < protocols.size(); ++i) {
            // 调用 func 并获取结果
            std::vector<uint8_t> result = func(protocols[i], i);

            auto payload = buildShortPacket(write_length, result);

            // 判断 result 的长度是否与 write_Length 匹配
            if (payload.size() != write_length) {
                throw std::runtime_error("Error: Length of result does not match write_Length.");
            }

            // 确保 result 是 std::vector<uint8_t> 类型
            packet.insert(packet.end(), payload.begin(), payload.end());
        }

        return buildCommandPacket(ORDER::SYNC_WRITE, address, packet);
    }

    ServoEEPROM::ServoEEPROM(uint8_t id) : Base(id) {
    }

    // 读取软件版本
    std::vector<uint8_t> ServoEEPROM::buildGetSoftwareVersion() {
        return buildCommandPacket(ORDER::READ_DATA, static_cast<uint8_t>(EEPROM::VERSION), {0x01});
    }

    // 读取舵机 ID
    std::vector<uint8_t> ServoEEPROM::buildGetID() {
        return buildCommandPacket(ORDER::READ_DATA, static_cast<uint8_t>(EEPROM::ID), {0x01});
    }

    // 读取波特率
    std::vector<uint8_t> ServoEEPROM::buildGetBaudrate() {
        return buildCommandPacket(ORDER::READ_DATA, static_cast<uint8_t>(EEPROM::BAUDRATE), {0x01});
    }

    // 读取返回延迟时间
    std::vector<uint8_t> ServoEEPROM::buildGetReturnDelayTime() {
        return buildCommandPacket(ORDER::READ_DATA, static_cast<uint8_t>(EEPROM::RETURN_DELAY_TIME), {0x01});
    }

    // 读取顺时针角度限制
    std::vector<uint8_t> ServoEEPROM::buildGetCwAngleLimit() {
        return buildCommandPacket(ORDER::READ_DATA, static_cast<uint8_t>(EEPROM::CW_ANGLE_LIMIT_L), {0x04});
    }

    // 读取逆时针角度限制
    std::vector<uint8_t> ServoEEPROM::buildGetCcwAngleLimit() {
        return buildCommandPacket(ORDER::READ_DATA, static_cast<uint8_t>(EEPROM::CCW_ANGLE_LIMIT_L), {0x02});
    }

    // 读取角度限制
    std::vector<uint8_t> ServoEEPROM::buildGetAngleLimit() {
        return buildCommandPacket(ORDER::READ_DATA, static_cast<uint8_t>(EEPROM::CW_ANGLE_LIMIT_L), {0x04});
    }

    // 读取最高温度上限
    std::vector<uint8_t> ServoEEPROM::buildGetMaxTemperature() {
        return buildCommandPacket(ORDER::READ_DATA, static_cast<uint8_t>(EEPROM::MAX_TEMPERATURE), {0x01});
    }

    // 读取最低输入电压
    std::vector<uint8_t> ServoEEPROM::buildGetMinVoltage() {
        return buildCommandPacket(ORDER::READ_DATA, static_cast<uint8_t>(EEPROM::MIN_VOLTAGE), {0x01});
    }

    // 读取最高输入电压
    std::vector<uint8_t> ServoEEPROM::buildGetMaxVoltage() {
        return buildCommandPacket(ORDER::READ_DATA, static_cast<uint8_t>(EEPROM::MAX_VOLTAGE), {0x01});
    }

    // 读取输入电压范围
    std::vector<uint8_t> ServoEEPROM::buildGetVoltageRange() {
        return buildCommandPacket(ORDER::READ_DATA, static_cast<uint8_t>(EEPROM::MIN_VOLTAGE), {0x02});
    }

    // 读取最大扭矩
    std::vector<uint8_t> ServoEEPROM::buildGetMaxTorque() {
        return buildCommandPacket(ORDER::READ_DATA, static_cast<uint8_t>(EEPROM::MAX_TORQUE_L), {0x02});
    }

    // 读取应答状态级别
    std::vector<uint8_t> ServoEEPROM::buildGetStatusReturnLevel() {
        return buildCommandPacket(ORDER::READ_DATA, static_cast<uint8_t>(EEPROM::STATUS_RETURN_LEVEL), {0x01});
    }

    // 读取 LED 闪烁报警条件
    std::vector<uint8_t> ServoEEPROM::buildGetAlarmLED() {
        return buildCommandPacket(ORDER::READ_DATA, static_cast<uint8_t>(EEPROM::ALARM_LED), {0x01});
    }

    // 读取卸载条件
    std::vector<uint8_t> ServoEEPROM::buildGetAlarmShutdown() {
        return buildCommandPacket(ORDER::READ_DATA, static_cast<uint8_t>(EEPROM::ALARM_SHUTDOWN), {0x01});
    }


    // 设置舵机 ID
    std::vector<uint8_t> ServoEEPROM::buildSetID(uint8_t new_id) {
        return buildCommandPacket(ORDER::WRITE_DATA, static_cast<uint8_t>(EEPROM::ID), {new_id});
    }

    // 设置波特率
    std::vector<uint8_t> ServoEEPROM::buildSetBaudrate(uint32_t baud) {
        uint8_t address4 = baudrateToAddress4(baud);
        return buildCommandPacket(ORDER::WRITE_DATA, static_cast<uint8_t>(EEPROM::BAUDRATE), {address4});
    }

    // 设置舵机返回数据的延迟时间（单位：微秒）。
    std::vector<uint8_t> ServoEEPROM::buildSetReturnDelayTime(uint8_t delay) {
        if (delay > 254) {
            throw std::out_of_range("返回延迟时间超出范围 (0 - 254)");
        }
        return buildCommandPacket(ORDER::WRITE_DATA, static_cast<uint8_t>(EEPROM::RETURN_DELAY_TIME), {delay});
    }

    // 设定角度限制
    std::vector<uint8_t> ServoEEPROM::buildSetAngleLimit(uint16_t min_angle, uint16_t max_angle) {
        if (min_angle >= max_angle) {
            throw std::invalid_argument("CW_ANGLE_LIMIT must be smaller than CCW_ANGLE_LIMIT.");
        }
        if (max_angle > 300) {
            throw std::out_of_range("Angle out of range (0 - 300°)");
        }

        // 角度转换为寄存器值 (10-bit 分辨率)
        uint16_t min_reg = (min_angle * 1023) / 300;
        uint16_t max_reg = (max_angle * 1023) / 300;

        return buildCommandPacket(ORDER::WRITE_DATA, static_cast<uint8_t>(EEPROM::CCW_ANGLE_LIMIT_L), {
                static_cast<uint8_t>(max_reg & 0xFF), // 低 8-bit
                static_cast<uint8_t>(max_reg >> 8) // 高 8-bit
        });
    }

    // 设定最大温度
    std::vector<uint8_t> ServoEEPROM::buildSetMaxTemperature(int32_t temperature) {
        if (temperature < 0 || temperature > 80) {
            throw std::out_of_range("Temperature out of range (0 - 80°C)");
        }

        uint8_t register_value = static_cast<uint8_t>(temperature); // 直接存储温度数值

        return buildCommandPacket(ORDER::WRITE_DATA, static_cast<uint8_t>(EEPROM::MAX_TEMPERATURE), {register_value});
    }

    // 设定电压范围
    std::vector<uint8_t> ServoEEPROM::buildSetVoltageRange(float min_voltage, float max_voltage) {
        // 电压值转换 (V → 存储值: V * 10)
        uint8_t min_value = static_cast<uint8_t>(min_voltage * 10);
        uint8_t max_value = static_cast<uint8_t>(max_voltage * 10);

        // 允许范围：6.6V ~ 10V（换算后 66 ~ 100）
        if (min_value < 60 || min_value > 100 || max_value < 60 || max_value > 100) {
            throw std::out_of_range("Voltage out of range (6V - 10V)");
        }

        return buildCommandPacket(ORDER::WRITE_DATA, static_cast<uint8_t>(EEPROM::MIN_VOLTAGE), {
                min_value, // 最低电压
                max_value // 最高电压
        });
    }

    // 设定最大扭矩
    std::vector<uint8_t> ServoEEPROM::buildSetMaxTorque(int32_t torque) {
        if (torque < 0 || torque > 1023) {
            throw std::out_of_range("Torque out of range (0 - 1023)");
        }

        return buildCommandPacket(ORDER::WRITE_DATA, static_cast<uint8_t>(EEPROM::MAX_TORQUE_L), {
                static_cast<uint8_t>(torque & 0xFF), // 低字节 (L)
                static_cast<uint8_t>((torque >> 8) & 0xFF) // 高字节 (H)
        });
    }

    // 设定应答返回级别
    std::vector<uint8_t> ServoEEPROM::buildSetStatusReturnLevel(StatusReturnLevel level) {
        return buildCommandPacket(ORDER::WRITE_DATA, static_cast<uint8_t>(EEPROM::STATUS_RETURN_LEVEL), {
                static_cast<uint8_t>(level)
        });
    }

    // 设定 LED 报警
    std::vector<uint8_t> ServoEEPROM::buildSetAlarmLED(AlarmLEDConfig config) {
        return buildCommandPacket(ORDER::WRITE_DATA, static_cast<uint8_t>(EEPROM::ALARM_LED), {
                static_cast<uint8_t>(config)
        });
    }

    // 设定报警卸载条件
    std::vector<uint8_t> ServoEEPROM::buildSetAlarmShutdown(AlarmShutdownConfig config) {
        return buildCommandPacket(ORDER::WRITE_DATA, static_cast<uint8_t>(EEPROM::ALARM_SHUTDOWN), {
                static_cast<uint8_t>(config)
        });
    }

    // 读取指定长度的 Eeprom 数据
    std::vector<uint8_t> ServoEEPROM::buildGetEepromData(EEPROM eeprom, int length) {
        return buildCommandPacket(ORDER::READ_DATA, static_cast<uint8_t>(eeprom), {static_cast<uint8_t>(length)});
    }


    ServoRAM::ServoRAM(uint8_t id) : Base(id) {
    }

    // 读取扭矩开关状态
    std::vector<uint8_t> ServoRAM::buildGetTorqueEnabled() {
        return buildCommandPacket(ORDER::READ_DATA, static_cast<uint8_t>(RAM::TORQUE_ENABLE), {0x01});
    }

    // 使能/禁用扭矩
    std::vector<uint8_t> ServoRAM::buildSetTorqueEnabled(bool enable) {
        return buildCommandPacket(ORDER::WRITE_DATA, static_cast<uint8_t>(RAM::TORQUE_ENABLE), {toByte(enable)});
    }

    // 读取 LED 状态
    std::vector<uint8_t> ServoRAM::buildGetLEDEnabled() {
        return buildCommandPacket(ORDER::READ_DATA, static_cast<uint8_t>(RAM::LED), {0x01});
    }

    // 设置 LED 状态
    std::vector<uint8_t> ServoRAM::buildSetLEDEnabled(bool enable) {
        return buildCommandPacket(ORDER::WRITE_DATA, static_cast<uint8_t>(RAM::LED), {toByte(enable)});
    }

    // 读取顺时针不灵敏区
    std::vector<uint8_t> ServoRAM::buildGetCwComplianceMargin() {
        return buildCommandPacket(ORDER::READ_DATA, static_cast<uint8_t>(RAM::CW_COMPLIANCE_MARGIN), {0x01});
    }

    // 读取逆时针不灵敏区
    std::vector<uint8_t> ServoRAM::buildGetCcwComplianceMargin() {
        return buildCommandPacket(ORDER::READ_DATA, static_cast<uint8_t>(RAM::CCW_COMPLIANCE_MARGIN), {0x01});
    }

    // 读取顺时针比例系数
    std::vector<uint8_t> ServoRAM::buildGetCwComplianceSlope() {
        return buildCommandPacket(ORDER::READ_DATA, static_cast<uint8_t>(RAM::CW_COMPLIANCE_SLOPE), {0x01});
    }

    // 读取逆时针比例系数
    std::vector<uint8_t> ServoRAM::buildGetCcwComplianceSlope() {
        return buildCommandPacket(ORDER::READ_DATA, static_cast<uint8_t>(RAM::CCW_COMPLIANCE_SLOPE), {0x01});
    }

    // 同步 控制舵机 直接移动到目标角度
    std::vector<uint8_t> ServoRAM::buildMoveToPosition(float angle) {
        if (angle < 0.0f || angle > 300.0f) {
            throw std::out_of_range("目标角度超出范围 [0° - 300°]");
        }

        // 计算目标位置 (0x0000 - 0x03FF)
        uint16_t position = static_cast<uint16_t>((angle / 300.0f) * 1023);

        return buildCommandPacket(ORDER::WRITE_DATA, static_cast<uint8_t>(RAM::GOAL_POSITION_L), {
                static_cast<uint8_t>(position & 0xFF), // 低字节
                static_cast<uint8_t>((position >> 8) & 0xFF) // 高字节
        });
    }

    // 目标角度和速度
    std::vector<uint8_t> ServoRAM::buildMoveToWithSpeedRpm(float angle, float rpm) {
        if (angle < 0.0f || angle > 300.0f) {
            throw std::out_of_range("目标角度超出范围 [0° - 300°]");
        }
        if (rpm <= 0 || rpm > 62.0f) {
            throw std::out_of_range("RPM 超出范围 (0 - 62.0]");
        }

        // **修正：目标位置计算**
        uint16_t position = static_cast<uint16_t>((angle / 300.0f) * 1023); // 0x0000 - 0x03FF

        // **修正：速度计算**
        uint16_t speed = static_cast<uint16_t>(rpm * 1023.0f / 62.0f); // 0x0000 - 0x03FF

        return buildCommandPacket(ORDER::WRITE_DATA, static_cast<uint8_t>(RAM::GOAL_POSITION_L), {
                static_cast<uint8_t>(position & 0xFF), // 位置低字节
                static_cast<uint8_t>((position >> 8) & 0xFF), // 位置高字节
                static_cast<uint8_t>(speed & 0xFF), // 速度低字节
                static_cast<uint8_t>((speed >> 8) & 0xFF) // 速度高字节
        });
    }


    // 异步写 (REG_WRITE)，舵机 不立即运动，等待 ACTION 指令
    std::vector<uint8_t> ServoRAM::buildAsyncMoveToPosition(float angle) {
        if (angle < 0.0f || angle > 300.0f) {
            throw std::out_of_range("目标角度超出范围 [0° - 300°]");
        }

        // 计算目标位置 (0x0000 - 0x03FF)
        uint16_t position = static_cast<uint16_t>((angle / 300.0f) * 1023);

        return buildCommandPacket(ORDER::REG_WRITE, static_cast<uint8_t>(RAM::GOAL_POSITION_L), {
                static_cast<uint8_t>(position & 0xFF), // 低字节
                static_cast<uint8_t>((position >> 8) & 0xFF) // 高字节
        });
    }

    // REG_WRITE + ACTION
    std::vector<uint8_t> ServoRAM::buildActionCommand() {
        return buildCommandPacket(ORDER::ACTION, 0, {});
    }

    // 设置舵机运行的加速度和减速度
    std::vector<uint8_t> ServoRAM::buildSetAccelerationDeceleration(uint8_t acceleration, uint8_t deceleration) {
        if (acceleration > 255 || deceleration > 255) {
            throw std::out_of_range("加速度或减速度超出范围 (0 - 255)");
        }

        return buildCommandPacket(ORDER::WRITE_DATA, static_cast<uint8_t>(RAM::ACCELERATION), {
                acceleration, // 加速度
                deceleration // 减速度
        });
    }

    std::vector<uint8_t> ServoRAM::buildGetGoalPosition() {
        return buildCommandPacket(ORDER::READ_DATA, static_cast<uint8_t>(RAM::GOAL_POSITION_L), {0x02});
    }

    std::vector<uint8_t> ServoRAM::buildGetRunSpeed() {
        return buildCommandPacket(ORDER::READ_DATA, static_cast<uint8_t>(RAM::MOVING_SPEED_L), {0x02});
    }

    std::vector<uint8_t> ServoRAM::buildGetAcceleration() {
        return buildCommandPacket(ORDER::READ_DATA, static_cast<uint8_t>(RAM::ACCELERATION), {0x01});
    }

    std::vector<uint8_t> ServoRAM::buildGetDeceleration() {
        return buildCommandPacket(ORDER::READ_DATA, static_cast<uint8_t>(RAM::DECELERATION), {0x01});
    }

    std::vector<uint8_t> ServoRAM::buildGetAccelerationDeceleration() {
        return buildCommandPacket(ORDER::READ_DATA, static_cast<uint8_t>(RAM::ACCELERATION), {0x02});
    }

    std::vector<uint8_t> ServoRAM::buildGetPosition() {
        return buildCommandPacket(ORDER::READ_DATA, static_cast<uint8_t>(RAM::PRESENT_POSITION_L), {0x02});
    }

    std::vector<uint8_t> ServoRAM::buildGetSpeed() {
        return buildCommandPacket(ORDER::READ_DATA, static_cast<uint8_t>(RAM::PRESENT_SPEED_L), {0x02});
    }

    std::vector<uint8_t> ServoRAM::buildGetLoad() {
        return buildCommandPacket(ORDER::READ_DATA, static_cast<uint8_t>(RAM::PRESENT_LOAD_L), {0x02});
    }

    std::vector<uint8_t> ServoRAM::buildGetVoltage() {
        return buildCommandPacket(ORDER::READ_DATA, static_cast<uint8_t>(RAM::PRESENT_VOLTAGE), {0x01});
    }

    std::vector<uint8_t> ServoRAM::buildGetTemperature() {
        return buildCommandPacket(ORDER::READ_DATA, static_cast<uint8_t>(RAM::TEMPERATURE), {0x01});
    }

    std::vector<uint8_t> ServoRAM::buildCheckRegWriteFlag() {
        return buildCommandPacket(ORDER::READ_DATA, static_cast<uint8_t>(RAM::REG_WRITE), {0x01});
    }

    std::vector<uint8_t> ServoRAM::buildCheckMovingFlag() {
        return buildCommandPacket(ORDER::READ_DATA, static_cast<uint8_t>(RAM::MOVING), {0x01});
    }

    // 设置锁标志（1：锁定，0：解锁）
    std::vector<uint8_t> ServoRAM::buildSetLockFlag(bool lock) {
        return buildCommandPacket(ORDER::WRITE_DATA, static_cast<uint8_t>(RAM::LOCK), {toByte(lock)});
    }

    // 读取锁标志
    std::vector<uint8_t> ServoRAM::buildGetLockFlag() {
        return buildCommandPacket(ORDER::READ_DATA, static_cast<uint8_t>(RAM::LOCK), {1});
    }

    // 设置最小PWM
    std::vector<uint8_t> ServoRAM::buildSetMinPWM(uint16_t pwm) {
        if (pwm > 0x03FF) {
            throw std::out_of_range("最小PWM值超出范围 (0 - 1023)");
        }

        return buildCommandPacket(ORDER::WRITE_DATA, static_cast<uint8_t>(RAM::MIN_PWM_L), {
                static_cast<uint8_t>(pwm & 0xFF), // 低字节
                static_cast<uint8_t>((pwm >> 8) & 0xFF) // 高字节
        });
    }

    // 读取最小PWM
    std::vector<uint8_t> ServoRAM::buildGetMinPWM() {
        return buildCommandPacket(ORDER::READ_DATA, static_cast<uint8_t>(RAM::MIN_PWM_L), {2});
    }

    // 读取一定长度的 RAM 数据
    std::vector<uint8_t> ServoRAM::buildGetRamData(servo::RAM ram, int length) {
        return buildCommandPacket(ORDER::READ_DATA, static_cast<uint8_t>(ram), {static_cast<uint8_t>(length)});
    }


    Motor::Motor(uint8_t id) : Base(id) {
    }

    // 设置舵机进入电机调速模式
    std::vector<uint8_t> Motor::buildMotorMode() {
        return buildCommandPacket(ORDER::WRITE_DATA, static_cast<uint8_t>(EEPROM::CW_ANGLE_LIMIT_L),
                                  {0x00, 0x00, 0x00, 0x00});
    }

    std::vector<uint8_t> Motor::buildServoMode() {
        return buildCommandPacket(ORDER::WRITE_DATA, static_cast<uint8_t>(EEPROM::CW_ANGLE_LIMIT_L),
                                  {0x00, 0x00, 0xff, 0x03});
    }

    // 设置电机模式的转速
    std::vector<uint8_t> Motor::buildSetMotorSpeed(float rpm) {
        if (rpm < -62.0f || rpm > 62.0f) {
            throw std::out_of_range("速度超出范围 (-62.0 - 62.0 RPM)");
        }

        // **计算速度值 (0~1023)**
        uint16_t speed = static_cast<uint16_t>(std::round(std::abs(rpm) * 1023.0f / 62.0f));

        // **设置方向位 BIT10**
        if (rpm >= 0) {
            speed |= 0x0400; // 顺时针 (BIT10 = 1)
        } else {
            speed &= 0x03FF; // 逆时针 (BIT10 = 0)
        }

        // **调用 `buildCommandPacket` 生成命令**
        return buildCommandPacket(ORDER::WRITE_DATA, static_cast<uint8_t>(RAM::MOVING_SPEED_L), {
                static_cast<uint8_t>(speed & 0xFF), // 速度低字节
                static_cast<uint8_t>((speed >> 8) & 0xFF) // 速度高字节 (包含方向位)
        });
    }

    std::vector<uint8_t> Motor::buildRestoreAngleLimits() {
        return buildCommandPacket(ORDER::WRITE_DATA, static_cast<uint8_t>(EEPROM::CW_ANGLE_LIMIT_L), {
                0x00, 0x00, // CW 角度限制 = 0x0000
                0xFF, 0x03 // CCW 角度限制 = 0x03FF
        });
    }

    // 将错误位映射到结构
    ServoErrorInfo getServoErrorInfo(uint8_t error) {
        ServoErrorInfo errorInfo = {NO_ERROR, "无错误"};

        std::vector<ServoErrorInfo> errors = {
                {INSTRUCTION_ERROR,          "指令错误"},
                {OVERLOAD,                   "过载"},
                {CHECKSUM_ERROR,             "校验和错误"},
                {COMMAND_OUT_OF_RANGE,       "指令超范围"},
                {OVERHEAT,                   "过热"},
                {OUT_OF_RANGE,               "角度超范围"},
                {OVER_VOLTAGE_UNDER_VOLTAGE, "过压/欠压"},
        };

        for (const auto &err: errors) {
            if (error & static_cast<uint8_t>(err.error)) {
                if (errorInfo.error == NO_ERROR) {
                    // 只取第一个错误
                    errorInfo = err;
                } else {
                    errorInfo.description += "，" + std::string(err.description); // 连接多个错误信息
                }
            }
        }

        return errorInfo;
    }

    // 从 speed_ratio 转换为 RPM
    float speedRatioToRPM(float speed_ratio) {
        if (speed_ratio < -1.0f || speed_ratio > 1.0f) {
            throw std::out_of_range("速度比例超出范围 (-1.0 - 1.0)");
        }
        return speed_ratio * 62.0f;
    }

    // 从 RPM 转换为 speed_ratio
    float rpmToSpeedRatio(float rpm) {
        if (rpm < -62.0f || rpm > 62.0f) {
            throw std::out_of_range("RPM 超出范围 (-62.0 - 62.0)");
        }
        return rpm / 62.0f;
    }
} // namespace servo
