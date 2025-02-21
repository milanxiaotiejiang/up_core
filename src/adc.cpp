//
// Created by noodles on 25-2-19.
//

#include "adc.h"
#include <iostream>

namespace adc {
/**
 * @brief ADC 构造函数
 * @param spi  SPI 设备对象
 * @param channel_count ADC 通道数（如 8 通道、16 通道）
 */
    ADC::ADC(spi::SPI &spi, uint8_t channel_count) : spi(spi), channel_count(channel_count) {}

/**
 * @brief 初始化 ADC（可选，某些 ADC 可能不需要初始化）
 */
    void ADC::init() {
        // 某些 ADC 需要特殊的 SPI 初始化指令，这里留空
    }

/**
 * @brief 读取单个 ADC 通道
 * @param channel ADC 通道索引
 * @return 16 位 ADC 数据
 * @throws ADCException 读取失败时抛出异常
 */
    uint16_t ADC::readChannel(uint8_t channel) {
        if (channel >= channel_count) {
            throw ADCException("无效的 ADC 通道索引: " + std::to_string(channel));
        }

        uint8_t tx_buffer[3] = {0x01, (uint8_t) ((0x80 | (channel << 4)) & 0xF0), 0x00};
        uint8_t rx_buffer[3] = {0};

        spi.transfer(tx_buffer, rx_buffer, 3);

        uint16_t result = ((rx_buffer[1] & 0x03) << 8) | rx_buffer[2];  // 提取有效数据
        return result;
    }

/**
 * @brief 读取所有 ADC 通道
 * @return 各通道的 ADC 数据向量
 * @throws ADCException 读取失败时抛出异常
 */
    std::vector<uint16_t> ADC::readAll() {
        std::vector<uint16_t> values(channel_count);

        for (uint8_t i = 0; i < channel_count; ++i) {
            values[i] = readChannel(i);
        }

        return values;
    }

}