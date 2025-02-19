//
// Created by noodles on 25-2-19.
//

#include "spi.h"

#include "spi.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>

namespace spi {

/**
 * @brief SPI 构造函数
 * @param device  SPI 设备路径（如 "/dev/spidev0.0"）
 * @param speed   SPI 传输速率（Hz）
 * @param mode    SPI 模式（0~3）
 * @param bits_per_word 数据位宽（通常 8）
 */
    SPI::SPI(const std::string &device, uint32_t speed, uint8_t mode, uint8_t bits_per_word)
            : device(device), speed(speed), mode(mode), bits_per_word(bits_per_word), fd(-1) {}

/**
 * @brief SPI 析构函数（确保资源释放）
 */
    SPI::~SPI() {
        close();
    }

/**
 * @brief 初始化 SPI 设备
 * @throws SPIException 初始化失败时抛出异常
 */
    void SPI::init() {
        fd = open(device.c_str(), O_RDWR);
        if (fd < 0) {
            throw SPIException("无法打开 SPI 设备: " + device);
        }

        // 设置 SPI 模式
        if (ioctl(fd, SPI_IOC_WR_MODE32, &mode) < 0) {
            uint8_t mode8 = static_cast<uint8_t>(mode);
            if (ioctl(fd, SPI_IOC_WR_MODE, &mode8) < 0) {
                throw SPIException("无法设置 SPI 模式: " + std::string(strerror(errno)));
            }
        }

        // 设置数据位宽
        if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits_per_word) < 0) {
            throw SPIException("无法设置 SPI 位宽");
        }

        // 设置 SPI 速度
        if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
            throw SPIException("无法设置 SPI 速度");
        }
    }

/**
 * @brief 通过 SPI 传输数据
 * @param tx  发送缓冲区
 * @param rx  接收缓冲区
 * @param length 传输数据长度
 * @throws SPIException 传输失败时抛出异常
 */
    void SPI::transfer(uint8_t *tx, uint8_t *rx, size_t length) {
        if (fd < 0) {
            throw SPIException("SPI 设备未初始化");
        }

        struct spi_ioc_transfer tr = {};
        memset(&tr, 0, sizeof(tr));

        tr.tx_buf = (unsigned long) tx;
        tr.rx_buf = (unsigned long) rx;
        tr.len = length;
        tr.speed_hz = speed;
        tr.bits_per_word = bits_per_word;

        if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) < 0) {
            throw SPIException("SPI 传输失败");
        }
    }

/**
 * @brief 关闭 SPI 设备
 */
    void SPI::close() {
        if (fd >= 0) {
            ::close(fd);
            fd = -1;
        }
    }

}