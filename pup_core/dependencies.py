# dependencies.py

import pup_core.serial_manager as sm
import logging
from up_core import ServoProtocol

# 创建 serial_manager 实例
serial_manager = sm.SerialManager()


# 路由依赖
def get_serial_manager():
    return serial_manager

# servoProtocol = ServoProtocol(0x01)
# data = servoProtocol.eeprom.buildGetSoftwareVersion()
#
# serial_id = serial_manager.open_serial("/dev/ttyUSB0", 1000000)
# byte_buffer = serial_manager.write(serial_id, data)
#
# hex_string = byte_buffer.hex().upper()  # 转换为大写
# formatted_hex_string = ' '.join([hex_string[i:i + 2] for i in range(0, len(hex_string), 2)])
#
# # 打印格式化后的十六进制字符串
# logging.info(f"\n收到数据: {formatted_hex_string}")
#
# serial_manager.close_serial(serial_id)
