//
// Created by noodles on 25-2-19.
//

#ifndef UP_CORE_GPIO_H
#define UP_CORE_GPIO_H

#include <stdexcept>
#include <string>
#include <gpiod.h>

namespace gpio {
/**
 * GPIO 设备异常类
 */
    class GPIOException : public std::runtime_error {
    public:
        explicit GPIOException(const std::string &message)
                : std::runtime_error("GPIOException: " + message) {}
    };

/**
 * GPIO 控制类
 */
    class GPIO {
    public:
        GPIO(int chip_id, int line_id);

        ~GPIO();

        void init();         // 初始化 GPIO
        void setValue(int value); // 设置 GPIO 电平（0 或 1）
        void release();      // 释放 GPIO 资源

    private:
        int chip_id;
        int line_id;
        struct gpiod_chip *chip;
        struct gpiod_line *line;
    };
}

#endif //UP_CORE_GPIO_H
