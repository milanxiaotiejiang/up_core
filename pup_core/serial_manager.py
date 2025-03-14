import asyncio
import threading
import uuid
from typing import Dict, Optional, NamedTuple
import logging

import up_core as serial
import pup_core.c_serial as cs

from pup_core.model.up_core import UpErrorCode
from pup_core.model.up_exception import PySerialException
from pup_core.signals import error_signal


class CommandResult(NamedTuple):
    success: bool
    response: Optional[bytearray] = None


def list_serial_ports():
    return cs.list_serial_ports()


class SerialPort:
    def __init__(self, ser: serial.Serial):
        self.ser = ser
        self.received_data_queue = asyncio.Queue()  # 接收数据的异步队列
        self.pending_response: Optional[asyncio.Future] = None  # 等待的异步响应
        self.response_timeout = 3.0  # 响应超时时间（秒）
        self.lock = asyncio.Lock()  # 串口操作锁，确保并发安全
        self._running = True  # 控制数据接收循环
        self.task: Optional[asyncio.Task] = None  # 数据接收任务

    async def enqueue_data(self, data: bytearray):
        """将接收到的数据加入队列"""
        await self.received_data_queue.put(data)

    async def send_command(self, data: bytes, wait_for_response: bool = True) -> CommandResult:
        """发送指令到串口，支持等待响应"""
        async with self.lock:  # 确保串口写入和读取的原子性
            if not cs.write(self.ser, data):
                raise PySerialException(UpErrorCode.WRITE_DATA_FAILED, "串口写入数据失败。")

            if not wait_for_response:
                return CommandResult(success=True, response=None)

            self.pending_response = asyncio.Future()

            try:
                # 等待响应，设置超时时间
                response = await asyncio.wait_for(self.pending_response, timeout=self.response_timeout)
                return CommandResult(success=True, response=response)
            except asyncio.TimeoutError:
                logging.warning("串口响应超时。")
                return CommandResult(success=False, response=None)
            finally:
                self.pending_response = None

    async def respond(self, data: bytearray):
        """将接收到的数据设置为响应"""
        if self.pending_response and not self.pending_response.done():
            self.pending_response.set_result(data)

    async def receive_data(self, serial_id: str):
        """持续读取串口数据"""
        try:
            while self._running:
                if not self.ser.isOpen():
                    break

                available_bytes = self.ser.available()
                if available_bytes > 0:
                    buffer, bytes_read = self.ser.read(available_bytes)
                    byte_buffer = bytearray(buffer)

                    await self.enqueue_data(byte_buffer)
                    await self.respond(byte_buffer)  # 立即响应等待的数据
                else:
                    await asyncio.sleep(0.01)  # 避免空轮询消耗 CPU
        except Exception as e:
            logging.exception("读取串口数据时出错: %s", e)
            await error_signal.send_async("serial_error", serial_id=serial_id, message=f"读取串口数据时出错: {str(e)}")
            raise

    def close(self):
        """关闭串口并清理资源"""
        self._running = False
        if self.pending_response and not self.pending_response.done():
            self.pending_response.set_result(None)

        if self.task:  # 取消数据接收任务
            try:
                self.task.cancel()
            except asyncio.CancelledError:
                pass

        if cs.is_open(self.ser):
            try:
                cs.close_serial(self.ser)
            except Exception as e:
                logging.error(f"关闭串口时出错: {e}")


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
