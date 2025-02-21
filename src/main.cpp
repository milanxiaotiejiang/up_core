//
// Created by noodles on 25-2-20.
//
#include <iostream>
#include "servo.h"
#include "serial/serial.h"
#include <unistd.h>

int main() {

    try {
        servo::ServoProtocol servoProtocol1(0x01);
        servoProtocol1.eeprom.buildGetSoftwareVersion();

        const std::vector<serial::PortInfo> &listPorts = serial::list_ports();
        for (const auto &item: listPorts) {
            std::cout << "设备名称：" << item.port << ", 描述：" << item.description << ", 硬件 ID：" << item.hardware_id
                      << std::endl;
        }


        Servo servo("/dev/ttyUSB0", 1000000);
        servo.init();

        // 检查串口是否打开
        if (servo.getSerial().isOpen()) {
            std::cout << "✅ 串口已打开：" << servo.getSerial().getPort() << std::endl;

            std::cout << "设备名称：" << servo.getSerial().getPort() << std::endl;
            std::cout << "波特率：" << servo.getSerial().getBaudrate() << std::endl;
            std::cout << "字节大小：" << static_cast<int>(servo.getSerial().getBytesize()) << std::endl;
            std::cout << "停止位：" << static_cast<int>(servo.getSerial().getStopbits()) << std::endl;
            std::cout << "奇偶校验：" << static_cast<int>(servo.getSerial().getParity()) << std::endl;

        } else {
            std::cerr << "❌ 串口打开失败！" << std::endl;
            return -1;
        }

        sleep(2);


        servo::ServoProtocol servoProtocol(0x01);
        auto ref = servo.sendCommand(servoProtocol.eeprom.buildGetSoftwareVersion());
        if (ref) {
            std::cout << "发送成功" << std::endl;
        } else {
            std::cout << "发送失败" << std::endl;
        }

        sleep(2);

        servo.close();


    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }

}