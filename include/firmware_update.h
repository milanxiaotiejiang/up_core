//
// Created by noodles on 25-3-13.
//

#ifndef UP_CORE_FIRMWARE_UPDATE_H
#define UP_CORE_FIRMWARE_UPDATE_H


#include <string>
#include <vector>

class FirmwareUpdate {
public:
    void sendData(const std::string &fileName, int retryTimes);

private:
    void readFile(const std::string &fileName, std::vector<uint8_t> &buffer);

    void buildFrame(const std::vector<uint8_t> &data, int packetNumber, std::vector<uint8_t> &frame);

    uint16_t calculateCRC(const std::vector<uint8_t> &data);

    void sendFrame(const std::vector<uint8_t> &frame);

    bool waitForResponse();
};


#endif //UP_CORE_FIRMWARE_UPDATE_H
