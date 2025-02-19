//
// Created by noodles on 25-2-19.
//

#ifndef UP_CORE_SERVO_PROTOCOL_H
#define UP_CORE_SERVO_PROTOCOL_H

#include <vector>
#include <stdint.h>

/**
 * 舵机协议处理类
 */
class ServoProtocol {
public:
    static std::vector<uint8_t> createFrame(uint8_t id, uint8_t cmd, const std::vector<uint8_t> &data);

    static bool parseResponse(const std::vector<uint8_t> &response, uint8_t expected_id, uint8_t &status,
                              std::vector<uint8_t> &payload);
};

#endif //UP_CORE_SERVO_PROTOCOL_H
