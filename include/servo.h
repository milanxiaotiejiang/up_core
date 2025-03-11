//
// Created by noodles on 25-2-19.
//

#ifndef UP_CORE_SERVO_H
#define UP_CORE_SERVO_H

#include "serial/serial.h"
#include "gpio.h"
#include "servo_protocol.h"
#include <stdint.h>
#include <utility>
#include <vector>
#include <queue>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <unordered_map>

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

//    Servo(const gpio::GPIO &gpio, bool gpio_enabled,
//          const std::string &port = "",
//          uint32_t baudrate = 9600,
//          serial::Timeout timeout = serial::Timeout(),
//          serial::bytesize_t bytesize = serial::eightbits,
//          serial::parity_t parity = serial::parity_none,
//          serial::stopbits_t stopbits = serial::stopbits_one,
//          serial::flowcontrol_t flowcontrol = serial::flowcontrol_none);

    explicit Servo(std::shared_ptr<serial::Serial> serial, std::shared_ptr<gpio::GPIO> gpio = nullptr)
            : serial(std::move(serial)), gpio(std::move(gpio)) {}

    ~Servo();

    /** @brief 初始化 */
    void init();

    /** @brief 关闭 */
    void close();

    /** @brief 发送指令 */
    bool sendCommand(const std::vector<uint8_t> &frame);

    bool sendWaitCommand(const std::vector<uint8_t> &frame, std::vector<uint8_t> &response_data);

    /** @brief 解析串口数据 */
    bool performSerialData(const std::vector<uint8_t> &packet);

    /** @brief 解析串口数据 */
    std::pair<bool, std::pair<int, int>> performID(const std::vector<uint8_t> &packet);

    // 注册回调函数类型
    using DataCallback = std::function<void(const std::vector<uint8_t> &)>;

    // 设置数据接收回调
    void setDataCallback(DataCallback callback) {
        dataCallback = std::move(callback);
    }

private:
    std::shared_ptr<serial::Serial> serial;
    std::shared_ptr<gpio::GPIO> gpio;

    bool gpio_enabled = false;

    // 接收数据的线程
    std::thread receive_thread;
    // 控制接收线程是否运行的标志
    std::atomic<bool> running{false};

    DataCallback dataCallback;

    // 用于存放接收到的数据，按消息 ID 存储
    std::unordered_map<uint32_t, std::vector<uint8_t>> received_data_;
    // 存储每个消息 ID 对应的条件变量
    std::unordered_map<uint32_t, std::unique_ptr<std::condition_variable>> message_conditions_;
    // 用于保护接收到的数据和发送过程的互斥锁
    std::mutex mutex_;
    // 消息 ID 计数器
    uint32_t message_counter;

    // 声明全局锁对象，确保单线程执行
    std::mutex send_mutex;

    // 生成唯一的消息 ID
    uint32_t generateMessageId() {
        return ++message_counter;  // 简单的递增 ID 生成策略
    }

    void processSerialData();

    void enableBus();

    void disableBus();

    void processDataPacket(const std::vector<uint8_t> &packet) {
        if (dataCallback) {
            dataCallback(packet); // 调用回调函数
        }
    }
};

#endif //UP_CORE_SERVO_H
