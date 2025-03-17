//
// Created by noodles on 25-3-15.
//

#ifndef UP_CORE_SERVO_PROTOCOL_PARSE_H
#define UP_CORE_SERVO_PROTOCOL_PARSE_H

#include <string>
#include <vector>
#include "servo_protocol.h"
#include <unordered_map>

std::string bytesToHex(const std::vector<uint8_t> &data);

int singleByteToInt(uint8_t byte);

int doubleByteToInt(uint8_t lowByte, uint8_t highByte);

float combineSpeed(uint8_t lowByte, uint8_t highByte);

float combinePosition(uint8_t lowByte, uint8_t highByte);

bool previewSerialData(const std::vector<uint8_t> &packet);

std::pair<bool, std::pair<int, int>> performExtractID(const std::vector<uint8_t> &packet);

namespace servo {
    std::string eePROMValue(EEPROM eeprom, uint8_t value);

    std::unordered_map<EEPROM, uint8_t>
    parseEEPROMData(const std::vector<uint8_t> &data, EEPROM start = servo::EEPROM::MODEL_NUMBER_L);

    std::string ramValue(RAM ram, uint8_t value);

    std::unordered_map<RAM, uint8_t>
    parseRAMData(const std::vector<uint8_t> &data, RAM start = servo::RAM::TORQUE_ENABLE);
}


#endif //UP_CORE_SERVO_PROTOCOL_PARSE_H
