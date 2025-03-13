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

int main() {

    Logger::setLogLevel(Logger::INFO);

    // 固件升级

//    FirmwareUpdate sender;
//    try {
//        sender.sendData("example.bin", 3);
//    } catch (const std::exception& e) {
//        std::cerr << e.what() << std::endl;
//    }
//    return 0;

    // 搜索舵机 ID

//    Logger::info(ServoManager::instance().searching() ? "正在搜索中..." : "搜索已停止");
//
//    std::thread stopThread([]() {
////        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
////        ServoManager::instance().stopSearchServoID();
//    });
//    stopThread.detach();
//
//    ServoManager::instance().setCallback([](int baud, int id, int error) {
//        Logger::info("----------------      ID: " + std::to_string(id) + ", 错误码: " + std::to_string(error));
//    });
//    ServoManager::instance().startSearchServoID("/dev/ttyUSB0", {1000000});
//
//
//    std::this_thread::sleep_for(std::chrono::milliseconds(30000));
//
//    return 0;

    try {

//        const std::vector<serial::PortInfo> &listPorts = serial::list_ports();
//        for (const auto &item: listPorts) {
//            Logger::info("设备名称：" + item.port + ", 描述：" + item.description + ", 硬件 ID：" + item.hardware_id);
//        }


        std::shared_ptr<serial::Serial> serialPtr =
                std::make_shared<serial::Serial>("/dev/ttyUSB0", 1000000, serial::Timeout::simpleTimeout(1000));

        Servo servo(serialPtr);
        servo.init();
//        servo.setDataCallback([&servo](const std::vector<uint8_t> &data) {
//            servo.performSerialData(data);
//        });

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
        /**
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
//        {
//            servo::ServoProtocol servoProtocol(0x01);
//            auto data = servoProtocol.eeprom.buildGetSoftwareVersion();
//            std::vector<uint8_t> response_data;
//            bool success = servo.sendWaitCommand(data, response_data);
//
//            if (success) {
//                // 处理响应数据
//                servo.performSerialData(response_data);
//            }
//
//            sleep(1);
//        }


        // 搜索
//        {
//
//            // 	m_SendBuf[0] = 0xff;
//            //	m_SendBuf[1] = 0xff;
//            //	m_SendBuf[2] = aSid;
//            //	m_SendBuf[3] = 0x04;
//            //	m_SendBuf[4] = 0x02;
//            //	m_SendBuf[5] = 0x1E;
//            //	m_SendBuf[6] = 0x0C;
//            //
//            //	m_nSendLen = 7;
//
//            // FF FF 01 04 02 1E 0C CE
//            // FF FF 01 0E 00 6C 01 00 00 20 20 6D 01 00 00 00 00 D5
//            //                6C 01 00 00 20 20 6D 01 00 00 00 00
//            servo::ServoProtocol servoProtocol(0x01);
////            auto data = servoProtocol.eeprom.buildCommandPacket(servo::ORDER::READ_DATA,
////                                                                static_cast<uint8_t>(servo::RAM::GOAL_POSITION_L),
////                                                                {0x0C});
//            auto data = servoProtocol.buildReadPacket(servo::HEAD_ADDRESS, 0x0C);
//
//            Logger::info("发送命令后收到数据：" + bytesToHex(data));
//            std::vector<uint8_t> response_data;
//            bool success = servo.sendWaitCommand(data, response_data);
//
//            if (success) {
//                // 处理响应数据
//                servo.performSerialData(response_data);
//            }
//
//            std::this_thread::sleep_for(std::chrono::microseconds(1000));
//        }

//        {
//            servo.setDataCallback([&servo](const std::vector<uint8_t> &data) {
//                servo.performSerialData(data);
//            });
//
//            servo::ServoProtocol servoProtocol1(1);
//            auto data1 = servoProtocol1.buildPingPacket();
//            std::vector<uint8_t> response_data;
//            bool success1 = servo.sendWaitCommand(data1, response_data);
//            if (success1) {
//                servo.performSerialData(response_data);
//            }
//
//            for (int i = 0; i < 254; ++i) {
//                // 等待 100 ms
//                std::this_thread::sleep_for(std::chrono::microseconds(1000));
//                Logger::info("搜索 ID: " + std::to_string(i));
//                servo::ServoProtocol servoProtocol(i);
//                auto data = servoProtocol.buildPingPacket();
//                bool success = servo.sendCommand(data);
//            }
//
//            std::this_thread::sleep_for(std::chrono::microseconds(10000));
//        }

//        {
//            servo::ServoProtocol servoProtocol(0x01);
//            const std::vector<uint8_t> &vector = servoProtocol.motor.buildMotorMode();
//            for (const auto &item: vector) {
//                std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(item) << " ";
//            }
//            std::cout << std::endl;
//            const std::vector<uint8_t> &mode = servoProtocol.motor.buildServoMode();
//            for (const auto &item: mode) {
//                std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(item) << " ";
//            }
//            std::cout << std::endl;
//
//            serialPtr->write(vector);
//            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//            serialPtr->write(mode);
//            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//
//        }

        servo::ServoProtocol servoProtocol(0x01);

//        {
//            const std::vector<uint8_t> &mode = servoProtocol.motor.buildServoMode();
////            const std::vector<uint8_t> &mode = servoProtocol.motor.buildMotorMode();
//            Logger::info("发送命令：" + bytesToHex(mode));
//
//            std::vector<uint8_t> response_data;
//            bool success = servo.sendWaitCommand(mode, response_data);
//            if (success) {
//                // 处理响应数据
//                servo.performSerialData(response_data);
//            }
//
//            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//        }

//        {
//            // FF FF 00 07 03 1E 00 02 00 02 D3     0 号舵机以中速运动至 150°的位置，中速 0x01ff，150° 0x01ff
//            // FF FF 01 07 03 1E 00 02 FF 01 D4     1 号舵机以中速运动至 150°的位置，中速 0x01ff，150° 0x01ff
//            std::vector<uint8_t> cmd = servoProtocol.ram.buildMoveToWithSpeedRpm(300.0f, 31.0f);
//            Logger::info("发送命令：" + bytesToHex(cmd));
//
//            std::vector<uint8_t> response_data;
//            bool success = servo.sendWaitCommand(cmd, response_data);
//            if (success) {
//                // 处理响应数据
//                servo.performSerialData(response_data);
//            }
//
//            std::this_thread::sleep_for(std::chrono::milliseconds(5000));
//        }

//        {
//            // **RPM 顺时针**
//            std::vector<uint8_t> cmd = servoProtocol.motor.buildSetMotorSpeed(32.0f);
//
//            std::vector<uint8_t> response_data;
//            bool success = servo.sendWaitCommand(cmd, response_data);
//            if (success) {
//                // 处理响应数据
//                servo.performSerialData(response_data);
//            }
//
//            std::this_thread::sleep_for(std::chrono::milliseconds(5000));
//        }
//        {
//            // **RPM 逆时针**
//            std::vector<uint8_t> cmd = servoProtocol.motor.buildSetMotorSpeed(-32.0f);
//
//            std::vector<uint8_t> response_data;
//            bool success = servo.sendWaitCommand(cmd, response_data);
//            if (success) {
//                // 处理响应数据
//                servo.performSerialData(response_data);
//            }
//
//            std::this_thread::sleep_for(std::chrono::milliseconds(5000));
//        }
//        {
//            // **停止 (速度 0)**
//            std::vector<uint8_t> cmd = servoProtocol.motor.buildSetMotorSpeed(0.0f);
//
//            std::vector<uint8_t> response_data;
//            bool success = servo.sendWaitCommand(cmd, response_data);
//            if (success) {
//                // 处理响应数据
//                servo.performSerialData(response_data);
//            }
//
//            std::this_thread::sleep_for(std::chrono::milliseconds(5000));
//        }

//        {
//            const std::vector<uint8_t> &angleLimit = servoProtocol.eeprom.buildGetCwAngleLimit();
//
//            std::vector<uint8_t> response_data;
//            bool success = servo.sendWaitCommand(angleLimit, response_data);
//            if (success) {
//                // ff ff 01 06 02 00 00 00 00 f6 电机
//                // ff ff 01 06 00 00 00 ff 03 f6 舵机
//                servo.performSerialData(response_data);
//            }
//            std::this_thread::sleep_for(std::chrono::milliseconds(5000));
//        }

//        {
//            // ff ff 01 03 06 f5
//            // FF FF 01 03 06 F5
//            const std::vector<uint8_t> &vector = servoProtocol.buildResetPacket();
//            Logger::info("发送命令：" + bytesToHex(vector));
//
//            bool success = servo.sendCommand(vector);
//            if (success) {
//                Logger::info("复位成功");
//            }
//            std::this_thread::sleep_for(std::chrono::milliseconds(5000));
//        }

        servo.close();


    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }

}