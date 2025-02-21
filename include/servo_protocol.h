//
// Created by noodles on 25-2-19.
//

#ifndef UP_CORE_SERVO_PROTOCOL_H
#define UP_CORE_SERVO_PROTOCOL_H

#include <vector>
#include <stdint.h>

namespace servo {

    enum class AlarmShutdownConfig : uint8_t {
        NONE = 0,       // 不启用卸载保护
        INSTRUCTION_ERROR = 1 << 6,  // 指令错误
        CHECKSUM_ERROR = 1 << 4,  // 校验和错误
        RANGE_ERROR = 1 << 3,  // 指令超范围
        OVERHEAT = 1 << 2,  // 过热（默认）
        ANGLE_ERROR = 1 << 1,  // 角度超范围
        VOLTAGE_ERROR = 1 << 0   // 电压超范围
    };

    // 允许 `AlarmShutdownConfig` 进行按位操作（`|`, `&`, `~`）
    inline AlarmShutdownConfig operator|(AlarmShutdownConfig a, AlarmShutdownConfig b) {
        return static_cast<AlarmShutdownConfig>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
    }

    inline AlarmShutdownConfig operator&(AlarmShutdownConfig a, AlarmShutdownConfig b) {
        return static_cast<AlarmShutdownConfig>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
    }

    inline AlarmShutdownConfig operator~(AlarmShutdownConfig a) {
        return static_cast<AlarmShutdownConfig>(~static_cast<uint8_t>(a));
    }

    enum class AlarmLEDConfig : uint8_t {
        NONE = 0,       // 不设置任何报警
        INSTRUCTION_ERROR = 1 << 6,  // 指令错误
        OVERLOAD = 1 << 5,  // 过载
        CHECKSUM_ERROR = 1 << 4,  // 校验和错误
        RANGE_ERROR = 1 << 3,  // 指令超范围
        OVERHEAT = 1 << 2,  // 过热
        ANGLE_ERROR = 1 << 1,  // 超过角度范围
        VOLTAGE_ERROR = 1 << 0   // 超过电压范围
    };

    // 允许 `AlarmLEDConfig` 进行按位操作（`|`, `&`, `~`）
    inline AlarmLEDConfig operator|(AlarmLEDConfig a, AlarmLEDConfig b) {
        return static_cast<AlarmLEDConfig>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
    }

    inline AlarmLEDConfig operator&(AlarmLEDConfig a, AlarmLEDConfig b) {
        return static_cast<AlarmLEDConfig>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
    }

    inline AlarmLEDConfig operator~(AlarmLEDConfig a) {
        return static_cast<AlarmLEDConfig>(~static_cast<uint8_t>(a));
    }


    enum class StatusReturnLevel : uint8_t {
        NO_RESPONSE = 0,     // 对所有指令都不返回
        READ_ONLY = 1,       // 只对读指令返回
        ALL_RESPONSE = 2     // 对所有指令返回
    };

    enum class ORDER : uint8_t {
        PING = 0x01,                    // 1（0x01） Ping
        READ_DATA = 0x02,               // 2（0x02） 读数据
        WRITE_DATA = 0x03,              // 3（0x03） 写数据
        REG_WRITE = 0x04,               // 4（0x04） REG WRITE 类似于WRITE DATA，但是控制字符写入后并不马上动作，直到ACTION指令到达
        ACTION = 0x05,                  // 5（0x05） ACTION 触发REG WRITE指令
        RESET = 0x06,                   // 6（0x06） 复位舵机
        SYNC_WRITE = 0x83,              // 131（0x83） 同步写
    };

    // ===================  基类  ===================
    class Base {
    protected:
        uint8_t id_;  // 舵机 ID

    public:
        explicit Base(uint8_t id);

        virtual ~Base() = default;  // ✅ 统一析构函数

