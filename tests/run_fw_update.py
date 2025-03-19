import up_core as up
from up_core import LogLevel

from up_core import FirmwareUpdate

up.set_log_level(LogLevel.DEBUG)

fw_update = FirmwareUpdate()
success = fw_update.upgrade("/dev/ttyUSB0", 1000000, "/home/noodles/CLionProjects/up_core/file/CDS5516_1.0.bin")
print("升级成功" if success else "升级失败")
