//
// Created by noodles on 25-2-19.
//

#include "gpio.h"
#include <stdio.h>

namespace gpio {
/**
 * @brief GPIO 构造函数
 * @param chip_id  GPIO 控制器编号（如 `/dev/gpiochip0` 对应 chip_id=0）
 * @param line_id  GPIO 具体的引脚编号
 */
    GPIO::GPIO(int chip_id, int line_id)
            : chip_id(chip_id), line_id(line_id), chip(nullptr), line(nullptr) {}

/**
 * @brief GPIO 析构函数（确保资源释放）
 */
    GPIO::~GPIO() {
        release();
    }

/**
 * @brief 初始化 GPIO（打开 GPIO 控制器并设置为输出模式）
 * @throws GPIOException 初始化失败时抛出异常
 */
    void GPIO::init() {
        chip = gpiod_chip_open_by_number(chip_id);
        if (!chip) {
            throw GPIOException("无法打开 /dev/gpiochip" + std::to_string(chip_id));
        }

        line = gpiod_chip_get_line(chip, line_id);
        if (!line) {
            gpiod_chip_close(chip);
            throw GPIOException("无法获取 GPIO" + std::to_string(line_id));
        }

        if (gpiod_line_request_output(line, "gpio_control", 0) < 0) {
            gpiod_chip_close(chip);
            throw GPIOException("无法设置 GPIO" + std::to_string(line_id) + " 为输出模式");
        }
    }

/**
 * @brief 设置 GPIO 的电平（0 或 1）
 * @param value 0 表示低电平，1 表示高电平
 * @throws GPIOException 设置失败时抛出异常
 */
    void GPIO::setValue(int value) {
        if (!line) {
            throw GPIOException("GPIO 未初始化或已释放");
        }
        if (gpiod_line_set_value(line, value) < 0) {
            throw GPIOException("无法设置 GPIO" + std::to_string(line_id) + " 的值");
        }
    }

/**
 * @brief 释放 GPIO 资源
 */
    void GPIO::release() {
        if (line) {
            gpiod_line_release(line);
            line = nullptr;
        }
        if (chip) {
            gpiod_chip_close(chip);
            chip = nullptr;
        }
    }

}