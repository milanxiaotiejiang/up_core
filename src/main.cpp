//
// Created by noodles on 25-2-20.
//
#include <iostream>
#include "servo.h"
#include "serial/serial.h"
#include "logger.h"
#include <unistd.h>
#include <iomanip>
#include <thread>
#include <chrono>
#include "servo_manager.h"
#include "firmware_update.h"
#include "servo_protocol_parse.h"

int searchServo();

void listPorts();

void getSoftwareVersion(Servo &servo);

void search(Servo &servo);

void changeMode(Servo &servo, servo::ServoProtocol &servoProtocol);

void runServo(Servo &servo, servo::ServoProtocol &servoProtocol);

void runMotor(Servo &servo, servo::ServoProtocol &servoProtocol);

void loadInfo(Servo &servo, servo::ServoProtocol &servoProtocol);

void reset(Servo &servo, servo::ServoProtocol &servoProtocol);

int main() {

    Logger::setLogLevel(Logger::DEBUG);

    // 搜索舵机 ID
//    return searchServo();


    try {
        listPorts();


        std::shared_ptr<serial::Serial> serialPtr =
                std::make_shared<serial::Serial>("/dev/ttyUSB0", 1000000, serial::Timeout::simpleTimeout(1000));

        Servo servo(serialPtr);
        servo.init();
        servo.setDataCallback([&servo](const std::vector<uint8_t> &data) {
            servo.performSerialData(data);
        });

        // 检查串口是否打开
        if (serialPtr->isOpen()) {
            Logger::info("✅ 串口已打开：" + serialPtr->getPort());

            Logger::info("设备名称：" + serialPtr->getPort()
                         + ", 波特率：" + std::to_string(serialPtr->getBaudrate())
                         + ", 字节大小：" + std::to_string(static_cast<int>(serialPtr->getBytesize()))
                         + ", 停止位：" + std::to_string(static_cast<int>(serialPtr->getStopbits()))
                         + ", 奇偶校验：" + std::to_string(static_cast<int>(serialPtr->getParity())));
        } else {
            Logger::error("❌ 串口打开失败！");
            return -1;
        }

        sleep(1);


        // 获取版本
        getSoftwareVersion(servo);


        // 搜索
//        search(servo);

//        servo::ServoProtocol servoProtocol(0x01);

        // 模式
//        changeMode(servo, servoProtocol);

//        runServo(servo, servoProtocol);

//        runMotor(servo, servoProtocol);

        // 速度、温度、负载、电压、位置
//        loadInfo(servo, servoProtocol);

//        reset(servo, servoProtocol);

//        servo.close();


        // 固件升级
//        FirmwareUpdate sender;
//        sender.upgrade_path("/dev/ttyUSB0", 1000000,
//                            "/home/noodles/CLionProjects/up_core/file/CDS5516_1.0.bin", 0x01);
//
//        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        servo.close();
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }

}

