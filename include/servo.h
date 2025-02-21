//
// Created by noodles on 25-2-19.
//

#ifndef UP_CORE_SERVO_H
#define UP_CORE_SERVO_H

#include "serial/serial.h"
#include "gpio.h"
#include "servo_protocol.h"
#include <stdint.h>
#include <vector>
#include <queue>
#include <thread>
#include <atomic>
#include <condition_variable>

/**
 * 1. 指令包格式
 *  一个完整的指令包格式：
 *  [0xFF] [0xFF] [ID] [Length] [Instruction] [Param1] ... [ParamN] [CheckSum]
 *  其中：
 *      0xFF 0xFF → 固定字头，表示数据包开始
 *      ID → 舵机的 ID（0x00 ~ 0xFD），0xFE（广播 ID）不会返回应答
 *      Length → 数据长度 N + 2
 *      Instruction → 指令码，如 WRITE, READ
 *      Parameters → 额外参数，如目标角度、速度等
 *      CheckSum → 取反校验和：
 *          CheckSum = ~ (ID + Length + Instruction + Param1 + ... ParamN)
 *
 * 2. 应答包格式
 *  一个完整的应答包格式：
 *  [0xFF] [0xFF] [ID] [Length] [ERROR] [Param1] ... [ParamN] [CheckSum]
 *  其中：
 *      ERROR 字节表示舵机当前状态，按位解析：
 *          BIT6 - 指令错误
 *          BIT5 - 过载
 *          BIT4 - 校验和错误
 *          BIT3 - 指令超范围
 *          BIT2 - 过热
 *          BIT1 - 角度超范围
 *          BIT0 - 过压欠压
 *       如果 ERROR = 0，说明舵机状态正常。
 */

class Servo {
public:
    /**
        * @brief 构造 Servo 对象，初始化串口设备
        * @param serial_device 串口设备路径 (如 "/dev/ttyUSB0")
        * @param baudrate 波特率
        * @param id 舵机 ID
        */
    Servo(const std::string &serial_device, int baudrate);

    /**
     * @brief 构造 Servo 对象，带 GPIO 控制
     * @param serial_device 串口设备路径
     * @param baudrate 波特率
     * @param id 舵机 ID
     * @param gpio_chip GPIO 芯片编号
     * @param gpio_line GPIO 线编号
     */
    Servo(const std::string &serial_device, int baudrate, int gpio_chip, int gpio_line);


    ~Servo();

    /** @brief 初始化 */
    void init();

    /** @brief 关闭 */
    void close();

    /** @brief 发送指令 */
    bool sendCommand(const std::vector<uint8_t> &frame);

    const serial::Serial &getSerial() const;

private:
    serial::Serial serial;
    gpio::GPIO gpio;

    bool gpio_enabled = false;

    std::thread serialThread;
    std::atomic<bool> running{false};  // 控制线程运行状态

    void processSerialData();

    void enableBus();

    void disableBus();
};

#endif //UP_CORE_SERVO_H
