//
// Created by noodles on 25-3-13.
//

#ifndef UP_CORE_FIRMWARE_UPDATE_H
#define UP_CORE_FIRMWARE_UPDATE_H


#include <string>
#include <vector>
#include "serial/serial.h"
#include <unordered_map>

class FirmwareUpdate {
public:

    /**
     *
     * @param port_input                      保存串口设备路径到成员变量
     * @param baud_rate         保存当前波特率到成员变量，波特率决定了与设备通信的速度，单位为比特/秒
     * @param bin_path                  固件文件路径
     * @param servo_id          舵机ID
     * @param total_retry               固件升级主循环，最多尝试 total_retry 次，每次循环都会执行完整的升级流程，如果任何步骤失败则重试整个流程
     * @param handshake_threshold           保存握手成功计数阈值，当接收到的握手确认次数达到此值时，认为握手成功
     * @param frame_retry_count     保存固件数据帧发送的最大重试次数，每个数据帧发送失败后最多重试这么多次
     * @param sign_retry_count           保存结束标志发送的最大重试次数，结束标志用于通知设备固件传输已完成
     * @return
     */
    bool upgrade_path(const std::string &port_input,
                      int baud_rate,
                      const std::string &bin_path,
                      u_int8_t servo_id,
                      int total_retry = 10,
                      int handshake_threshold = 5,
                      int frame_retry_count = 5,
                      int sign_retry_count = 5);

    bool upgrade_stream(const std::string &port_input,
                        int baud_rate,
                        std::vector<uint8_t> &fileBuffer,
                        u_int8_t servo_id,
                        int total_retry = 10,
                        int handshake_threshold = 5,
                        int frame_retry_count = 5,
                        int sign_retry_count = 5);

private:
    std::string port;
    int current_baud_rate;

    int handshake_count{5};
    int fire_ware_frame_retry{5};
    int wave_sign_retry{5};

    std::shared_ptr<serial::Serial> upgradeSerial;

    std::atomic<int> read_count{0};
    std::atomic<bool> stop_writing{false};
    std::atomic<bool> stop_reading{false};
    std::mutex mtx;
    std::condition_variable cv;
    bool write_finished = false;

    const int write_iteration = 10;
    const int read_iteration = 3;
    const u_int8_t request_sing = 0x64;
    const u_int8_t handshake_sign = 0x43;
    const u_int8_t wave_sign = 0x04;


    std::atomic<bool> stop_receive{false};
    std::unordered_map<uint32_t, std::vector<uint8_t>> received_data_;
    std::unordered_map<uint32_t, std::unique_ptr<std::condition_variable>> message_conditions_;
    std::mutex mutex_;
    uint32_t message_counter;

    // 生成唯一的消息 ID
    uint32_t generateMessageId() {
        return ++message_counter;  // 简单的递增 ID 生成策略
    }

    static std::vector<uint8_t> textureBinArray(const std::string &binPath);

    static std::vector<std::vector<uint8_t>> splitBinArray(std::vector<uint8_t> &fileBuffer, size_t dataSize);

    bool bootloader(uint8_t id);

    bool firmware_upgrade();

    bool firmwareUpdate(std::vector<std::vector<uint8_t>> binArray);

    bool sendFrame(const std::vector<uint8_t> &frame);

    bool wave();

    static void buildFrame(const std::vector<uint8_t> &data, int packetNumber, std::vector<uint8_t> &frame);

    static uint16_t calculateCRC(const std::vector<uint8_t> &data);

    static void readFile(const std::string &fileName, std::vector<uint8_t> &buffer);

};


#endif //UP_CORE_FIRMWARE_UPDATE_H