void reset(Servo &servo, servo::ServoProtocol &servoProtocol) {
    // ff ff 01 02 06 f6
    const std::vector<uint8_t> &resetPacket = servoProtocol.buildResetPacket();
    Logger::info("发送复位命令：" + bytesToHex(resetPacket));

    bool success = servo.sendCommand(resetPacket);
    if (success) {
        Logger::info("✅ 发送复位命令成功！");
    } else {
        Logger::error("❌ 发送复位命令失败！");
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

}

void loadInfo(Servo &servo, servo::ServoProtocol &servoProtocol) {// buildGetEepromData 读取 EEPROM 数据
// buildGetAngleLimit 读取角度限制
// buildGetMaxTemperature 读取最高温度上限
// buildGetVoltageRange 读取输入电压范围
// buildGetMaxTorque 读取最大扭矩
    {
        std::vector<uint8_t> cmd = servoProtocol.eeprom.buildGetEepromData(
                servo::EEPROM::MODEL_NUMBER_L,
                static_cast<int>(servo::EEPROM::EEPROM_COUNT) - static_cast<int>(servo::EEPROM::MODEL_NUMBER_L) - 1
        );
        Logger::info("发送命令：" + bytesToHex(cmd));
        std::vector<uint8_t> response_data;
        bool success = servo.sendWaitCommand(cmd, response_data);
        if (success) {
            success = servo.performSerialData(response_data);
            if (success) {
                std::vector<uint8_t> payload(response_data.begin() + 5, response_data.end() - 1);
                servo::parseEEPROMData(payload);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }

    // buildGetRam 读取 RAM 数据
// buildGetLEDEnabled 读取 LED 状态
// buildGetGoalPosition 读取目标位置
// buildGetRunSpeed 读取运行速度
// buildGetAccelerationDeceleration 读取加碱速度
// buildGetPosition 读取当前位置
// buildGetSpeed 读取当前速度
// buildGetLoad 读取负载
// buildGetVoltage 读取电压
// buildGetTemperature 读取温度
    {
        std::vector<uint8_t> cmd = servoProtocol.ram.buildGetRamData(
                servo::RAM::TORQUE_ENABLE,
                static_cast<int>(servo::RAM::RAM_COUNT) - static_cast<int>(servo::RAM::TORQUE_ENABLE) - 1
        );
        Logger::info("发送命令：" + bytesToHex(cmd));
        std::vector<uint8_t> response_data;
        bool success = servo.sendWaitCommand(cmd, response_data);
        if (success) {
            success = servo.performSerialData(response_data);
            if (success) {
                std::vector<uint8_t> payload(response_data.begin() + 5, response_data.end() - 1);
                auto map = servo::parseRAMData(payload);

                int speed = combineSpeed(map[servo::RAM::MOVING_SPEED_L], map[servo::RAM::MOVING_SPEED_H]);
                Logger::info("运行速度：" + std::to_string(speed));
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }
}

void runMotor(Servo &servo, servo::ServoProtocol &servoProtocol) {
    {
        // **RPM 顺时针**
        std::vector<uint8_t> cmd = servoProtocol.motor.buildSetMotorSpeed(32.0f);

        std::vector<uint8_t> response_data;
        bool success = servo.sendWaitCommand(cmd, response_data);
        if (success) {
            // 处理响应数据
            servo.performSerialData(response_data);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    }
    {
        // **RPM 逆时针**
        std::vector<uint8_t> cmd = servoProtocol.motor.buildSetMotorSpeed(-32.0f);

        std::vector<uint8_t> response_data;
        bool success = servo.sendWaitCommand(cmd, response_data);
        if (success) {
            // 处理响应数据
            servo.performSerialData(response_data);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    }
    {
        // **停止 (速度 0)**
        std::vector<uint8_t> cmd = servoProtocol.motor.buildSetMotorSpeed(0.0f);

        std::vector<uint8_t> response_data;
        bool success = servo.sendWaitCommand(cmd, response_data);
        if (success) {
            // 处理响应数据
            servo.performSerialData(response_data);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    }
}

void runServo(Servo &servo, servo::ServoProtocol &servoProtocol) {
    // FF FF 00 07 03 1E 00 02 00 02 D3     0 号舵机以中速运动至 150°的位置，中速 0x01ff，150° 0x01ff
    // FF FF 01 07 03 1E 00 02 FF 01 D4     1 号舵机以中速运动至 150°的位置，中速 0x01ff，150° 0x01ff
    std::vector<uint8_t> cmd = servoProtocol.ram.buildMoveToWithSpeedRpm(300.0f, 31.0f);
    Logger::info("发送命令：" + bytesToHex(cmd));

    std::vector<uint8_t> response_data;
    bool success = servo.sendWaitCommand(cmd, response_data);
    if (success) {
        // 处理响应数据
        servo.performSerialData(response_data);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}

void changeMode(Servo &servo, servo::ServoProtocol &servoProtocol) {
    {
        const std::vector<uint8_t> &mode = servoProtocol.motor.buildServoMode();
//            const std::vector<uint8_t> &mode = servoProtocol.motor.buildMotorMode();
        Logger::info("发送命令：" + bytesToHex(mode));

        std::vector<uint8_t> response_data;
        bool success = servo.sendWaitCommand(mode, response_data);
        if (success) {
            // 处理响应数据
            servo.performSerialData(response_data);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

void search(Servo &servo) {

    // 	m_SendBuf[0] = 0xff;
    //	m_SendBuf[1] = 0xff;
    //	m_SendBuf[2] = aSid;
    //	m_SendBuf[3] = 0x04;
    //	m_SendBuf[4] = 0x02;
    //	m_SendBuf[5] = 0x1E;
    //	m_SendBuf[6] = 0x0C;
    //
    //	m_nSendLen = 7;

    // FF FF 01 04 02 1E 0C CE
    // FF FF 01 0E 00 6C 01 00 00 20 20 6D 01 00 00 00 00 D5
    //                6C 01 00 00 20 20 6D 01 00 00 00 00
    {
        servo::ServoProtocol servoProtocol(0x01);
//            auto data = servoProtocol.eeprom.buildCommandPacket(servo::ORDER::READ_DATA,
//                                                                static_cast<uint8_t>(servo::RAM::GOAL_POSITION_L),
//                                                                {0x0C});
        auto data = servoProtocol.buildReadPacket(servo::HEAD_ADDRESS, 0x0C);

        Logger::info("发送命令后收到数据：" + bytesToHex(data));
        std::vector<uint8_t> response_data;
        bool success = servo.sendWaitCommand(data, response_data);

        if (success) {
            // 处理响应数据
            servo.performSerialData(response_data);
        }
        std::this_thread::sleep_for(std::chrono::microseconds(1000));
    }


    {
        servo::ServoProtocol servoProtocol1(1);
        auto data1 = servoProtocol1.buildPingPacket();
        std::vector<uint8_t> response_data;
        bool success1 = servo.sendWaitCommand(data1, response_data);
        if (success1) {
            servo.performSerialData(response_data);
        }

        for (int i = 0; i < 254; ++i) {
            // 等待 100 ms
            std::this_thread::sleep_for(std::chrono::microseconds(1000));
            Logger::info("搜索 ID: " + std::to_string(i));
            servo::ServoProtocol servoProtocol(i);
            auto data = servoProtocol.buildPingPacket();
            bool success = servo.sendCommand(data);
        }

        std::this_thread::sleep_for(std::chrono::microseconds(10000));
    }
}

void getSoftwareVersion(Servo &servo) {/**
 * FF
 * FF
 * 01 id (每个舵机都有一个 ID 号。ID 号范围 0～253,转换为十六进制 0X00～0XFD。)(ID 号 254 为广播 ID,若控制器发出的 ID 号为 254(0XFE)，所有的舵机均接收指令，但都不返回应答信息。)
 * 04 length
 * 02 instruction ([PING:0x01] [READ DATA:0x02] [WRITE DATA:0x03] [REG WRITE:0x04] [ACTION:0x05] [RESET:0x06] [SYNC WRITE:0x83])
 * 02 ([PING:无] [READ DATA:数据读出段的首地址] [WRITE DATA:数据写入段的首地址])
 * 01 ([PING:无] [READ DATA:读取数据的长度] [WRITE DATA:写入的第一个数据、依次类推])
 * F5 check sum
 */
    // FF FF 01 04 02 02 01 F5

    /**
 * FF
 * FF
 * 01 id
 * 03 length
 * 00 error
 * 0A
 * F1
 */
    // FF FF 01 03 00 0A F1
    {
        servo::ServoProtocol servoProtocol(0x01);
        auto data = servoProtocol.eeprom.buildGetSoftwareVersion();
        std::vector<uint8_t> response_data;
        bool success = servo.sendWaitCommand(data, response_data);

        if (success) {
            // 处理响应数据
            servo.performSerialData(response_data);
        }

        sleep(1);
    }
}

void listPorts() {//        const std::vector<serial::PortInfo> &listPorts = serial::list_ports();
//        for (const auto &item: listPorts) {
//            Logger::info("设备名称：" + item.port + ", 描述：" + item.description + ", 硬件 ID：" + item.hardware_id);
//        }


}

int searchServo() {
    Logger::info(ServoManager::instance().searching() ? "正在搜索中..." : "搜索已停止");

    std::thread stopThread([]() {
//        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
//        ServoManager::instance().stopSearchServoID();
    });
    stopThread.detach();

    ServoManager::instance().setCallback([](int baud, int id, int error) {
        Logger::info("----------------      ID: " + std::to_string(id) + ", 错误码: " + std::to_string(error));
    });
    ServoManager::instance().startSearchServoID("/dev/ttyUSB0", {1000000});


    std::this_thread::sleep_for(std::chrono::milliseconds(30000));

    return 0;
}

