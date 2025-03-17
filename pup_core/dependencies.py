# dependencies.py
import asyncio

import serial.serial_manager as sm
import logging

import up_core as up
from up_core import ServoProtocol, ServoError
from up_core import LogLevel
from up_core import EEPROM
from up_core import RAM

from pup_core.model.EEPROMData import EEPROMData
from pup_core.model.RAMData import RAMData
from serial.servo_parser import preview_data

# 创建 serial_manager 实例
serial_manager = sm.SerialManager()


# 路由依赖
def get_serial_manager():
    return serial_manager


async def concurrencyTesting():
    serial_id = serial_manager.open("/dev/ttyUSB0", 1000000, "none", 8, 1, "none", 1.0)

    try:
        # todo 0
        servoProtocol = ServoProtocol(0x01)
        data = servoProtocol.eeprom.buildGetSoftwareVersion()

        byte_buffer = await serial_manager.write_wait(serial_id, data)

        if byte_buffer:
            hex_string = byte_buffer.hex().upper()  # 转换为大写
            formatted_hex_string = ' '.join([hex_string[i:i + 2] for i in range(0, len(hex_string), 2)])

            # 打印格式化后的十六进制字符串
            logging.info(f"\n收到数据: {formatted_hex_string}")

        # todo 1
        # 并发发送1000次，间隔0.1秒，每次交替使用 `write_wait` 和 `write` 发送指令
        async def write_eeprom():
            for i in range(100):
                cmd = servoProtocol.eeprom.buildGetEepromData(
                    EEPROM.MODEL_NUMBER_L,
                    int(EEPROM.EEPROM_COUNT) - int(EEPROM.MODEL_NUMBER_L) - 1
                )
                try:
                    byte_array = await serial_manager.write_wait(serial_id, cmd)
                    if byte_array:
                        error_code, payload = preview_data(byte_array)
                        if error_code == ServoError.NO_ERROR:
                            logging.info("请求 EEPROM 数据成功")
                            result = up.parseEEPROMData(payload)

                            # MODEL_NUMBER_L = 0x00,          // 0（0x00） 低位型号号码 读 2（0x02）
                            # MODEL_NUMBER_H = 0x01,          // 1（0x01） 高位型号号码 读 1（0x01）
                            # VERSION = 0x02,                 // 2（0x02） 软件版本 读 1（0x01）
                            # ID = 0x03,                      // 3（0x03） ID 读/写 1（0x01）
                            # BAUDRATE = 0x04,                // 4（0x04） 波特率 读/写 1（0x01）
                            # RETURN_DELAY_TIME = 0x05,       // 5（0x05） 返回延迟时间 读/写 0（0x00）
                            # CW_ANGLE_LIMIT_L = 0x06,        // 6（0x06） 顺时针角度限制（L） 读/写 0（0x00）
                            # CW_ANGLE_LIMIT_H = 0x07,        // 7（0x07） 顺时针角度限制（H） 读/写 0（0x00）
                            # CCW_ANGLE_LIMIT_L = 0x08,       // 8（0x08） 逆时针角度限制（L） 读/写 255（0xFF）
                            # CCW_ANGLE_LIMIT_H = 0x09,       // 9（0x09） 逆时针角度限制（H） 读/写 3（0x03）
                            # MAX_TEMPERATURE = 0x0B,         // 11（0x0B） 最高温度上限 读/写 80（0x50）
                            # MIN_VOLTAGE = 0x0C,             // 12（0x0C） 最低输入电压 读/写 ?
                            # MAX_VOLTAGE = 0x0D,             // 13（0x0D） 最高输入电压 读/写 ?
                            # MAX_TORQUE_L = 0x0E,            // 14（0x0E） 最大扭矩（L） 读/写 255（0xFF）
                            # MAX_TORQUE_H = 0x0F,            // 15（0x0F） 最大扭矩（H） 读/写 3（0x03）
                            # STATUS_RETURN_LEVEL = 0x10,     // 16（0x10） 应答状态级别 读/写 2（0x02）
                            # ALARM_LED = 0x11,               // 17（0x11） LED闪烁 读/写 37（0x25）
                            # ALARM_SHUTDOWN = 0x12,          // 18（0x12） 卸载条件 读/写 4（0x04）

                            eeprom_data = EEPROMData(result)
                            eeprom_json = eeprom_data.to_json()
                            logging.info(f"EEPROM data: {eeprom_json}")
                        else:
                            logging.warning(f"请求 EEPROM 数据错误: {error_code}")
                    else:
                        logging.warning("请求 EEPROM 数据错误")
                except Exception as e:
                    logging.error("请求 EEPROM 数据失败 " + str(e))
                await asyncio.sleep(0.01)

        async def write_ram():
            for i in range(100):
                cmd = servoProtocol.ram.buildGetRamData(
                    RAM.TORQUE_ENABLE,
                    int(RAM.RAM_COUNT) - int(RAM.TORQUE_ENABLE) - 1
                )
                try:
                    byte_array = await serial_manager.write_wait(serial_id, cmd)
                    if byte_array:
                        error_code, payload = preview_data(byte_array)
                        if error_code == ServoError.NO_ERROR:
                            logging.info("请求 RAM 数据成功")
                            result = up.parseRAMData(payload)

                            # torque_enable = 0x18,           // 24（0x18） 扭矩开关 读/写 0（0x00）
                            # led = 0x19,                     // 25（0x19） LED开关 读/写 0（0x00）
                            # cw_compliance_margin = 0x1A,    // 26（0x1A） 顺时针不灵敏区 读/写 2（0x02）
                            # ccw_compliance_margin = 0x1B,   // 27（0x1B） 逆时针不灵敏区 读/写 2（0x02）
                            # cw_compliance_slope = 0x1C,     // 28（0x1C） 顺时针比例系数 读/写 32（0x20）
                            # ccw_compliance_slope = 0x1D,    // 29（0x1D） 逆时针比例系数 读/写 32（0x20）
                            # goal_position_l = 0x1E,         // 30（0x1E） 目标位置（L） 读/写 [Addr36]value
                            # goal_position_h = 0x1F,         // 31（0x1F） 目标位置（H） 读/写 [Addr37]value
                            # moving_speed_l = 0x20,          // 32（0x20） 运行速度（L） 读/写 0
                            # moving_speed_h = 0x21,          // 33（0x21） 运行速度（H） 读/写 0
                            # acceleration = 0x22,            // 34（0x22） 加速度 读/写 32
                            # deceleration = 0x23,            // 35（0x23） 减速度 读/写 32
                            # present_position_l = 0x24,      // 36（0x24） 当前位置（L） 读 ？
                            # present_position_h = 0x25,      // 37（0x25） 当前位置（H） 读 ？
                            # present_speed_l = 0x26,         // 38（0x26） 当前速度（L） 读 ？
                            # present_speed_h = 0x27,         // 39（0x27） 当前速度（H） 读 ？
                            # present_load_l = 0x28,          // 40（0x28） 当前负载 读 ？
                            # present_load_h = 0x29,          // 41（0x29） 当前负载 读 ？
                            # present_voltage = 0x2A,         // 42（0x2A） 当前电压 读 ？
                            # temperature = 0x2B,             // 43（0x2B） 当前温度 读 ？
                            # reg_write = 0x2C,               // 44（0x2C） REG WRITE标志 读 0（0x00）
                            # moving = 0x2E,                  // 46（0x2E） 运行中标志 读 0（0x00）
                            # lock = 0x2F,                    // 47（0x2F） 锁标志 读/写 0（0x00）
                            # min_pwm_l = 0x30,               // 48（0x30） 最小PWM(L) 读/写 90（0x5A）
                            # min_pwm_h = 0x31,               // 49（0x31） 最小PWM(H) 读/写 00（0x00）

                            ram_data = RAMData(result)
                            ram_json = ram_data.to_json()
                            logging.info(f"RAM data: {ram_json}")
                        else:
                            logging.warning(f"请求 RAM 数据错误: {error_code}")
                except Exception as e:
                    logging.error("请求 RAM 数据失败失败 " + str(e))
                await asyncio.sleep(0.01)

        # todo 2
        # 使用 asyncio.gather 并发执行 `write_wait` 和 `write` 协程
        await asyncio.gather(write_eeprom(), write_ram())

    except KeyboardInterrupt:
        logging.info("程序被手动中断")

    finally:
        logging.info("清理并关闭串口连接")
        serial_manager.close(serial_id)


if __name__ == "__main__":
    up.set_log_level(LogLevel.INFO)
    logging.basicConfig(level=logging.INFO)
    try:
        asyncio.run(concurrencyTesting())
    except KeyboardInterrupt:
        logging.info("程序手动终止")
    except Exception as e:
        logging.error(f"Error: {e}")
