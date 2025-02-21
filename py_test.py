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

# .def(py::init<const std::string &, uint32_t, uint8_t, uint8_t>(),
#                  py::arg("device"), py::arg("speed"), py::arg("mode"), py::arg("bits_per_word"),
#                  "构造 SPI 对象")
spi = SPI("/dev/spidev0.0", 1000000, 0, 8)
# .def(py::init<spi::SPI &, uint8_t>(), py::arg("spi"), py::arg("channel_count"), "构造 ADC 对象")
adc = ADC(spi, 8)
# .def(py::init<int, int>(), py::arg("chip_id"), py::arg("line_id"), "构造 GPIO 对象")
gpio = GPIO(0, 0)

# .def(py::init<uint8_t>(), py::arg("id"), "构造 ServoRAM 对象")
servo_ram = ServoRAM(1)
#  .def("buildGetTorqueEnabled", &servo::ServoRAM::buildGetTorqueEnabled, "读取扭矩开关状态")
print(servo_ram.buildGetTorqueEnabled())

# .def(py::init<const std::string &, int>(),
#                  py::arg("serial_device"), py::arg("baudrate"), "构造 Servo 对象，初始化串口设备")
serial = Serial("/dev/ttyUSB0", 1000000)
print(serial.isOpen())  # 输出 True

serial.close()

# 示例：列出所有串口
ports = up.list_ports()
for port in ports:
    print(f"Port: {port.port}, Description: {port.description}, Hardware ID: {port.hardware_id}")
