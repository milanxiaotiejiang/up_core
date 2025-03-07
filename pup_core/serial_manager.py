import asyncio
import uuid

import up_core as serial
import pup_core.c_serial as cs
import threading
from typing import Dict
import time
import logging

from pup_core.model.up_core import UpErrorCode
from pup_core.model.up_exception import SerialException
from pup_core.signals import error_signal


def list_serial_ports():
    """
    获取系统可用的串口列表。
    """
    return cs.list_serial_ports()


class SerialPort:
    def __init__(self, ser: serial.Serial):
        self.ser = ser  # 关联的串口对象
        self.stop_event = threading.Event()  # 线程停止事件，控制接收线程的退出
        self.data_event = threading.Event()  # 数据接收事件，用于通知主线程有数据到达
        self.received_data = None  # 存储接收到的数据
        self.lock = threading.Lock()  # 串口操作锁，防止多线程访问冲突


class SerialManager:
    def __init__(self):
        self.serial_ports: Dict[str, SerialPort] = {}  # 存储所有打开的串口对象，键为串口 ID
        self.lock = threading.Lock()  # 线程锁，防止多线程访问冲突
        self.receive_threads = {}  # 存储接收线程，用于管理线程退出

    def open(self, device, baudrate=9600, parity='none', databits=8,
             stopbits=1, flowcontrol='none', timeout=1.0):
        """
        打开串口，并创建一个 SerialPort 实例。
        如果串口已存在并打开，则直接返回对应的串口 ID。
        """
        with self.lock:
            # 检查当前串口是否已打开
            for serial_id, serial_port in self.serial_ports.items():
                if serial_port.ser.getPort() == device:
                    if serial_port.ser.isOpen():
                        logging.info(f"串口 {device} 已经打开")
                        return serial_id  # 返回已有的串口 ID

            # 如果串口不存在，则创建一个新的串口实例
            # serial_id = len(self.serial_ports) + 1
            serial_id = str(uuid.uuid4())
            ser = cs.open_serial(device, baudrate, parity, databits, stopbits, flowcontrol, timeout)

            serial_port = SerialPort(ser)
            self.serial_ports[serial_id] = serial_port

            # 启动串口数据接收线程
            self.start_receiving_data(serial_id)

            return serial_id

    def close(self, serial_id: str):
        """
        关闭指定的串口，并移除对应的 SerialPort 实例。
        """
        with self.lock:
            if serial_id in self.serial_ports:
                serial_port = self.serial_ports.pop(serial_id)
                cs.close_serial(serial_port.ser)  # 关闭串口
                serial_port.stop_event.set()  # 设置停止事件，终止接收线程
                if serial_id in self.receive_threads:
                    self.receive_threads[serial_id].join()  # 确保接收线程退出
                    del self.receive_threads[serial_id]
            else:
                raise SerialException(UpErrorCode.SERIAL_NOT_FOUND, f"Serial port {serial_id} not found.")

    def write(self, serial_id: str, data: bytes):
        """
        发送数据到指定的串口，并等待响应数据。
        """
        if serial_id in self.serial_ports:
            serial_port = self.serial_ports[serial_id]
            return cs.write(serial_port.ser, data)
        else:
            raise SerialException(UpErrorCode.SERIAL_NOT_FOUND, f"Serial port {serial_id} not found.")

    def write_wait(self, serial_id: str, data: bytes):
        """
        发送数据到指定的串口，并等待响应数据。
        """
        if serial_id in self.serial_ports:
            serial_port = self.serial_ports[serial_id]
            with serial_port.lock:  # 确保只有一个线程访问串口
                if cs.write(serial_port.ser, data):  # 调用底层写入方法
                    return self._wait_for_response(serial_port)  # 等待响应
                else:
                    raise SerialException(UpErrorCode.WRITE_DATA_FAILED,
                                          f"Failed to write data to serial port {serial_id}.")
        else:
            raise SerialException(UpErrorCode.SERIAL_NOT_FOUND, f"Serial port {serial_id} not found.")

    def _wait_for_response(self, serial_port, timeout: int = 3, retries: int = 1):
        """
        等待串口返回的数据，最多等待 timeout 秒。
        """
        attempt = 0
        while attempt < retries:
            start_time = time.time()
            while time.time() - start_time < timeout:
                if serial_port.data_event.wait(timeout=0.1):  # 等待数据事件触发
                    data = serial_port.received_data
                    serial_port.received_data = None
                    return data
                time.sleep(0.1)
            attempt += 1
            logging.warning(f"Retrying to receive data from serial port {serial_port.ser.getPort()}, attempt {attempt}")
        return None  # 超时未收到响应

    def _receive_data(self, serial_id: str):
        """
        串口数据接收线程，持续监听串口数据。
        """
        serial_port = self.serial_ports.get(serial_id)
        if not serial_port:
            logging.error(f"Serial port {serial_id} not found for receiving data.")
            return

        ser = serial_port.ser
        stop_event = serial_port.stop_event

        try:
            while ser.isOpen() and not stop_event.is_set():
                available_bytes = ser.available()  # 检查可读取的数据字节数
                if available_bytes > 0:
                    buffer, bytes_read = ser.read(available_bytes)  # 读取数据

                    # 将 list 转换为 bytearray
                    byte_buffer = bytearray(buffer)

                    serial_port.received_data = byte_buffer
                    serial_port.data_event.set()  # 触发数据事件，通知主线程
                    serial_port.data_event.clear()  # 清除事件，准备接收下一次数据
                else:
                    time.sleep(0.1)  # 无数据时稍作等待，避免高 CPU 占用
        except Exception as e:
            logging.exception("读取串口数据时出错: %s", e)
            asyncio.run(error_signal.send_async("serial_error", message=f"Error reading serial data: {str(e)}"))
            raise

    def start_receiving_data(self, serial_id: str):
        """
        启动一个新的线程用于接收串口数据。
        """
        receive_thread = threading.Thread(target=self._receive_data, args=(serial_id,), daemon=True)
        self.receive_threads[serial_id] = receive_thread
        receive_thread.start()
