//
// Created by noodles on 25-2-19.
//

#ifndef UP_CORE_SERVO_PROTOCOL_H
#define UP_CORE_SERVO_PROTOCOL_H

#include <vector>
#include <stdint.h>
#include "string"
#include <functional>

namespace servo {

    const uint8_t HEAD_ADDRESS = 0x1E;

    class ServoProtocol;

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

        std::vector<uint8_t> buildShortPacket(int write_length, std::vector<uint8_t> commandData);

        std::vector<uint8_t> buildCommandPacket(ORDER command, uint8_t address, const std::vector<uint8_t> &data);

        /**
         * PING（查询）     查询工作状态          0x01    0
         *
         * 读取 ID 号为 1 的 CDS55xx 的工作状态
         *
         * 指令帧:
         *  0XFF
         *  0XFF
         *  0X01    ID
         *  0X02    Length
         *  0X01    Instruction
         *  0XFB    Check Sum
         * 返回的数据帧:
         *  0XFF
         *  0XFF
         *  0X01    ID
         *  0X02    Length
         *  0X00    Error
         *  0XFC    Check Sum
         *
         *  不管是广播 ID 还是 Return Level (Address 16)等于 0，只要发送 PINE 指令且校验和正确，都会返回舵机工作状态
         */
        std::vector<uint8_t> buildPingPacket();

        /**
         * READ DATA（读） 查询控制表里的字符       0x02    2
         *
         * 读取 ID 为 1 的 CDS55xx 的内部温度。在控制表里从地址 0X2B 处读取一个字节。
         *
         * 指令帧:
         *  0XFF
         *  0XFF
         *  0X01    ID
         *  0X04    Length
         *  0X02    Instruction
         *  0X2B    对应 EEPROM::TEMPERATURE
         *  0X01    读取 1 个字节
         *  0XCC    Check Sum
         * 返回的数据帧:
         *  0XFF
         *  0XFF
         *  0X01    ID
         *  0X03    Length
         *  0X00    Error
         *  0X20
         *  0XDB    Check Sum
         *
         *  读出的数据是 0x20，说明当前的温度约是 32℃（0x20）
         */
        std::vector<uint8_t> buildReadPacket(uint8_t address, uint8_t read_length);


        /**
         * WRITE DATA（写） 往控制表里写入字符      0x03    不小于2
         *
         * 把一个任意编号的 CDS55xx 的 ID 设置为 1。在控制表里保存 ID 号的地址为 3（控制表见后文），所以在地址 3 处写入 1 即可. 发送指令包的 ID 使用广播 ID (0xFE)。
         *
         * 指令帧:
         *  0XFF
         *  0XFF
         *  0XFE    ID（广播）
         *  0X04    Length
         *  0X03    Instruction
         *  0X03    对应 EEPROM::ID
         *  0X01    修改数据
         *  0XF6
         *
         *  因为采用广播 ID 发送指令，所以不会有数据返回
         */
        std::vector<uint8_t> buildWritePacket(uint8_t address, const std::vector<uint8_t> &data);

        /**
         * REG WRITE（异步写）   类似于WRITE DATA，但是控制字符写入后并不马上动作，直到ACTION指令到达  0x04    不小于2
         */
        std::vector<uint8_t> buildRegWritePacket(uint8_t address, const std::vector<uint8_t> &data);

        /**
         * ACTION（执行异步写） 触发REG WRITE写入的动作   0x05    0
         */
        std::vector<uint8_t> buildActionPacket();

        /**
         * RESET（复位） 把控制表复位为出厂值     0x06    0
         *
         * 复位 CDS55xx，ID 号为 0
         *
         * 指令帧:
         *  0XFF
         *  0XFF
         *  0X00     ID
         *  0X02     Length
         *  0X06     Instruction
         *  0XF7
         * 返回的数据帧:
         *  0XFF
         *  0XFF
         *  0X00     ID
         *  0X02     Length
         *  0X00     Error
         *  0XFD
         */
        std::vector<uint8_t> buildResetPacket();

        /**
         * SYNC WRITE
         *
         * 用于同时控制多个 CDS55xx。
         *
         * 0XFF 0XFF
         * 0XFE     ID（广播）
         * 0X18     Length
         * 0X83     Instruction
         *
         * 0X1E     写入数据的首地址，对应 RAM::GOAL_POSITION_L
         * 0X04     写入的数据的长度(L)
         * 0X00     第一个 CDS55xx 的 ID 号
         * 0X10 0X00 0X50 0X01  写入的四个数据
         * 0X01     第二个 CDS55xx 的 ID 号
         * 0X20 0X02 0X60 0X03  写入的四个数据
         * 0X02     第三个 CDS55xx 的 ID 号
         * 0X30 0X00 0X70 0X01  写入的四个数据
         * 0X03     第四个 CDS55xx 的 ID 号
         * 0X20 0X02 0X80 0X03  写入的四个数据
         *
         * 0X12     Check Sum
         */
        std::vector<uint8_t>
        buildSyncWritePacket(uint8_t address, int write_length, std::vector<ServoProtocol> &protocols,
                             std::function<std::vector<uint8_t>(ServoProtocol &data, int position)> func);


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
        EEPROM_COUNT
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

