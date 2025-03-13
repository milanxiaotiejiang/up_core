//
// Created by noodles on 25-3-13.
//

#include <iostream>
#include <fstream>
#include <cstring>
#include "firmware_update.h"

void FirmwareUpdate::sendData(const std::string &fileName, int retryTimes) {
    std::vector<uint8_t> buffer;
    readFile(fileName, buffer);

    int packetNumber = 1;
    size_t dataSize = buffer.size();
    size_t offset = 0;

    while (offset < dataSize) {
        std::vector<uint8_t> frame;
        buildFrame(buffer, packetNumber, frame);

        bool success = false;
        for (int i = 0; i < retryTimes; ++i) {
            sendFrame(frame);
            if (waitForResponse()) {
                success = true;
                break;
            }
        }

        if (!success) {
            std::cerr << "Failed to send frame after " << retryTimes << " retries." << std::endl;
            return;
        }

        offset += 128;
        ++packetNumber;
    }
}

void FirmwareUpdate::readFile(const std::string &fileName, std::vector<uint8_t> &buffer) {
    std::ifstream file(fileName, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file");
    }

    buffer.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

void FirmwareUpdate::buildFrame(const std::vector<uint8_t> &data, int packetNumber, std::vector<uint8_t> &frame) {
    frame.resize(133);
    frame[0] = 0x01;
    frame[1] = packetNumber;
    frame[2] = 255 - packetNumber;

    size_t dataSize = data.size();
    size_t offset = (packetNumber - 1) * 128;
    size_t copySize = std::min(dataSize - offset, static_cast<size_t>(128));
    std::memcpy(&frame[3], &data[offset], copySize);

    uint16_t crc = calculateCRC(std::vector<uint8_t>(frame.begin() + 3, frame.begin() + 3 + copySize));
    frame[131] = crc >> 8;
    frame[132] = crc & 0xFF;
}

uint16_t FirmwareUpdate::calculateCRC(const std::vector<uint8_t> &data) {
    uint16_t crc = 0;
    for (uint8_t byte: data) {
        crc ^= (byte << 8);
        for (int i = 0; i < 8; ++i) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

void FirmwareUpdate::sendFrame(const std::vector<uint8_t> &frame) {
    // Implement the actual sending logic here
    std::cout << "Sending frame: ";
    for (uint8_t byte: frame) {
        std::cout << std::hex << static_cast<int>(byte) << " ";
    }
    std::cout << std::dec << std::endl;
}

bool FirmwareUpdate::waitForResponse() {
    // Implement the actual waiting logic here
    // Simulate a successful response for this example
    return true;
}
