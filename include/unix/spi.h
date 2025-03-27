//
// Created by noodles on 25-2-19.
//

#ifndef UP_CORE_SPI_H
#define UP_CORE_SPI_H

#include <stdexcept>
#include <string>
#include <stdint.h>
#include <linux/spi/spidev.h>

namespace spi {

/**
 * SPI 设备异常类
 */
    class SPIException : public std::runtime_error {
    public:
        explicit SPIException(const std::string &message)
                : std::runtime_error("SPIException: " + message) {}
    };

/**
 * SPI 控制类
 */
    class SPI {
    public:
        SPI(const std::string &device, uint32_t speed, uint8_t mode, uint8_t bits_per_word);

        ~SPI();

        void init(); // 初始化 SPI 设备
        void transfer(uint8_t *tx, uint8_t *rx, size_t length); // SPI 传输数据
        void close(); // 关闭 SPI 设备

    private:
        std::string device;
        uint32_t speed;
        uint8_t mode;
        uint8_t bits_per_word;
        int fd;
    };
}

#endif //UP_CORE_SPI_H