        // 读取角度限制
        std::vector<uint8_t> buildGetAngleLimit();

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

        // 读取输入电压范围
        std::vector<uint8_t> buildGetVoltageRange();

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

        // 读取指定长度的 Eeprom 数据
        std::vector<uint8_t> buildGetEepromData(EEPROM eeprom, int length);

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
        RAM_COUNT
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

        /**
         * 位置范围：0x0000 对应 0 度，0x03ff 对应 300 度
         *      位置范围为 0 到 1023（0x0000 到 0x03ff），每个单位对应约 0.293 度。
         * 速度范围：最大速度 1023（62RPM），最小速度 1。0 和 1023 都表示最大速度。
         */
        // 目标角度和速度
        std::vector<uint8_t> buildMoveToWithSpeedRpm(float angle, float rpm);

        // 异步写 (REG_WRITE)，舵机 不立即运动，等待 ACTION 指令
        std::vector<uint8_t> buildAsyncMoveToPosition(float angle);

        // REG_WRITE + ACTION
        std::vector<uint8_t> buildActionCommand();

        // 设置舵机运行的加速度和减速度
        std::vector<uint8_t> buildSetAccelerationDeceleration(uint8_t acceleration, uint8_t deceleration);

        // 读取目标位置
        std::vector<uint8_t> buildGetGoalPosition();

        // 读取运行速度
        std::vector<uint8_t> buildGetRunSpeed();

        // 读取当前位置
        std::vector<uint8_t> buildGetPosition();

        // 读取当前速度
        std::vector<uint8_t> buildGetSpeed();

        // 读取加速度
        std::vector<uint8_t> buildGetAcceleration();

        // 读取减速度
        std::vector<uint8_t> buildGetDeceleration();

        // 读取加速度和减速度
        std::vector<uint8_t> buildGetAccelerationDeceleration();

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

        // 设置锁标志（1：锁定，0：解锁）
        std::vector<uint8_t> buildSetLockFlag(bool lock);

        // 读取锁标志
        std::vector<uint8_t> buildGetLockFlag();

        // 设置最小PWM
        std::vector<uint8_t> buildSetMinPWM(uint16_t pwm);

        // 读取最小PWM
        std::vector<uint8_t> buildGetMinPWM();

        // 读取指定长度的 RAM 数据
        std::vector<uint8_t> buildGetRamData(RAM ram, int length);
    };


    class Motor : public Base {
    public:
        explicit Motor(uint8_t id);

        /**
         * 角度限制解除：将角度限制（0x06~0x09）设为 0，使舵机进入轮式模式
         */
        // 设置舵机进入电机调速模式
        std::vector<uint8_t> buildMotorMode();

        // 回到舵机模式
        std::vector<uint8_t> buildServoMode();

        /**
         * 速度控制：速度大小由 0x20~0x21 控制，低 10 位（BIT0~BIT9）存储速度值，高 1 位（BIT10）表示方向：
         */
        // 设置电机模式的转速
        std::vector<uint8_t> buildSetMotorSpeed(float rpm);

        // 还原角度
        std::vector<uint8_t> buildRestoreAngleLimits();

    };

    class ServoProtocol : public Base {
    public:
        ServoEEPROM eeprom;
        ServoRAM ram;
        Motor motor;

        explicit ServoProtocol(uint8_t id)
                : Base(id), eeprom(id), ram(id), motor(id) {}
    };


    // 枚举定义错误状态
    enum ServoError {
        NO_ERROR = 0x00,
        OUT_OF_RANGE = 0x01,
        OVERHEAT = 0x02,
        COMMAND_OUT_OF_RANGE = 0x04,
        CHECKSUM_ERROR = 0x08,
        OVERLOAD = 0x10,
        INSTRUCTION_ERROR = 0x20,
        OVER_VOLTAGE_UNDER_VOLTAGE = 0x40,
    };

    // 错误处理结构
    struct ServoErrorInfo {
        ServoError error;
        std::string description;
    };

    // 将错误位映射到结构
    static ServoErrorInfo getServoErrorInfo(uint8_t error) {
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
                if (errorInfo.error == NO_ERROR) { // 只取第一个错误
                    errorInfo = err;
                } else {
                    errorInfo.description += "，" + err.description; // 连接多个错误信息
                }
            }
        }

        return errorInfo;
    }

// 从 speed_ratio 转换为 RPM
    static float speedRatioToRPM(float speed_ratio) {
        if (speed_ratio < -1.0f || speed_ratio > 1.0f) {
            throw std::out_of_range("速度比例超出范围 (-1.0 - 1.0)");
        }
        return speed_ratio * 62.0f;
    }

// 从 RPM 转换为 speed_ratio
    static float rpmToSpeedRatio(float rpm) {
        if (rpm < -62.0f || rpm > 62.0f) {
            throw std::out_of_range("RPM 超出范围 (-62.0 - 62.0)");
        }
        return rpm / 62.0f;
    }
} // namespace servo

#endif //UP_CORE_SERVO_PROTOCOL_H
