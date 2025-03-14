import asyncio
import uuid
from typing import Dict, Optional
import logging

import up_core as serial
import pup_core.c_serial as cs
from pup_core.model.up_core import UpErrorCode
from pup_core.model.up_exception import SerialException
from pup_core.signals import error_signal

from dataclasses import dataclass


def list_serial_ports():
    """列出可用的串口"""
    return cs.list_serial_ports()


@dataclass
class CommandResult:
    success: bool
    response: Optional[bytes] = None


class SerialPort:
    def __init__(self, ser: serial.Serial):
        self.ser = ser
        self.received_data_queue = asyncio.Queue()
        self.pending_requests: Dict[str, asyncio.Future] = {}
        self.response_timeout = 3.0
        self.lock = asyncio.Lock()
        self._running = True  # 控制数据接收循环

    async def enqueue_data(self, data: bytes):
        """将接收到的数据放入队列中"""
        await self.received_data_queue.put(data)

    async def send_command(self, data: bytes, wait_for_response: bool = True) -> CommandResult:
        """发送指令，返回响应"""
        async with self.lock:  # 确保同一时间只有一个线程能够发送数据
            if not cs.write(self.ser, data):
                raise SerialException(UpErrorCode.WRITE_DATA_FAILED, "Failed to write data to serial port.")

            if not wait_for_response:
                return CommandResult(success=True, response=None)

            future = asyncio.Future()
            request_id = str(uuid.uuid4())
            self.pending_requests[request_id] = future  # 使用 UUID 作为请求特定的 key

            try:
                response = await asyncio.wait_for(future, timeout=self.response_timeout)
                return CommandResult(success=True, response=response)
            except asyncio.TimeoutError:
                logging.warning("No response received within timeout.")
                return CommandResult(success=False, response=None)
            finally:
                # 移除过期的请求
                await self.pending_requests.pop(request_id, None)

    async def respond(self, data: bytes):
        """处理收到的响应数据"""
        future = next(iter(self.pending_requests.values()), None)
        if future and not future.done():
            future.set_result(data)  # 设置响应结果

    async def receive_data(self):
        """持续接收串口数据。"""
        try:
            while self._running and self.ser.isOpen():
                available_bytes = self.ser.available()  # 检查可读取的数据字节数
                if available_bytes > 0:
                    buffer, bytes_read = self.ser.read(available_bytes)  # 读取数据
                    byte_buffer = bytearray(buffer)

                    await self.enqueue_data(byte_buffer)  # 放入接收队列
                    await self.respond(byte_buffer)  # 处理响应
                else:
                    await asyncio.sleep(0.1)  # 无数据时稍作等待
        except serial.SerialException as e:
            logging.exception("读取串口数据时出错: %s", e)
            await error_signal.send_async("serial_error",
                                          serial_id=str(uuid.uuid4()),  # 这里可以传入串口 ID
                                          message=f"Error reading serial data: {str(e)}")

    def close(self):
        """关闭串口，停止接收数据"""
        self._running = False  # 停止接收循环
        if cs.is_open(self.ser):
            cs.close_serial(self.ser)  # 确保关闭串口
            logging.info("串口已关闭.")


class SerialManager:
    def __init__(self):
        self.serial_ports: Dict[str, SerialPort] = {}  # 存储打开的串口对象，键为串口 ID

    def open(self, device: str, baudrate: int = 9600, parity: str = 'none', databits: int = 8,
             stopbits: int = 1, flowcontrol: str = 'none', timeout: float = 1.0) -> str:
        for serial_id, serial_port in self.serial_ports.items():
            if serial_port.ser.getPort() == device:
                if serial_port.ser.isOpen():
                    logging.info(f"串口 {device} 已经打开")
                    return serial_id

        serial_id = str(uuid.uuid4())
        ser = cs.open_serial(device, baudrate, parity, databits, stopbits, flowcontrol, timeout)

        serial_port = SerialPort(ser)
        self.serial_ports[serial_id] = serial_port

        asyncio.create_task(serial_port.receive_data())

        return serial_id

    def close(self, serial_id: str):
        if serial_id in self.serial_ports:
            serial_port = self.serial_ports.pop(serial_id)
            try:
                serial_port.close()  # 先关闭串口，停止接收循环
            except serial.SerialException as e:
                logging.error(f"关闭串口 {serial_id} 出错: {e}")

            logging.info(f"串口 {serial_id} 关闭成功")
        else:
            raise SerialException(UpErrorCode.SERIAL_NOT_FOUND, f"Serial port {serial_id} not found.")

    async def write(self, serial_id: str, data: bytes) -> bool:
        serial_port = self.serial_ports.get(serial_id)
        if not serial_port:
            raise SerialException(UpErrorCode.SERIAL_NOT_FOUND, f"Serial port {serial_id} not found.")
        result = await serial_port.send_command(data, wait_for_response=False)
        return result.success

    async def write_wait(self, serial_id: str, data: bytes) -> Optional[bytes]:
        serial_port = self.serial_ports.get(serial_id)
        if not serial_port:
            raise SerialException(UpErrorCode.SERIAL_NOT_FOUND, f"Serial port {serial_id} not found.")

        result = await serial_port.send_command(data)
        if result.success:
            return bytearray(result.response)
        return None
