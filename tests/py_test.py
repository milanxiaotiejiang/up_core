import multiprocessing
import threading
import time
import sys
import asyncio

import logging

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

from up_core import ServoManager

logging.basicConfig(level=logging.DEBUG)

up.set_log_level(LogLevel.INFO)  # 设置日志级别为 DEBUG
#
result = up.add(3, 5)
print(result)  # 输出 8

up.system("ls")