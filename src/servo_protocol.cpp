//
// Created by noodles on 25-2-19.
//

#include "servo_protocol.h"

/**
 * @brief 创建一个舵机通信数据帧
 * @param id 舵机 ID
 * @param cmd 命令字节
 * @param data 传输的数据
 * @return 生成的数据帧
 */
std::vector<uint8_t> ServoProtocol::createFrame(uint8_t id, uint8_t cmd, const std::vector<uint8_t> &data) {
    size_t length = data.size() + 2; // 长度 = 数据长度 + cmd + 校验和
    std::vector<uint8_t> frame;
    frame.reserve(6 + data.size());  // 预分配空间，避免多次分配

    uint8_t checksum = 0;
    frame.push_back(0xff);  // 帧头
    frame.push_back(0xff);
    frame.push_back(id);
    checksum += id;

    frame.push_back(length);
    checksum += length;

    frame.push_back(cmd);
    checksum += cmd;

    for (uint8_t byte: data) {
        frame.push_back(byte);
        checksum += byte;
    }

    frame.push_back(~checksum);  // 校验和取反
    return frame;
}

/**
 * @brief 解析返回数据帧
 * @param response 接收到的完整数据帧
 * @param expected_id 期待的舵机 ID
 * @param status 返回的状态码
 * @param payload 解析出的数据
 * @return 是否解析成功
 */
bool ServoProtocol::parseResponse(const std::vector<uint8_t> &response, uint8_t expected_id, uint8_t &status,
                                  std::vector<uint8_t> &payload) {
    if (response.size() < 6) {
        return false; // 长度不足
    }

    if (response[0] != 0xff || response[1] != 0xff) {
        return false; // 帧头错误
    }

    uint8_t id = response[2];
    if (id != expected_id) {
        return false; // ID 不匹配
    }

    uint8_t length = response[3];
    if (response.size() < length + 4) {
        return false; // 长度错误
    }

    status = response[4]; // 状态码
    payload.assign(response.begin() + 5, response.begin() + 4 + length - 2);

    // **优化校验和计算**
    uint8_t checksum = 0;
    for (size_t i = 2; i < 4 + length - 1; i++) {
        checksum += response[i];
    }
    checksum = ~checksum;

    return (checksum == response[4 + length - 1]);
}

