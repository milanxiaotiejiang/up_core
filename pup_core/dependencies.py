# dependencies.py
import asyncio
import time

import pup_core.serial_manager as sm
import logging
from up_core import ServoProtocol, ServoError

from pup_core.servo_parser import perform_serial_data
from pup_core.utils.resolve import identify_mode

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
                data = servoProtocol.eeprom.buildGetSoftwareVersion()
                try:
                    byte_buffer = await serial_manager.write_wait(serial_id, data)
                    if byte_buffer:
                        error_code, payload = perform_serial_data(byte_buffer)
                        if error_code == ServoError.NO_ERROR:
                            hex_string = byte_buffer.hex().upper()
                            formatted_hex_string = ' '.join([hex_string[i:i + 2] for i in range(0, len(hex_string), 2)])
                            logging.info(str(i) + " write_wait success: " + formatted_hex_string)
                        else:
                            logging.warning(str(i) + " write_wait failed: error_code != NO_ERROR")
                    else:
                        logging.warning(str(i) + " write_wait failed 2")
                except Exception as e:
                    logging.error(str(i) + " write_wait failed 3")
                await asyncio.sleep(0.01)

        async def write():
            for i in range(100):
                data = servoProtocol.eeprom.buildGetID()
                try:
                    success = await serial_manager.write(serial_id, data)
                    if success:
                        logging.info(str(i) + " write success")
                    else:
                        logging.warning(str(i) + " write failed")
                except Exception as e:
                    logging.error(str(i) + " write failed 2")
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
    logging.basicConfig(level=logging.INFO)
    try:
        asyncio.run(concurrencyTesting())
    except KeyboardInterrupt:
        logging.info("程序手动终止")
    except Exception as e:
        logging.error(f"Error: {e}")
