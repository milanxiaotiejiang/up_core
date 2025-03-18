//
// Created by noodles on 25-3-13.
//

#ifndef UP_CORE_FIRMWARE_UPDATE_H
#define UP_CORE_FIRMWARE_UPDATE_H


#include <string>
#include <vector>

class FirmwareUpdate {
public:
    std::vector<std::vector<uint8_t>> textureBinArray(const std::string &fileName);

private:
    static void readFile(const std::string &fileName, std::vector<uint8_t> &buffer);

    void buildFrame(const std::vector<uint8_t> &data, int packetNumber, std::vector<uint8_t> &frame);

    uint16_t calculateCRC(const std::vector<uint8_t> &data);

};


#endif //UP_CORE_FIRMWARE_UPDATE_H
