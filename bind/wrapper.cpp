//
// Created by noodles on 25-2-19.
//
#include <pybind11/pybind11.h>
#include "logger.h"
#include "servo.h"

#ifdef __linux__

#include "unix/adc.h"
#include "unix/gpio.h"
#include "unix/spi.h"

#endif

#include "servo_manager.h"
#include "servo_protocol_parse.h"
#include "firmware_update.h"
#include <pybind11/stl.h>
#include <pybind11/functional.h>

#include "system_up.h"

extern "C" {
#include "add.h"
}

namespace py = pybind11;

PYBIND11_MODULE(up_core, m) {
    m.doc() = "up_core module for python";

    // Add
    m.def("add", &add, "计算两个整数的和。"
          "参数："
          "a 第一个加数。"
          "b 第二个加数。"
          "返回值：两个整数的和。");
    m.def("system", &system, "A function that prints the system architecture");

    // Logger enum
    py::enum_<Logger::LogLevel>(m, "LogLevel")
            .value("DEBUG", Logger::DEBUG)
            .value("INFO", Logger::INFO)
            .value("WARNING", Logger::WARNING)
            .value("ERROR", Logger::ERROR)
            .value("OFF", Logger::OFF)
            .export_values();

    // Logger
    m.def("set_log_level", &Logger::setLogLevel, "Set the global log level")
            .def("get_log_level", &Logger::getLogLevel, "Get the current log level")
            .def("debug", &Logger::debug, "Log a DEBUG message")
            .def("info", &Logger::info, "Log an INFO message")
            .def("warning", &Logger::warning, "Log a WARNING message")
            .def("error", &Logger::error, "Log an ERROR message")
            .def("init_logger", []() {
                Logger::setLogLevel(Logger::INFO); // 设置默认日志级别为INFO
            }, "Initialize the Logger with INFO level");

#ifdef __linux__

        // GPIO
        py::class_<gpio::GPIO, std::shared_ptr<gpio::GPIO>>(m, "GPIO")
                .def(py::init<int, int>(), py::arg("chip_id"), py::arg("line_id"), "构造 GPIO 对象")
                .def("init", &gpio::GPIO::init, "初始化 GPIO")
                .def("setValue", &gpio::GPIO::setValue, py::arg("value"), "设置 GPIO 的电平")
                .def("release", &gpio::GPIO::release, "释放 GPIO 资源");

        py::class_<gpio::GPIOException>(m, "GPIOException")
                .def(py::init<const char *>())
                .def("what", &gpio::GPIOException::what);

        // ADC
        py::class_<adc::ADC, std::shared_ptr<adc::ADC>>(m, "ADC")
                .def(py::init<spi::SPI &, uint8_t>(), py::arg("spi"), py::arg("channel_count"), "构造 ADC 对象")
                .def("init", &adc::ADC::init, "初始化 ADC")
                .def("read_channel", &adc::ADC::readChannel, py::arg("channel"), "读取单个 ADC 通道")
                .def("read_all", &adc::ADC::readAll, "读取所有 ADC 通道");

        py::class_<adc::ADCException>(m, "ADCException")
                .def(py::init<const char *>())
                .def("what", &adc::ADCException::what);

        // SPI
        py::class_<spi::SPI, std::shared_ptr<spi::SPI>>(m, "SPI")
                .def(py::init<const std::string &, uint32_t, uint8_t, uint8_t>(),
                     py::arg("device"), py::arg("speed"), py::arg("mode"), py::arg("bits_per_word"),
                     "构造 SPI 对象")
                .def("init", &spi::SPI::init, "初始化 SPI 设备")
                .def("transfer", &spi::SPI::transfer, py::arg("tx"), py::arg("rx"), py::arg("length"),
                     "SPI 传输数据")
                .def("close", &spi::SPI::close, "关闭 SPI 设备");

        py::class_<spi::SPIException>(m, "SPIException")
                .def(py::init<const char *>())
                .def("what", &spi::SPIException::what);

#endif

    // servo::AlarmShutdownConfig
    py::enum_<servo::AlarmShutdownConfig>(m, "AlarmShutdownConfig")
            .value("NONE", servo::AlarmShutdownConfig::NONE)
            .value("INSTRUCTION_ERROR", servo::AlarmShutdownConfig::INSTRUCTION_ERROR)
            .value("CHECKSUM_ERROR", servo::AlarmShutdownConfig::CHECKSUM_ERROR)
            .value("RANGE_ERROR", servo::AlarmShutdownConfig::RANGE_ERROR)
            .value("OVERHEAT", servo::AlarmShutdownConfig::OVERHEAT)
            .value("ANGLE_ERROR", servo::AlarmShutdownConfig::ANGLE_ERROR)
            .value("VOLTAGE_ERROR", servo::AlarmShutdownConfig::VOLTAGE_ERROR)
            .export_values();

    // servo::AlarmLEDConfig
    py::enum_<servo::AlarmLEDConfig>(m, "AlarmLEDConfig")
            .value("NONE", servo::AlarmLEDConfig::NONE)
            .value("INSTRUCTION_ERROR", servo::AlarmLEDConfig::INSTRUCTION_ERROR)
            .value("OVERLOAD", servo::AlarmLEDConfig::OVERLOAD)
            .value("CHECKSUM_ERROR", servo::AlarmLEDConfig::CHECKSUM_ERROR)
            .value("RANGE_ERROR", servo::AlarmLEDConfig::RANGE_ERROR)
            .value("OVERHEAT", servo::AlarmLEDConfig::OVERHEAT)
            .value("ANGLE_ERROR", servo::AlarmLEDConfig::ANGLE_ERROR)
            .value("VOLTAGE_ERROR", servo::AlarmLEDConfig::VOLTAGE_ERROR)
            .export_values();

    // servo::StatusReturnLevel
    py::enum_<servo::StatusReturnLevel>(m, "StatusReturnLevel")
            .value("NO_RESPONSE", servo::StatusReturnLevel::NO_RESPONSE)
            .value("READ_ONLY", servo::StatusReturnLevel::READ_ONLY)
            .value("ALL_RESPONSE", servo::StatusReturnLevel::ALL_RESPONSE)
            .export_values();

    // servo::ORDER
    py::enum_<servo::ORDER>(m, "ORDER")
            .value("PING", servo::ORDER::PING)
            .value("READ_DATA", servo::ORDER::READ_DATA)
            .value("WRITE_DATA", servo::ORDER::WRITE_DATA)
            .value("REG_WRITE", servo::ORDER::REG_WRITE)
            .value("ACTION", servo::ORDER::ACTION)
            .value("RESET", servo::ORDER::RESET)
            .value("SYNC_WRITE", servo::ORDER::SYNC_WRITE)
            .export_values();

    // servo::EEPROM
    py::enum_<servo::EEPROM>(m, "EEPROM")
            .value("MODEL_NUMBER_L", servo::EEPROM::MODEL_NUMBER_L)
            .value("MODEL_NUMBER_H", servo::EEPROM::MODEL_NUMBER_H)
            .value("VERSION", servo::EEPROM::VERSION)
            .value("ID", servo::EEPROM::ID)
            .value("BAUDRATE", servo::EEPROM::BAUDRATE)
            .value("RETURN_DELAY_TIME", servo::EEPROM::RETURN_DELAY_TIME)
            .value("CW_ANGLE_LIMIT_L", servo::EEPROM::CW_ANGLE_LIMIT_L)
            .value("CW_ANGLE_LIMIT_H", servo::EEPROM::CW_ANGLE_LIMIT_H)
            .value("CCW_ANGLE_LIMIT_L", servo::EEPROM::CCW_ANGLE_LIMIT_L)
            .value("CCW_ANGLE_LIMIT_H", servo::EEPROM::CCW_ANGLE_LIMIT_H)
            .value("MAX_TEMPERATURE", servo::EEPROM::MAX_TEMPERATURE)
            .value("MIN_VOLTAGE", servo::EEPROM::MIN_VOLTAGE)
            .value("MAX_VOLTAGE", servo::EEPROM::MAX_VOLTAGE)
            .value("MAX_TORQUE_L", servo::EEPROM::MAX_TORQUE_L)
            .value("MAX_TORQUE_H", servo::EEPROM::MAX_TORQUE_H)
            .value("STATUS_RETURN_LEVEL", servo::EEPROM::STATUS_RETURN_LEVEL)
            .value("ALARM_LED", servo::EEPROM::ALARM_LED)
            .value("ALARM_SHUTDOWN", servo::EEPROM::ALARM_SHUTDOWN)
            .value("EEPROM_COUNT", servo::EEPROM::EEPROM_COUNT)
            .export_values();

    // servo::RAM
    py::enum_<servo::RAM>(m, "RAM")
            .value("TORQUE_ENABLE", servo::RAM::TORQUE_ENABLE)
            .value("LED", servo::RAM::LED)
            .value("CW_COMPLIANCE_MARGIN", servo::RAM::CW_COMPLIANCE_MARGIN)
            .value("CCW_COMPLIANCE_MARGIN", servo::RAM::CCW_COMPLIANCE_MARGIN)
            .value("CW_COMPLIANCE_SLOPE", servo::RAM::CW_COMPLIANCE_SLOPE)
            .value("CCW_COMPLIANCE_SLOPE", servo::RAM::CCW_COMPLIANCE_SLOPE)
            .value("GOAL_POSITION_L", servo::RAM::GOAL_POSITION_L)
            .value("GOAL_POSITION_H", servo::RAM::GOAL_POSITION_H)
            .value("MOVING_SPEED_L", servo::RAM::MOVING_SPEED_L)
            .value("MOVING_SPEED_H", servo::RAM::MOVING_SPEED_H)
            .value("ACCELERATION", servo::RAM::ACCELERATION)
            .value("DECELERATION", servo::RAM::DECELERATION)
            .value("PRESENT_POSITION_L", servo::RAM::PRESENT_POSITION_L)
            .value("PRESENT_POSITION_H", servo::RAM::PRESENT_POSITION_H)
            .value("PRESENT_SPEED_L", servo::RAM::PRESENT_SPEED_L)
            .value("PRESENT_SPEED_H", servo::RAM::PRESENT_SPEED_H)
            .value("PRESENT_LOAD_L", servo::RAM::PRESENT_LOAD_L)
            .value("PRESENT_LOAD_H", servo::RAM::PRESENT_LOAD_H)
            .value("PRESENT_VOLTAGE", servo::RAM::PRESENT_VOLTAGE)
            .value("TEMPERATURE", servo::RAM::TEMPERATURE)
            .value("REG_WRITE", servo::RAM::REG_WRITE)
            .value("MOVING", servo::RAM::MOVING)
            .value("LOCK", servo::RAM::LOCK)
            .value("MIN_PWM_L", servo::RAM::MIN_PWM_L)
            .value("MIN_PWM_H", servo::RAM::MIN_PWM_H)
            .value("RAM_COUNT", servo::RAM::RAM_COUNT)
            .export_values();

    // servo::ServoError
    py::enum_<servo::ServoError>(m, "ServoError")
            .value("NO_ERROR", servo::ServoError::NO_ERROR)
            .value("OUT_OF_RANGE", servo::ServoError::OUT_OF_RANGE)
            .value("OVERHEAT", servo::ServoError::OVERHEAT)
            .value("COMMAND_OUT_OF_RANGE", servo::ServoError::COMMAND_OUT_OF_RANGE)
            .value("CHECKSUM_ERROR", servo::ServoError::CHECKSUM_ERROR)
            .value("OVERLOAD", servo::ServoError::OVERLOAD)
            .value("INSTRUCTION_ERROR", servo::ServoError::INSTRUCTION_ERROR)
            .value("OVER_VOLTAGE_UNDER_VOLTAGE", servo::ServoError::OVER_VOLTAGE_UNDER_VOLTAGE)
            .export_values();

    // 绑定 ServoErrorInfo 结构体
    py::class_<servo::ServoErrorInfo>(m, "ServoErrorInfo")
            .def(py::init<>()) // 默认构造函数
            .def(py::init<servo::ServoError, const std::string &>()) // 指定参数类型
            .def_readwrite("error", &servo::ServoErrorInfo::error)
            .def_readwrite("description", &servo::ServoErrorInfo::description);

    // 绑定 getServoErrorInfo 函数
    m.def("get_servo_error_info", &servo::getServoErrorInfo, "获取舵机错误信息",
          py::arg("error"));

    // 绑定 Base 类
    py::class_<servo::Base>(m, "Base")
            .def(py::init<uint8_t>(), py::arg("id"))
            .def("getID", &servo::Base::getID, "获取舵机 ID")
            .def("buildShortPacket", &servo::Base::buildShortPacket, py::arg("write_length"), py::arg("commandData"))
            .def("buildCommandPacket", &servo::Base::buildCommandPacket, py::arg("command"), py::arg("address"),
                 py::arg("data"))
            .def("buildPingPacket", &servo::Base::buildPingPacket)
            .def("buildReadPacket", &servo::Base::buildReadPacket, py::arg("address"), py::arg("read_length"))
            .def("buildWritePacket", &servo::Base::buildWritePacket, py::arg("address"), py::arg("data"))
            .def("buildRegWritePacket", &servo::Base::buildRegWritePacket, py::arg("address"), py::arg("data"))
            .def("buildActionPacket", &servo::Base::buildActionPacket)
            .def("buildResetPacket", &servo::Base::buildResetPacket)
            .def("buildResetBootLoader", &servo::Base::buildResetBootLoader)
            .def("buildSyncWritePacket", &servo::Base::buildSyncWritePacket, py::arg("address"),
                 py::arg("write_length"), py::arg("protocols"), py::arg("func"));


    // servo::ServoEEPROM
    py::class_<servo::ServoEEPROM>(m, "ServoEEPROM")
            .def(py::init<uint8_t>(), py::arg("id"), "构造 ServoEEPROM 对象")
            .def("buildGetSoftwareVersion", &servo::ServoEEPROM::buildGetSoftwareVersion, "读取软件版本")
            .def("buildGetID", &servo::ServoEEPROM::buildGetID, "读取舵机 ID")
            .def("buildSetID", &servo::ServoEEPROM::buildSetID, py::arg("new_id"), "设置舵机 ID")
            .def("buildGetBaudrate", &servo::ServoEEPROM::buildGetBaudrate, "读取波特率")
            .def("buildSetBaudrate", &servo::ServoEEPROM::buildSetBaudrate, py::arg("baud"), "设置波特率")
            .def("buildGetReturnDelayTime", &servo::ServoEEPROM::buildGetReturnDelayTime, "读取返回延迟时间")
            .def("buildSetReturnDelayTime", &servo::ServoEEPROM::buildSetReturnDelayTime, py::arg("delay"),
                 "设置舵机返回数据的延迟时间（单位：微秒）")
            .def("buildGetCwAngleLimit", &servo::ServoEEPROM::buildGetCwAngleLimit, "读取顺时针角度限制")
            .def("buildGetCcwAngleLimit", &servo::ServoEEPROM::buildGetCcwAngleLimit, "读取逆时针角度限制")
            .def("buildGetAngleLimit", &servo::ServoEEPROM::buildGetAngleLimit, "读取角度限制")
            .def("buildSetAngleLimit", &servo::ServoEEPROM::buildSetAngleLimit, py::arg("min_angle"),
                 py::arg("max_angle"),
                 "设定角度限制")
            .def("buildGetMaxTemperature", &servo::ServoEEPROM::buildGetMaxTemperature, "读取最高温度上限")
            .def("buildSetMaxTemperature", &servo::ServoEEPROM::buildSetMaxTemperature, py::arg("temperature"),
                 "设定最大温度")
            .def("buildGetMinVoltage", &servo::ServoEEPROM::buildGetMinVoltage, "读取最低输入电压")
            .def("buildGetMaxVoltage", &servo::ServoEEPROM::buildGetMaxVoltage, "读取最高输入电压")
            .def("buildGetVoltageRange", &servo::ServoEEPROM::buildGetVoltageRange, "读取输入电压范围")
            .def("buildSetVoltageRange", &servo::ServoEEPROM::buildSetVoltageRange, py::arg("min_voltage"),
                 py::arg("max_voltage"), "设定电压范围")
            .def("buildGetMaxTorque", &servo::ServoEEPROM::buildGetMaxTorque, "读取最大扭矩")
            .def("buildSetMaxTorque", &servo::ServoEEPROM::buildSetMaxTorque, py::arg("torque"), "设定最大扭矩")
            .def("buildGetStatusReturnLevel", &servo::ServoEEPROM::buildGetStatusReturnLevel, "读取应答状态级别")
            .def("buildSetStatusReturnLevel", &servo::ServoEEPROM::buildSetStatusReturnLevel, py::arg("level"),
                 "设定应答返回级别")
            .def("buildGetAlarmLED", &servo::ServoEEPROM::buildGetAlarmLED, "读取 LED 闪烁报警条件")
            .def("buildSetAlarmLED", &servo::ServoEEPROM::buildSetAlarmLED, py::arg("config"), "设定 LED 报警")
            .def("buildGetAlarmShutdown", &servo::ServoEEPROM::buildGetAlarmShutdown, "读取卸载条件")
            .def("buildSetAlarmShutdown", &servo::ServoEEPROM::buildSetAlarmShutdown, py::arg("config"),
                 "设定报警卸载条件")
            .def("buildGetEepromData", &servo::ServoEEPROM::buildGetEepromData, py::arg("eeprom"), py::arg("length"),
                 "读取EEPROM数据");


    // servo::ServoRAM
    py::class_<servo::ServoRAM>(m, "ServoRAM")
            .def(py::init<uint8_t>(), py::arg("id"), "构造 ServoRAM 对象")
            .def("buildGetTorqueEnabled", &servo::ServoRAM::buildGetTorqueEnabled, "读取扭矩开关状态")
            .def("buildSetTorqueEnabled", &servo::ServoRAM::buildSetTorqueEnabled, py::arg("enable"), "使能/禁用扭矩")
            .def("buildGetLEDEnabled", &servo::ServoRAM::buildGetLEDEnabled, "读取 LED 状态")
            .def("buildSetLEDEnabled", &servo::ServoRAM::buildSetLEDEnabled, py::arg("enable"), "设置 LED 状态")
            .def("buildGetCwComplianceMargin", &servo::ServoRAM::buildGetCwComplianceMargin, "读取顺时针不灵敏区")
            .def("buildGetCcwComplianceMargin", &servo::ServoRAM::buildGetCcwComplianceMargin, "读取逆时针不灵敏区")
            .def("buildGetCwComplianceSlope", &servo::ServoRAM::buildGetCwComplianceSlope, "读取顺时针比例系数")
            .def("buildGetCcwComplianceSlope", &servo::ServoRAM::buildGetCcwComplianceSlope, "读取逆时针比例系数")
            .def("buildMoveToPosition", &servo::ServoRAM::buildMoveToPosition, py::arg("angle"),
                 "同步 控制舵机 直接移动到目标角度")
            .def("buildMoveToWithSpeedRpm", &servo::ServoRAM::buildMoveToWithSpeedRpm, py::arg("angle"),
                 py::arg("rpm"), "目标角度和速度 rpm")
            .def("buildAsyncMoveToPosition", &servo::ServoRAM::buildAsyncMoveToPosition, py::arg("angle"),
                 "异步写 (REG_WRITE)，舵机 不立即运动，等待 ACTION 指令")
            .def("buildActionCommand", &servo::ServoRAM::buildActionCommand, "REG_WRITE + ACTION")
            .def("buildSetAccelerationDeceleration", &servo::ServoRAM::buildSetAccelerationDeceleration,
                 py::arg("acceleration"), py::arg("deceleration"), "设置舵机运行的加速度和减速度")
            .def("buildGetGoalPosition", &servo::ServoRAM::buildGetGoalPosition, "读取目标位置")
            .def("buildGetRunSpeed", &servo::ServoRAM::buildGetRunSpeed, "读取运行速度")
            .def("buildGetPosition", &servo::ServoRAM::buildGetPosition, "读取当前位置")
            .def("buildGetSpeed", &servo::ServoRAM::buildGetSpeed, "读取当前速度")
            .def("buildGetAcceleration", &servo::ServoRAM::buildGetAcceleration, "读取加速度")
            .def("buildGetDeceleration", &servo::ServoRAM::buildGetDeceleration, "读取减速度")
            .def("buildGetAccelerationDeceleration", &servo::ServoRAM::buildGetAccelerationDeceleration,
                 "读取加速度和减速度")
            .def("buildGetLoad", &servo::ServoRAM::buildGetLoad, "读取当前负载")
            .def("buildGetVoltage", &servo::ServoRAM::buildGetVoltage, "读取当前电压")
            .def("buildGetTemperature", &servo::ServoRAM::buildGetTemperature, "读取当前温度")
            .def("buildCheckRegWriteFlag", &servo::ServoRAM::buildCheckRegWriteFlag, "检查 REG WRITE 是否等待执行")
            .def("buildCheckMovingFlag", &servo::ServoRAM::buildCheckMovingFlag, "检查舵机是否正在运行")
            .def("buildSetLockFlag", &servo::ServoRAM::buildSetLockFlag, py::arg("lock"), "设置锁标志")
            .def("buildGetLockFlag", &servo::ServoRAM::buildGetLockFlag, "读取锁标志")
            .def("buildSetMinPWM", &servo::ServoRAM::buildSetMinPWM, py::arg("pwm"), "设置最小PWM")
            .def("buildGetMinPWM", &servo::ServoRAM::buildGetMinPWM, "读取最小PWM")
            .def("buildGetRamData", &servo::ServoRAM::buildGetRamData, py::arg("ram"), py::arg("length"),
                 "读取 RAM 数据");

    py::class_<servo::Motor>(m, "Motor")
            .def(py::init<uint8_t>(), py::arg("id"), "构造 Motor 对象")
            .def("buildMotorMode", &servo::Motor::buildMotorMode, "设置舵机进入电机调速模式")
            .def("buildServoMode", &servo::Motor::buildServoMode, "设置舵机回到舵机模式")
            .def("buildSetMotorSpeed", &servo::Motor::buildSetMotorSpeed, py::arg("rpm"), "设置电机模式的转速")
            .def("buildRestoreAngleLimits", &servo::Motor::buildRestoreAngleLimits, "还原角度");

    py::class_<servo::ServoProtocol>(m, "ServoProtocol")
            .def(py::init<uint8_t>(), py::arg("id"), "构造 ServoProtocol 对象")
            .def_readwrite("eeprom", &servo::ServoProtocol::eeprom) // 暴露 eeprom
            .def_readwrite("ram", &servo::ServoProtocol::ram) // 暴露 ram
            .def_readwrite("motor", &servo::ServoProtocol::motor); // 暴露 motor

    // Bind the bytesize_t enum
    py::enum_<serial::bytesize_t>(m, "bytesize_t")
            .value("fivebits", serial::bytesize_t::fivebits)
            .value("sixbits", serial::bytesize_t::sixbits)
            .value("sevenbits", serial::bytesize_t::sevenbits)
            .value("eightbits", serial::bytesize_t::eightbits)
            .export_values();

    // Bind the parity_t enum
    py::enum_<serial::parity_t>(m, "parity_t")
            .value("parity_none", serial::parity_t::parity_none)
            .value("parity_odd", serial::parity_t::parity_odd)
            .value("parity_even", serial::parity_t::parity_even)
            .value("parity_mark", serial::parity_t::parity_mark)
            .value("parity_space", serial::parity_t::parity_space)
            .export_values();

    // Bind the stopbits_t enum
    py::enum_<serial::stopbits_t>(m, "stopbits_t")
            .value("stopbits_one", serial::stopbits_t::stopbits_one)
            .value("stopbits_two", serial::stopbits_t::stopbits_two)
            .value("stopbits_one_point_five", serial::stopbits_t::stopbits_one_point_five)
            .export_values();

    // Bind the flowcontrol_t enum
    py::enum_<serial::flowcontrol_t>(m, "flowcontrol_t")
            .value("flowcontrol_none", serial::flowcontrol_t::flowcontrol_none)
            .value("flowcontrol_software", serial::flowcontrol_t::flowcontrol_software)
            .value("flowcontrol_hardware", serial::flowcontrol_t::flowcontrol_hardware)
            .export_values();

    // Bind the Timeout struct
    py::class_<serial::Timeout>(m, "Timeout")
            .def(py::init<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>(),
                 py::arg("inter_byte_timeout") = 0,
                 py::arg("read_timeout_constant") = 0,
                 py::arg("read_timeout_multiplier") = 0,
                 py::arg("write_timeout_constant") = 0,
                 py::arg("write_timeout_multiplier") = 0)
            .def_static("max", &serial::Timeout::max)
            .def_static("simpleTimeout", &serial::Timeout::simpleTimeout)
            .def_readwrite("inter_byte_timeout", &serial::Timeout::inter_byte_timeout)
            .def_readwrite("read_timeout_constant", &serial::Timeout::read_timeout_constant)
            .def_readwrite("read_timeout_multiplier", &serial::Timeout::read_timeout_multiplier)
            .def_readwrite("write_timeout_constant", &serial::Timeout::write_timeout_constant)
            .def_readwrite("write_timeout_multiplier", &serial::Timeout::write_timeout_multiplier);

    // Bind the Serial class
    py::class_<serial::Serial, std::shared_ptr<serial::Serial> >(m, "Serial")
            .def(py::init<const std::string &, uint32_t, serial::Timeout, serial::bytesize_t, serial::parity_t,
                     serial::stopbits_t, serial::flowcontrol_t>(),
                 py::arg("port") = "",
                 py::arg("baudrate") = 9600,
                 py::arg("timeout") = serial::Timeout(),
                 py::arg("bytesize") = serial::eightbits,
                 py::arg("parity") = serial::parity_none,
                 py::arg("stopbits") = serial::stopbits_one,
                 py::arg("flowcontrol") = serial::flowcontrol_none)
            .def("open", &serial::Serial::open)
            .def("isOpen", &serial::Serial::isOpen)
            .def("close", &serial::Serial::close)
            .def("available", &serial::Serial::available)
            .def("waitReadable", &serial::Serial::waitReadable)
            .def("waitByteTimes", &serial::Serial::waitByteTimes)

            // 绑定第一个 read 函数：接收 uint8_t* 缓冲区和大小
            .def("read_bytes", [](serial::Serial &self, py::buffer &buffer, size_t size) {
                // 获取 Python 缓冲区的信息
                py::buffer_info buf_info = buffer.request();
                // 检查缓冲区大小是否足够
                if (buf_info.size < size) {
                    throw std::runtime_error("Buffer size is smaller than the requested read size");
                }
                // 获取指向缓冲区的指针
                uint8_t *ptr = static_cast<uint8_t *>(buf_info.ptr);
                // 调用 C++ 的 read 函数
                size_t bytes_read = self.read(ptr, size);
                // 返回读取的字节数
                return bytes_read;
            })

            // 绑定第二个 read 函数：接收 std::vector<uint8_t>
            .def("read", [](serial::Serial &self, size_t size) {
                std::vector<uint8_t> buffer;
                // 调用 C++ 的 std::vector 版本的 read 函数
                size_t bytes_read = self.read(buffer, size);
                // 返回读取的数据和读取的字节数
                return py::make_tuple(buffer, bytes_read);
            })

            // 绑定第一个 read 函数：接收 std::string 引用
            .def("read_string", [](serial::Serial &self, size_t size) {
                std::string buffer;
                // 调用 C++ 的 std::string 版本的 read 函数
                size_t bytes_read = self.read(buffer, size);
                // 返回读取的数据和字节数
                return py::make_tuple(buffer, bytes_read);
            })

            // 绑定第二个 read 函数：返回 std::string
            .def("read_str", [](serial::Serial &self, size_t size) {
                // 直接调用 C++ 中返回 std::string 的 read 函数
                std::string result = self.read(size);
                // 返回结果
                return result;
            })

            // 绑定第一个 readline 函数：接收 string 引用
            .def("readline_buffer", [](serial::Serial &self, size_t size, const std::string &eol) {
                std::string buffer;
                // 调用 C++ 的 readline 函数
                size_t bytes_read = self.readline(buffer, size, eol);
                // 返回读取的数据和字节数
                return py::make_tuple(buffer, bytes_read);
            })

            // 绑定第二个 readline 函数：返回 string
            .def("readline", [](serial::Serial &self, size_t size, const std::string &eol) {
                // 直接调用 C++ 中返回 std::string 的 readline 函数
                std::string result = self.readline(size, eol);
                // 返回结果
                return result;
            })

            // 绑定 readlines 函数：返回 vector<string>
            .def("readlines", [](serial::Serial &self, size_t size, const std::string &eol) {
                // 调用 C++ 中的 readlines 函数
                std::vector<std::string> lines = self.readlines(size, eol);
                // 返回 Python 列表
                return py::cast(lines);
            })

            .def("write", py::overload_cast<const uint8_t *, size_t>(&serial::Serial::write))
            .def("write", py::overload_cast<const std::vector<uint8_t> &>(&serial::Serial::write))
            .def("write", py::overload_cast<const std::string &>(&serial::Serial::write))
            .def("setPort", &serial::Serial::setPort)
            .def("getPort", &serial::Serial::getPort)
            .def("setTimeout", py::overload_cast<serial::Timeout &>(&serial::Serial::setTimeout))
            .def("setTimeout",
                 py::overload_cast<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>(&serial::Serial::setTimeout))
            .def("getTimeout", &serial::Serial::getTimeout)
            .def("setBaudrate", &serial::Serial::setBaudrate)
            .def("getBaudrate", &serial::Serial::getBaudrate)
            .def("setBytesize", &serial::Serial::setBytesize)
            .def("getBytesize", &serial::Serial::getBytesize)
            .def("setParity", &serial::Serial::setParity)
            .def("getParity", &serial::Serial::getParity)
            .def("setStopbits", &serial::Serial::setStopbits)
            .def("getStopbits", &serial::Serial::getStopbits)
            .def("setFlowcontrol", &serial::Serial::setFlowcontrol)
            .def("getFlowcontrol", &serial::Serial::getFlowcontrol)
            .def("flush", &serial::Serial::flush)
            .def("flushInput", &serial::Serial::flushInput)
            .def("flushOutput", &serial::Serial::flushOutput)
            .def("sendBreak", &serial::Serial::sendBreak)
            .def("setBreak", &serial::Serial::setBreak)
            .def("setRTS", &serial::Serial::setRTS)
            .def("setDTR", &serial::Serial::setDTR)
            .def("waitForChange", &serial::Serial::waitForChange)
            .def("getCTS", &serial::Serial::getCTS)
            .def("getDSR", &serial::Serial::getDSR)
            .def("getRI", &serial::Serial::getRI)
            .def("getCD", &serial::Serial::getCD);

    // Bind the SerialException class
    py::class_<serial::SerialException>(m, "SerialException")
            .def(py::init<const char *>())
            .def("what", &serial::SerialException::what);

    // Bind the IOException class
    py::class_<serial::IOException>(m, "IOException")
            .def(py::init<std::string, int, int>())
            .def(py::init<std::string, int, const char *>())
            .def("what", &serial::IOException::what)
            .def("getErrorNumber", &serial::IOException::getErrorNumber);

    // Bind the PortNotOpenedException class
    py::class_<serial::PortNotOpenedException>(m, "PortNotOpenedException")
            .def(py::init<const char *>())
            .def("what", &serial::PortNotOpenedException::what);

    // Bind the PortInfo struct
    py::class_<serial::PortInfo>(m, "PortInfo")
            .def_readonly("port", &serial::PortInfo::port)
            .def_readonly("description", &serial::PortInfo::description)
            .def_readonly("hardware_id", &serial::PortInfo::hardware_id);

    // Bind the list_ports function
    m.def("list_ports", &serial::list_ports);

    // Servo
    py::class_<Servo>(m, "Servo")
#ifdef __linux__
        .def(py::init<std::shared_ptr<serial::Serial>, std::shared_ptr<gpio::GPIO>>(),
             py::arg("serial"), py::arg("gpio") = nullptr, "构造 Servo 对象")
#elif _WIN32
            .def(py::init<std::shared_ptr<serial::Serial> >(),
                 py::arg("serial"), "构造 Servo 对象")
#endif
            .def("init", &Servo::init, "Initialize the servo")
            .def("close", &Servo::close, "Close the servo connection")
            .def("send_command", &Servo::sendCommand, py::arg("frame"), "Send command to the servo")
            .def("set_data_callback", &Servo::setDataCallback, "Set a data reception callback");

    // 绑定 ServoManager 类
    py::class_<ServoManager>(m, "ServoManager")
            .def_static("instance", &ServoManager::instance, py::return_value_policy::reference, "获取单例实例")
            .def("searching", &ServoManager::searching, "是否正在搜索舵机")
            .def("setSearchTimeout", &ServoManager::setSearchTimeout, py::arg("timeout"), "设置搜索超时时间")
            .def("setVerify", &ServoManager::setVerify, py::arg("verify"), "设置校验标志")
            .def("setCallback", &ServoManager::setCallback, py::arg("callback"), "设置回调函数")
            .def("startSearchServoID", &ServoManager::startSearchServoID, py::arg("port"), py::arg("baudrates"),
                 "启动舵机搜索")
            .def("stopSearchServoID", &ServoManager::stopSearchServoID, "停止舵机搜索");


    // 绑定 servo 命名空间中的全局函数
    m.def("speedRatioToRPM", &servo::speedRatioToRPM, py::arg("speed_ratio"), "从速度比例转换为 RPM");
    m.def("rpmToSpeedRatio", &servo::rpmToSpeedRatio, py::arg("rpm"), "从 RPM 转换为速度比例");

    m.def("bytesToHex", &bytesToHex, py::arg("data"), "Convert bytes to hex string");
    m.def("singleByteToInt", &singleByteToInt, py::arg("byte"), "Convert single byte to int");
    m.def("doubleByteToInt", &doubleByteToInt, py::arg("lowByte"), py::arg("highByte"), "Convert double byte to int");
    m.def("combineSpeed", &combineSpeed, py::arg("lowByte"), py::arg("highByte"),
          "Combine low byte and high byte to speed");
    m.def("combinePosition", &combinePosition, py::arg("lowByte"), py::arg("highByte"),
          "Combine low byte and high byte to position");
    m.def("previewSerialData", &previewSerialData, py::arg("packet"), "Preview serial data");
    m.def("performExtractID", &performExtractID, py::arg("packet"), "Perform extract ID");

    m.def("eePROMValue", &servo::eePROMValue, "Print EEPROM value", py::arg("eeprom"), py::arg("value"));
    m.def("parseEEPROMData", &servo::parseEEPROMData, "Parse EEPROM data", py::arg("data"),
          py::arg("start") = servo::EEPROM::MODEL_NUMBER_L);
    m.def("ramValue", &servo::ramValue, "Print RAM value", py::arg("ram"), py::arg("value"));
    m.def("parseRAMData", &servo::parseRAMData, "Parse RAM data", py::arg("data"),
          py::arg("start") = servo::RAM::TORQUE_ENABLE);


    m.def("setConsoleOutputCP", &setConsoleOutputCP, "");

    py::class_<FirmwareUpdate>(m, "FirmwareUpdate")
            .def(py::init<>()) // 绑定构造函数
            // 绑定 upgrade 方法，并支持默认参数
            .def("upgrade_path", &FirmwareUpdate::upgrade_path,
                 py::arg("port_input"),
                 py::arg("baud_rate"),
                 py::arg("bin_path"),
                 py::arg("servo_id"),
                 py::arg("total_retry") = 10,
                 py::arg("handshake_threshold") = 5,
                 py::arg("frame_retry_count") = 5,
                 py::arg("sign_retry_count") = 5,
                 "升级固件的方法")
            .def("upgrade_stream", &FirmwareUpdate::upgrade_stream,
                 py::arg("port_input"),
                 py::arg("baud_rate"),
                 py::arg("fileBuffer"),
                 py::arg("servo_id"),
                 py::arg("total_retry") = 10,
                 py::arg("handshake_threshold") = 5,
                 py::arg("frame_retry_count") = 5,
                 py::arg("sign_retry_count") = 5,
                 "升级固件的方法");
}
