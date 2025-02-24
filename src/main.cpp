//
// Created by noodles on 25-2-20.
//
#include <iostream>
#include "servo.h"
#include "serial/serial.h"
#include "logger.h"
#include <unistd.h>
#include <iomanip>

int main() {

    Logger::setLogLevel(Logger::INFO);

    try {
        servo::ServoProtocol servoProtocol1(0x01);
        servoProtocol1.eeprom.buildGetSoftwareVersion();

        const std::vector<serial::PortInfo> &listPorts = serial::list_ports();
        for (const auto &item: listPorts) {
            Logger::info("设备名称：" + item.port + ", 描述：" + item.description + ", 硬件 ID：" + item.hardware_id);
        }

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

        sleep(2);


        servo::ServoProtocol servoProtocol(0x01);
        auto ref = servo.sendCommand(servoProtocol.eeprom.buildGetSoftwareVersion());
        Logger::error(ref ? "发送成功" : "发送失败");

        sleep(2);

        servo.close();


    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }

}