# dependencies.py
import asyncio

import serial.serial_manager as sm
import logging

import up_core as up
from up_core import ServoProtocol, ServoError
from up_core import LogLevel
from up_core import EEPROM
from up_core import RAM

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
        async def write_wait():
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
                            up.parseEEPROMData(data)
                        else:
                            logging.warning(f"请求 EEPROM 数据错误: {error_code}")
                    else:
                        logging.warning("请求 EEPROM 数据错误")
                except Exception as e:
                    logging.error("请求 EEPROM 数据失败 " + str(e))
                await asyncio.sleep(0.01)

        async def write():
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
                            up.parseRAMData(data)
                        else:
                            logging.warning(f"请求 RAM 数据错误: {error_code}")
                except Exception as e:
                    logging.error("请求 RAM 数据失败失败 " + str(e))
                await asyncio.sleep(0.01)

        # todo 2
        # 使用 asyncio.gather 并发执行 `write_wait` 和 `write` 协程
        await asyncio.gather(write_wait(), write())

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
