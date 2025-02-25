import up_core as serial

import c_serial as cs

import threading
from typing import Dict
import time


class SerialPort:
    def __init__(self, ser: serial.Serial):
        self.ser = ser
        self.stop_event = threading.Event()
        self.data_event = threading.Event()  # 用于通知数据已接收
        self.received_data = None  # 存储接收到的数据


class SerialManager:
    def __init__(self):
        self.serial_ports: Dict[int, SerialPort] = {}
        self.lock = threading.Lock()
        self.responses = {}

    def open_serial(self, device, baudrate=9600, parity='none', databits=8,
                    stopbits=1, flowcontrol='none', timeout=1.0):
        with self.lock:
            # 检查串口是否已存在并且是否已经打开
            for serial_id, serial_port in self.serial_ports.items():
                if serial_port.ser.getPort() == device:
                    if serial_port.ser.isOpen():
                        print(f"串口 {device} 已经打开")
                        return serial_id  # 如果串口已打开，返回现有的串口 ID

            # 串口不存在时，创建一个新的串口实例
            serial_id = len(self.serial_ports) + 1
            ser = cs._open_serial(device, baudrate, parity, databits, stopbits, flowcontrol, timeout)

            serial_port = SerialPort(ser)
            self.serial_ports[serial_id] = serial_port

            self.start_receiving_data(serial_id)  # 启动串口数据接收线程

            return serial_id

    def close_serial(self, serial_id: int):
        with self.lock:
            if serial_id in self.serial_ports:
                serial_port = self.serial_ports.pop(serial_id)
                cs._close_serial(serial_port.ser)
                serial_port.stop_event.set()
            else:
                raise ValueError(f"Serial port {serial_id} not found.")

    def write(self, serial_id: int, data: bytes):
        with self.lock:
            if serial_id in self.serial_ports:
                serial_port = self.serial_ports[serial_id]
                if cs._write(serial_port.ser, data):
                    # 等待响应数据
                    return self._wait_for_response(serial_port)
            else:
                raise ValueError(f"Serial port {serial_id} not found.")

    def _wait_for_response(self, serial_port, timeout: int = 5):
        """
        等待串口的响应数据
        """
        start_time = time.time()
        while time.time() - start_time < timeout:
            # 等待数据
            if serial_port.data_event.wait(timeout=0.1):  # 阻塞直到数据到达
                data = serial_port.received_data
                return data
            time.sleep(0.1)
        return None  # 超时未响应

    def receive_data(self, serial_id: int):

        serial_port = self.serial_ports.get(serial_id)
        if not serial_port:
            return

        ser = serial_port.ser
        stop_event = serial_port.stop_event

        try:
            while ser.isOpen() and not stop_event.is_set():
                available_bytes = ser.available()
                if available_bytes > 0:
                    buffer, bytes_read = ser.read(available_bytes)

                    # 将 list 转换为 bytearray
                    byte_buffer = bytearray(buffer)

                    serial_port.received_data = byte_buffer
                    serial_port.data_event.set()  # 通知主线程数据已接收
                    serial_port.data_event.clear()  # 清除事件，准备下一次接收
                else:
                    # 如果没有数据可读，延时避免高CPU占用
                    time.sleep(0.1)
        except Exception as e:
            print(f"读取串口数据时出错: {e}")

    def get_serial_list(self):
        return cs._list_serial_ports()

    def start_receiving_data(self, serial_id: int):
        """
        启动串口数据接收线程
        """
        threading.Thread(target=self.receive_data, args=(serial_id,), daemon=True).start()