        uint8_t getID() const { return id_; }  // ✅ 统一获取 ID
        // 发送 WRITE DATA 指令
        std::vector<uint8_t> buildCommandPacket(ORDER command, uint8_t address, const std::vector<uint8_t> &data);

        // 发送 READ DATA 指令
        bool readRegister(uint8_t address, uint8_t length, std::vector<uint8_t> &data);
    };

    // ===================  EEPROM 相关  ===================
    enum class EEPROM : uint8_t {
        MODEL_NUMBER_L = 0x00,          // 0（0x00） 低位型号号码 读 2（0x02）
        MODEL_NUMBER_H = 0x01,          // 1（0x01） 高位型号号码 读 1（0x01）
        VERSION = 0x02,                 // 2（0x02） 软件版本 读 1（0x01）
        ID = 0x03,                      // 3（0x03） ID 读/写 1（0x01）
        BAUDRATE = 0x04,                // 4（0x04） 波特率 读/写 1（0x01）
        RETURN_DELAY_TIME = 0x05,       // 5（0x05） 返回延迟时间 读/写 0（0x00）
        CW_ANGLE_LIMIT_L = 0x06,        // 6（0x06） 顺时针角度限制（L） 读/写 0（0x00）
        CW_ANGLE_LIMIT_H = 0x07,        // 7（0x07） 顺时针角度限制（H） 读/写 0（0x00）
        CCW_ANGLE_LIMIT_L = 0x08,       // 8（0x08） 逆时针角度限制（L） 读/写 255（0xFF）
        CCW_ANGLE_LIMIT_H = 0x09,       // 9（0x09） 逆时针角度限制（H） 读/写 3（0x03）
        MAX_TEMPERATURE = 0x0B,         // 11（0x0B） 最高温度上限 读/写 80（0x50）
        MIN_VOLTAGE = 0x0C,             // 12（0x0C） 最低输入电压 读/写 ?
        MAX_VOLTAGE = 0x0D,             // 13（0x0D） 最高输入电压 读/写 ?
        MAX_TORQUE_L = 0x0E,            // 14（0x0E） 最大扭矩（L） 读/写 255（0xFF）
        MAX_TORQUE_H = 0x0F,            // 15（0x0F） 最大扭矩（H） 读/写 3（0x03）
        STATUS_RETURN_LEVEL = 0x10,     // 16（0x10） 应答状态级别 读/写 2（0x02）
        ALARM_LED = 0x11,               // 17（0x11） LED闪烁 读/写 37（0x25）
        ALARM_SHUTDOWN = 0x12,          // 18（0x12） 卸载条件 读/写 4（0x04）
    };

    class ServoEEPROM : public Base {
    public:
        explicit ServoEEPROM(uint8_t id);

        // 读取软件版本
        std::vector<uint8_t> buildGetSoftwareVersion();

        // 读取舵机 ID
        std::vector<uint8_t> buildGetID();

        // 设置舵机 ID
        std::vector<uint8_t> buildSetID(uint8_t new_id);

        // 读取波特率
        std::vector<uint8_t> buildGetBaudrate();

        // 设置波特率
        std::vector<uint8_t> buildSetBaudrate(uint32_t baud);

        // 读取返回延迟时间
        std::vector<uint8_t> buildGetReturnDelayTime();

        // 设置舵机返回数据的延迟时间（单位：微秒）。
        std::vector<uint8_t> buildSetReturnDelayTime(uint8_t delay);

        // 读取顺时针角度限制
        std::vector<uint8_t> buildGetCwAngleLimit();

        // 读取逆时针角度限制
        std::vector<uint8_t> buildGetCcwAngleLimit();

        // 设定角度限制
        std::vector<uint8_t> buildSetAngleLimit(uint16_t min_angle, uint16_t max_angle);

        // 读取最高温度上限
        std::vector<uint8_t> buildGetMaxTemperature();

        // 设定最大温度
        std::vector<uint8_t> buildSetMaxTemperature(int32_t temperature);

        // 读取最低输入电压
        std::vector<uint8_t> buildGetMinVoltage();

