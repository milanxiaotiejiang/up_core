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


        Servo servo("/dev/ttyUSB0", 1000000);
        servo.init();
        servo.setDataCallback([&servo](const std::vector<uint8_t> &data) {
            servo.performSerialData(data);
        });

        // 检查串口是否打开
        if (servo.getSerial().isOpen()) {
            Logger::info("✅ 串口已打开：" + servo.getSerial().getPort());

            Logger::info("设备名称：" + servo.getSerial().getPort()
                         + ", 波特率：" + std::to_string(servo.getSerial().getBaudrate())
                         + ", 字节大小：" + std::to_string(static_cast<int>(servo.getSerial().getBytesize()))
                         + ", 停止位：" + std::to_string(static_cast<int>(servo.getSerial().getStopbits()))
                         + ", 奇偶校验：" + std::to_string(static_cast<int>(servo.getSerial().getParity())));
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