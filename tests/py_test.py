import multiprocessing
import os
import threading
import time
import sys
import asyncio
import platform

import logging

print(sys.path)

import up_core as up
from up_core import LogLevel

if platform.system() in ['Linux']:
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
from up_core import Base

from up_core import ServoManager

from up_core import FirmwareUpdate

import sys

print(sys.getdefaultencoding())  # 获取 Python 默认编码
print(sys.stdout.encoding)  # 获取标准输出编码
print(sys.getfilesystemencoding())  # 获取文件系统编码

logging.basicConfig(level=logging.DEBUG)

logging.debug("logging debug 中文测试 -----------")

up.set_log_level(LogLevel.DEBUG)  # 设置日志级别为 DEBUG

# up.setConsoleOutputCP()

# 设置 Python 标准输出流为 UTF-8
# sys.stdout.reconfigure(encoding='utf-8')
# sys.stderr.reconfigure(encoding='utf-8')

up.debug("Logger debug 中文测试 -----------")
up.error("Logger error 中文测试 -----------")
up.info("Logger info 中文测试 -----------")

#
result = up.add(3, 5)
print(result)  # 输出 8

up.system("ls")

servoProtocol = Base(0)
data = servoProtocol.buildResetPacket()
print(issubclass(up.SerialException, Exception))  # True
