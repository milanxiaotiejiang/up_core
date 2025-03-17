import asyncio
import logging
import threading
import uuid
from typing import Dict, Optional

from . import c_serial as cs
from pup_core.model.up_core import UpErrorCode
from pup_core.model.up_exception import PySerialException
from pup_core.serial.serial_port import SerialPort


def list_serial_ports():
    return cs.list_serial_ports()


class SerialManager:
    def __init__(self):
        self.serial_ports: Dict[str, SerialPort] = {}  # 存储打开的串口对象，键为串口 ID
        self._lock = threading.Lock()  # 多线程操作串口的锁

    def open(self, device: str, baudrate: int = 9600, parity: str = 'none', databits: int = 8,
             stopbits: int = 1, flowcontrol: str = 'none', timeout: float = 1.0) -> str:
        """打开串口，返回串口 ID"""
        with self._lock:
            # 检查串口是否已打开
            for serial_id, serial_port in self.serial_ports.items():
                if serial_port.ser.getPort() == device:
                    if serial_port.ser.isOpen():
                        logging.info(f"串口 {device} 已经打开")
                        return serial_id

            # 打开新的串口
            serial_id = str(uuid.uuid4())
            ser = cs.open_serial(device, baudrate, parity, databits, stopbits, flowcontrol, timeout)

            serial_port = SerialPort(ser)
            self.serial_ports[serial_id] = serial_port

        # 启动数据接收任务
        serial_port.task = asyncio.create_task(serial_port.receive_data(serial_id))

        return serial_id

    def close(self, serial_id: str):
        """关闭指定的串口"""
        with self._lock:
            if serial_id in self.serial_ports:
                serial_port = self.serial_ports.pop(serial_id)
                try:
                    serial_port.close()
                except Exception as e:
                    logging.error(f"关闭串口 {serial_id} 出错: {e}")

                logging.info(f"串口 {serial_id} 关闭成功")
            else:
                raise PySerialException(UpErrorCode.SERIAL_NOT_FOUND, f"未找到串口 {serial_id}。")

    def _get_serial_port(self, serial_id: str) -> SerialPort:
        """根据串口 ID 获取 SerialPort 对象"""
        serial_port = self.serial_ports.get(serial_id)
        if not serial_port:
            raise PySerialException(UpErrorCode.SERIAL_NOT_FOUND, f"未找到串口 {serial_id}。")
        return serial_port

    def is_open(self, serial_id: str) -> bool:
        """检查串口是否打开"""
        serial_port = self._get_serial_port(serial_id)
        return serial_port.is_open()

    async def write(self, serial_id: str, data: bytes) -> bool:
        """向串口写入数据，不等待响应"""
        serial_port = self._get_serial_port(serial_id)
        result = await serial_port.send_command(data, wait_for_response=False)
        return result.success

    async def write_wait(self, serial_id: str, data: bytes) -> Optional[bytearray]:
        """向串口写入数据，并等待响应"""
        serial_port = self._get_serial_port(serial_id)
        result = await serial_port.send_command(data)
        if result.success:
            return result.response
        return None
