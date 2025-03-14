import logging
from typing import List, Tuple, Optional
from pup_core.model.up_core import UpErrorCode
from pup_core.model.up_exception import PySerialException


# 定义舵机错误的枚举
class ServoError:
    NO_ERROR = 0
    INSTRUCTION_ERROR = 1 << 6  # 指令错误 (BIT6)
    OVERLOAD = 1 << 5  # 过载 (BIT5)
    CHECKSUM_ERROR = 1 << 4  # 校验和错 (BIT4)
    OUT_OF_RANGE = 1 << 3  # 指令超范围 (BIT3)
    OVERHEAT = 1 << 2  # 过热 (BIT2)
    OUT_OF_ANGLE_RANGE = 1 << 1  # 角度超范围 (BIT1)
    OVER_VOLTAGE_UNDER_VOLTAGE = 1 << 0  # 过压欠压 (BIT0)

    # 自定义
    INVALID_PACKET_LENGTH = 1 << 7  # 数据包长度无效
    INVALID_DATA = 1 << 8  # 无效数据

    ERROR_DESCRIPTIONS = {
        NO_ERROR: "No error",
        INSTRUCTION_ERROR: "Instruction error",
        OVERLOAD: "Overload",
        CHECKSUM_ERROR: "Checksum error",
        OUT_OF_RANGE: "Out of range",
        OVERHEAT: "Overheat",
        OUT_OF_ANGLE_RANGE: "Out of angle range",
        OVER_VOLTAGE_UNDER_VOLTAGE: "Over voltage or under voltage",
        INVALID_PACKET_LENGTH: "Invalid packet length",  # 自定义错误描述
    }

    @staticmethod
    def get_error_description(error_code: int) -> str:
        """
        获取错误码的描述
        :param error_code: 错误码
        :return: 错误描述字符串
        """
        return ServoError.ERROR_DESCRIPTIONS.get(error_code, "Unknown error")


# 错误信息类
class ServoErrorInfo:
    def __init__(self, error: int, description: str):
        self.error = error
        self.description = description


# 将字节列表转换为十六进制字符串
def bytes_to_hex(byte_list: bytearray) -> str:
    return ' '.join([f'{byte:02X}' for byte in byte_list])


# 解析串口数据包
def perform_serial_data(packet: bytearray) -> Tuple[int, Optional[bytearray]]:
    """
    解析串口数据包，验证校验和并提取有效数据。

    :param packet: 串口数据包
    :return: 如果数据包有效并且校验和正确，返回 (True, payload)；
             否则返回 (False, 错误信息)
    """
    # 检查数据包长度是否足够
    if len(packet) < 6:
        logging.debug("❌ 数据包长度不足，丢弃数据")
        return ServoError.INVALID_PACKET_LENGTH, None

    # 计算校验和
    checksum = 0
    for i in range(2, len(packet) - 1):  # 从第三个字节开始到倒数第二个字节
        checksum += packet[i]
    checksum = ~checksum & 0xFF  # 校验和取反（确保是一个字节的值）

    # 校验失败，丢弃数据包
    if checksum != packet[-1]:
        logging.debug("❌ 校验失败，丢弃数据包")
        return ServoError.CHECKSUM_ERROR, None

    # 提取数据包中的各个字段
    packet_id = packet[2]  # ID
    error_code = packet[4]  # 错误码
    payload = packet[5:-1]  # 数据有效负载（去掉起始和结束的校验字段）

    # 获取错误信息
    if error_code != ServoError.NO_ERROR:
        error_description = ServoError.get_error_description(error_code)
        logging.warning(f"⚠️ 舵机 {packet_id} 返回错误: {error_code} ({error_description})")
        return error_code, None  # 返回具体的错误码

    logging.info(f"✅ 接收到数据包: {bytes_to_hex(packet)}")

    # 返回有效负载
    return ServoError.NO_ERROR, payload


def bytearray_to_int_big_endian(byte_list: bytearray) -> int:
    return int.from_bytes(byte_list, byteorder='big')


def bytearray_to_int_little_endian(byte_list: bytearray) -> int:
    return int.from_bytes(byte_list, byteorder='little')


def hex_to_decimal(hex_str: str) -> int:
    return int(hex_str, 16)


def decimal_to_hex(decimal_value: int) -> str:
    return hex(decimal_value)[2:].upper()  # 去掉 '0x' 前缀，并转换为大写


def perform_version(payload: bytearray):
    if len(payload) == 1:  # 假设版本号是一个字节
        version = payload[0]  # 将版本号提取出来，转换为整数
        logging.info(f"📦 获取到版本: {version}")
        return ServoError.NO_ERROR, version  # 返回版本号作为整数
    else:
        logging.warning(f"⚠️ 数据包无效，payload 长度不符合预期: {len(payload)}")
        return ServoError.INVALID_DATA, None  # 返回无效数据
