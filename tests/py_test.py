import time
import sys

print(sys.path)

import up_core as up
from up_core import LogLevel
from up_core import GPIO
from up_core import ADC
from up_core import SPI
from up_core import AlarmShutdownConfig
from up_core import AlarmLEDConfig
from up_core import StatusReturnLevel
from up_core import ORDER
from up_core import EEPROM
from up_core import RAM
from up_core import ServoError

from up_core import ServoEEPROM
from up_core import ServoRAM

from up_core import bytesize_t
from up_core import parity_t
from up_core import stopbits_t
from up_core import flowcontrol_t

from up_core import Serial
from up_core import Servo
from up_core import ServoProtocol

up.set_log_level(LogLevel.DEBUG)  # 设置日志级别为 DEBUG
#
result = up.add(3, 5)
print(result)  # 输出 8

up.system("ls")

current_level = up.get_log_level()
print(current_level)  # 例如，返回 1（INFO）

up.debug("这是 DEBUG 级别的日志")
up.info("这是 INFO 级别的日志")
up.warning("这是 WARNING 级别的日志")
up.error("这是 ERROR 级别的日志")

spi = SPI("/dev/spidev0.0", 1000000, 0, 8)
adc = ADC(spi, 8)
gpio = GPIO(0, 0)

servo_ram = ServoRAM(1)
print(servo_ram.buildGetTorqueEnabled())

# 示例：列出所有串口
ports = up.list_ports()
for port in ports:
    print(f"Port: {port.port}, Description: {port.description}, Hardware ID: {port.hardware_id}")

serial = Serial("/dev/ttyUSB0", 1000000)

servoProtocol = ServoProtocol(0x01)


def data_callback(data):
    print(f"Received data: {data}")


servo = Servo(serial, None)
servo.init()
# 注册回调
servo.set_data_callback(data_callback)
# .def("send_command", &Servo::sendCommand, py::arg("frame"), "Send command to the servo")
servo.send_command(servoProtocol.eeprom.buildGetSoftwareVersion())

time.sleep(5)
servo.close()
