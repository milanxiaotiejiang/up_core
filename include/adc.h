//
// Created by noodles on 25-2-19.
//

#ifndef UP_CORE_ADC_H
#define UP_CORE_ADC_H

#include "spi.h"
#include <vector>
#include <stdexcept>
#include <stdint.h>

namespace adc {
/**
 * ADC 设备异常类
 */
    class ADCException : public std::runtime_error {
    public:
        explicit ADCException(const std::string &message)
                : std::runtime_error("ADCException: " + message) {}
    };

/**
 * ADC 控制类
 */
    class ADC {
    public:
        explicit ADC(spi::SPI &spi, uint8_t channel_count);

        ~ADC() = default;

        void init();                             // 初始化 ADC
        uint16_t readChannel(uint8_t channel);   // 读取单个 ADC 通道
        std::vector<uint16_t> readAll();         // 读取所有 ADC 通道

    private:
        spi::SPI &spi;
        uint8_t channel_count;
    };

}

#endif //UP_CORE_ADC_H
