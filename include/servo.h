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

class Servo {
public:
    Servo(const std::string &serial_device, int baudrate, int gpio_chip, int gpio_line);

    ~Servo();

    void init();

    void close();

    bool sendCommand(uint8_t id, uint8_t cmd, const std::vector<uint8_t> &data);

    bool readResponse(uint8_t id, uint8_t &status, std::vector<uint8_t> &payload);

    void setMode(uint8_t id, uint16_t mode);

    void setAngle(uint8_t id, uint16_t angle, uint16_t speed);

    void setSpeed(uint8_t id, int16_t speed);

    int getPosition(uint8_t id);

private:
    serial::Serial serial;
    gpio::GPIO gpio;

    void enableBus();

    void disableBus();
};

#endif //UP_CORE_SERVO_H