        // 读取最高输入电压
        std::vector<uint8_t> buildGetMaxVoltage();

        // 设定电压范围
        std::vector<uint8_t> buildSetVoltageRange(float min_voltage, float max_voltage);

        // 读取最大扭矩
        std::vector<uint8_t> buildGetMaxTorque();

        // 设定最大扭矩
        std::vector<uint8_t> buildSetMaxTorque(int32_t torque);

        // 读取应答状态级别
        std::vector<uint8_t> buildGetStatusReturnLevel();

        // 设定应答返回级别
        std::vector<uint8_t> buildSetStatusReturnLevel(StatusReturnLevel level);

        // 读取 LED 闪烁报警条件
        std::vector<uint8_t> buildGetAlarmLED();

        // 设定 LED 报警
        std::vector<uint8_t> buildSetAlarmLED(AlarmLEDConfig config);

        // 读取卸载条件
        std::vector<uint8_t> buildGetAlarmShutdown();

        // 设定报警卸载条件
        std::vector<uint8_t> buildSetAlarmShutdown(AlarmShutdownConfig config);

    };

    // ===================  RAM 相关  ===================
    enum class RAM : uint8_t {
        TORQUE_ENABLE = 0x18,           // 24（0x18） 扭矩开关 读/写 0（0x00）
        LED = 0x19,                     // 25（0x19） LED开关 读/写 0（0x00）
        CW_COMPLIANCE_MARGIN = 0x1A,    // 26（0x1A） 顺时针不灵敏区 读/写 2（0x02）
        CCW_COMPLIANCE_MARGIN = 0x1B,   // 27（0x1B） 逆时针不灵敏区 读/写 2（0x02）
        CW_COMPLIANCE_SLOPE = 0x1C,     // 28（0x1C） 顺时针比例系数 读/写 32（0x20）
        CCW_COMPLIANCE_SLOPE = 0x1D,    // 29（0x1D） 逆时针比例系数 读/写 32（0x20）
        GOAL_POSITION_L = 0x1E,         // 30（0x1E） 目标位置（L） 读/写 [Addr36]value
        GOAL_POSITION_H = 0x1F,         // 31（0x1F） 目标位置（H） 读/写 [Addr37]value
        MOVING_SPEED_L = 0x20,          // 32（0x20） 运行速度（L） 读/写 0
        MOVING_SPEED_H = 0x21,          // 33（0x21） 运行速度（H） 读/写 0
        ACCELERATION = 0x22,            // 34（0x22） 加速度 读/写 32
        DECELERATION = 0x23,            // 35（0x23） 减速度 读/写 32
        PRESENT_POSITION_L = 0x24,      // 36（0x24） 当前位置（L） 读 ？
        PRESENT_POSITION_H = 0x25,      // 37（0x25） 当前位置（H） 读 ？
        PRESENT_SPEED_L = 0x26,         // 38（0x26） 当前速度（L） 读 ？
        PRESENT_SPEED_H = 0x27,         // 39（0x27） 当前速度（H） 读 ？
        PRESENT_LOAD_L = 0x28,          // 40（0x28） 当前负载 读 ？
        PRESENT_LOAD_H = 0x29,          // 41（0x29） 当前负载 读 ？
        PRESENT_VOLTAGE = 0x2A,         // 42（0x2A） 当前电压 读 ？
        TEMPERATURE = 0x2B,             // 43（0x2B） 当前温度 读 ？
        REG_WRITE = 0x2C,               // 44（0x2C） REG WRITE标志 读 0（0x00）
        MOVING = 0x2E,                  // 46（0x2E） 运行中标志 读 0（0x00）
        LOCK = 0x2F,                    // 47（0x2F） 锁标志 读/写 0（0x00）
        MIN_PWM_L = 0x30,               // 48（0x30） 最小PWM(L) 读/写 90（0x5A）
        MIN_PWM_H = 0x31,               // 49（0x31） 最小PWM(H) 读/写 00（0x00）
    };

