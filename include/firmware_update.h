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

    bool upgrade(std::string port,
                 uint32_t current_baud_rate,
                 std::string bin_path,
                 int total_retry = 10,
                 int handshake_count = 5,
                 int fire_ware_frame_retry = 5,
                 int wave_sign_retry = 5);

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

    std::vector<std::vector<uint8_t>> textureBinArray(const std::string &fileName);

    bool bootloader(uint8_t id);

    bool firmware_upgrade();

    bool firmwareUpdate(std::vector<std::vector<uint8_t>> binArray);

    bool wave();

    bool sendFrame(const std::vector<uint8_t> &frame);

    void buildFrame(const std::vector<uint8_t> &data, int packetNumber, std::vector<uint8_t> &frame);

    uint16_t calculateCRC(const std::vector<uint8_t> &data);

    static void readFile(const std::string &fileName, std::vector<uint8_t> &buffer);

};


#endif //UP_CORE_FIRMWARE_UPDATE_H
