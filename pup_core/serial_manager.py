import asyncio
import threading
import uuid
from typing import Dict, Optional, NamedTuple
import logging

import up_core as serial
import pup_core.c_serial as cs
from pup_core.model.up_core import UpErrorCode
from pup_core.model.up_exception import SerialException
from pup_core.signals import error_signal

from dataclasses import dataclass


def list_serial_ports():
    return cs.list_serial_ports()


class CommandResult(NamedTuple):
    success: bool
    response: Optional[bytearray] = None


class SerialPort:
    def __init__(self, ser: serial.Serial):
        self.ser = ser
        self.received_data_queue = asyncio.Queue()
        self.pending_response: Optional[asyncio.Future] = None
        self.response_timeout = 3.0
        self.lock = asyncio.Lock()
        self._running = True  # 控制数据接收循环
        self.task: Optional[asyncio.Task] = None  # 明确初始化为 None

    async def enqueue_data(self, data: bytearray):
        await self.received_data_queue.put(data)

    async def send_command(self, data: bytes, wait_for_response: bool = True) -> CommandResult:
        async with self.lock:
            if not cs.write(self.ser, data):
                raise SerialException(UpErrorCode.WRITE_DATA_FAILED, "Failed to write data to serial port.")

            if not wait_for_response:
                return CommandResult(success=True, response=None)

            self.pending_response = asyncio.Future()

            try:
                response = await asyncio.wait_for(self.pending_response, timeout=self.response_timeout)
                return CommandResult(success=True, response=response)
            except asyncio.TimeoutError:
                logging.warning("No response received within timeout.")
                return CommandResult(success=False, response=None)
            finally:
                self.pending_response = None

    async def respond(self, data: bytearray):
        if self.pending_response and not self.pending_response.done():
            self.pending_response.set_result(data)

    async def receive_data(self, serial_id):
        try:
            while self._running:
                if self.ser.isOpen():
                    break

                available_bytes = self.ser.available()
                if available_bytes > 0:
                    buffer, bytes_read = self.ser.read(available_bytes)
                    byte_buffer = bytearray(buffer)

                    await self.enqueue_data(byte_buffer)
                    await self.respond(byte_buffer)
                else:
                    await asyncio.sleep(0.01)
        except serial.SerialException as e:
            logging.exception("读取串口数据时出错: %s", e)
            await error_signal.send_async("serial_error",
                                          serial_id=serial_id,  # 这里可以传入串口 ID
                                          message=f"Error reading serial data: {str(e)}")
        except Exception as e:
            logging.exception("读取串口数据时出错: %s", e)
            raise

    def close(self):
        self._running = False
        if self.pending_response and not self.pending_response.done():
            self.pending_response.set_result(None)

        if self.task:  # 取消任务
            try:
                self.task.cancel()
            except asyncio.CancelledError:
                pass  # 忽略任务取消异常

        if cs.is_open(self.ser):
            try:
                cs.close_serial(self.ser)
            except serial.SerialException as e:
                logging.error(f"关闭串口时出错: {e}")


class SerialManager:
    def __init__(self):
        self.serial_ports: Dict[str, SerialPort] = {}  # 存储打开的串口对象，键为串口 ID
        self._lock = threading.Lock()

    def open(self, device: str, baudrate: int = 9600, parity: str = 'none', databits: int = 8,
             stopbits: int = 1, flowcontrol: str = 'none', timeout: float = 1.0) -> str:
        with self._lock:
            for serial_id, serial_port in self.serial_ports.items():
                if serial_port.ser.getPort() == device:
                    if serial_port.ser.isOpen():
                        logging.info(f"串口 {device} 已经打开")
                        return serial_id

            serial_id = str(uuid.uuid4())
            ser = cs.open_serial(device, baudrate, parity, databits, stopbits, flowcontrol, timeout)

            serial_port = SerialPort(ser)
            self.serial_ports[serial_id] = serial_port

        serial_port.task = asyncio.create_task(serial_port.receive_data(serial_id))

        return serial_id

    def close(self, serial_id: str):
        with self._lock:
            if serial_id in self.serial_ports:
                serial_port = self.serial_ports.pop(serial_id)
                try:
                    serial_port.close()
                except serial.SerialException as e:
                    logging.error(f"关闭串口 {serial_id} 出错: {e}")

                logging.info(f"串口 {serial_id} 关闭成功")
            else:
                raise SerialException(UpErrorCode.SERIAL_NOT_FOUND, f"Serial port {serial_id} not found.")

    def _get_serial_port(self, serial_id: str) -> SerialPort:
        serial_port = self.serial_ports.get(serial_id)
        if not serial_port:
            raise SerialException(UpErrorCode.SERIAL_NOT_FOUND, f"Serial port {serial_id} not found.")
        return serial_port

    async def write(self, serial_id: str, data: bytes) -> bool:
        serial_port = self._get_serial_port(serial_id)
        result = await serial_port.send_command(data, wait_for_response=False)
        return result.success

    async def write_wait(self, serial_id: str, data: bytes) -> Optional[bytearray]:
        serial_port = self._get_serial_port(serial_id)
        result = await serial_port.send_command(data)
        if result.success:
            return result.response
        return None