    class ServoRAM : public Base {
    public:
        explicit ServoRAM(uint8_t id);

        // 读取扭矩开关状态
        std::vector<uint8_t> buildGetTorqueEnabled();

        // 使能/禁用扭矩
        std::vector<uint8_t> buildSetTorqueEnabled(bool enable);

        // 读取 LED 状态
        std::vector<uint8_t> buildGetLEDEnabled();

        // 设置 LED 状态
        std::vector<uint8_t> buildSetLEDEnabled(bool enable);

        // 读取顺时针不灵敏区
        std::vector<uint8_t> buildGetCwComplianceMargin();

        // 读取逆时针不灵敏区
        std::vector<uint8_t> buildGetCcwComplianceMargin();

        // 读取顺时针比例系数
        std::vector<uint8_t> buildGetCwComplianceSlope();

        // 读取逆时针比例系数
        std::vector<uint8_t> buildGetCcwComplianceSlope();

        // 同步 控制舵机 直接移动到目标角度
        std::vector<uint8_t> buildMoveToPosition(float angle);

        // 目标角度和速度
        std::vector<uint8_t> buildMoveToWithSpeed(float angle, float speed_ratio);

        // 异步写 (REG_WRITE)，舵机 不立即运动，等待 ACTION 指令
        std::vector<uint8_t> buildAsyncMoveToPosition(float angle);

        // REG_WRITE + ACTION
        std::vector<uint8_t> buildActionCommand();

        // 设置舵机运行的加速度和减速度
        std::vector<uint8_t> buildSetAccelerationDeceleration(uint8_t acceleration, uint8_t deceleration);

        // 读取当前位置
        std::vector<uint8_t> buildGetPosition();

        // 读取当前速度
        std::vector<uint8_t> buildGetSpeed();

        // 读取加速度
        std::vector<uint8_t> buildGetAcceleration();

        // 读取减速度
        std::vector<uint8_t> buildGetDeceleration();

        // 读取当前负载
        std::vector<uint8_t> buildGetLoad();

        // 读取当前电压
        std::vector<uint8_t> buildGetVoltage();

        // 读取当前温度
        std::vector<uint8_t> buildGetTemperature();

        // 检查 REG WRITE 是否等待执行
        std::vector<uint8_t> buildCheckRegWriteFlag();

        // 检查舵机是否正在运行
        std::vector<uint8_t> buildCheckMovingFlag();

        std::vector<uint8_t> buildSetLockFlag(bool lock);

        // 读取锁标志
        std::vector<uint8_t> buildGetLockFlag();

        // 设置最小PWM
        std::vector<uint8_t> buildSetMinPWM(uint16_t pwm);

        // 读取最小PWM
        std::vector<uint8_t> buildGetMinPWM();

    };


    class Motor : public Base {
    public:
        explicit Motor(uint8_t id);

        // 设置舵机进入电机调速模式
        std::vector<uint8_t> buildEnterWheelMode();

        // 设置电机模式的转速
        std::vector<uint8_t> buildSetWheelSpeed(float speed_ratio);

        // 还原角度
        std::vector<uint8_t> buildRestoreAngleLimits();

    };

    class ServoProtocol : public Base {
    public:
        ServoEEPROM eeprom;
        ServoRAM ram;

        explicit ServoProtocol(uint8_t id)
                : Base(id), eeprom(id), ram(id) {}
    };
} // namespace servo


///**
// * 舵机协议处理类
// */
//class ServoProtocol {
//public:
//    static std::vector<uint8_t> createFrame(uint8_t id, uint8_t cmd, const std::vector<uint8_t> &data);
//
//    static bool parseResponse(const std::vector<uint8_t> &response, uint8_t expected_id, uint8_t &status,
//                              std::vector<uint8_t> &payload);
//};

#endif //UP_CORE_SERVO_PROTOCOL_H
