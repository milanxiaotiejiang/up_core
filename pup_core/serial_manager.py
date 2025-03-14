import asyncio
import uuid
from queue import Queue, Empty

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
        self.received_data_queue = Queue()  # 存储接收到的数据
        self.send_queue = Queue()  # 存储待发送的指令队列
        self.lock = threading.Lock()  # 串口操作锁，防止多线程访问冲突

    def enqueue_data(self, data):
        with self.lock:
            self.received_data_queue.put(data)
            self.data_event.set()

    def enqueue_send_command(self, data, need_response, done_callback=None):
        """
        将发送命令放入发送队列
        """
        command = {
            'data': data,
            'need_response': need_response,
            'done_callback': done_callback
        }
        with self.lock:
            self.send_queue.put(command)  # 添加到发送队列

    def process_send_queue(self):
        """
        处理发送队列中的指令
        """
        while True:
            try:
                command = self.send_queue.get(timeout=1)  # 获取发送命令
                data = command['data']
                need_response = command['need_response']

                # 发送数据
                with self.lock:
                    if not cs.write(self.ser, data):
                        raise SerialException(UpErrorCode.WRITE_DATA_FAILED,
                                              f"Failed to write data to serial port.")

                # 如果需要响应，则处理
                if need_response:
                    response = self._wait_for_response()  # 等待响应
                    if command['done_callback'] is not None:  # 确保回调存在
                        command['done_callback'](response)

                time.sleep(0.5)  # 不需要响应时稍作等待
            except Empty:  # 队列为空时的处理逻辑
                continue
            except Exception as e:
                logging.exception(f"处理发送队列时出错: {e}")

    def _wait_for_response(self):
        """
        等待响应数据
        """
        start_time = time.time()
        while time.time() - start_time < 3.0:  # 设置3秒超时
            if self.data_event.wait(timeout=0.1):
                if not self.received_data_queue.empty():
                    data = self.received_data_queue.get()
                    self.data_event.clear()
                    return data
        logging.warning("No response received within timeout.")
        return None


class SerialManager:
    def __init__(self):
        self.serial_ports: Dict[str, SerialPort] = {}  # 存储打开的串口对象，键为串口 ID
        self.lock = threading.Lock()  # 统一的锁以防多线程访问冲突
        self.receive_threads = {}  # 存储接收线程，用于管理
        self.send_threads = {}  # 用于管理发送线程

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

            # 启动发送线程
            self.start_sending_data(serial_id)

            return serial_id

    def close(self, serial_id: str):
        """
        关闭指定的串口，并移除对应的 SerialPort 实例。
        """
        with self.lock:
            if serial_id in self.serial_ports:
                serial_port = self.serial_ports.pop(serial_id)
                serial_port.stop_event.set()  # 设置停止事件，终止接收线程
                try:
                    if cs.is_open(serial_port.ser):
                        cs.close_serial(serial_port.ser)  # 关闭串口
                except Exception as e:
                    logging.error(f"关闭串口 {serial_id} 出错: {e}")
                finally:
                    # 确保线程退出后清理
                    if serial_id in self.receive_threads:
                        thread = self.receive_threads.pop(serial_id)
                        thread.join(timeout=5)  # 等待线程退出

                    if serial_id in self.send_threads:
                        serial_port.stop_event.set()  # 设置停止事件，终止发送线程
                        self.send_threads[serial_id].join(timeout=5)  # 等待发送线程退出

                logging.info(f"串口 {serial_id} 关闭成功")
            else:
                raise SerialException(UpErrorCode.SERIAL_NOT_FOUND, f"Serial port {serial_id} not found.")

    def write(self, serial_id: str, data: bytes):
        """
        发送数据到指定的串口，返回 True 表示成功。
        """
        serial_port = self.serial_ports.get(serial_id)
        if not serial_port:
            raise SerialException(UpErrorCode.SERIAL_NOT_FOUND, f"Serial port {serial_id} not found.")

        serial_port.enqueue_send_command(data, need_response=False)
        return True

    def write_wait(self, serial_id: str, data: bytes):
        """
        发送数据并等待响应。
        """
        serial_port = self.serial_ports.get(serial_id)
        if not serial_port:
            raise SerialException(UpErrorCode.SERIAL_NOT_FOUND, f"Serial port {serial_id} not found.")

        response = [None]  # 使用列表来存储响应，以便在回调中引用

        def set_response(r):
            response[0] = r  # 通过索引赋值

        serial_port.enqueue_send_command(data, need_response=True, done_callback=set_response)

        # 等待响应结果
        while response[0] is None:
            time.sleep(0.1)  # 等待响应
        return response[0]

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

                try:

                    available_bytes = ser.available()  # 检查可读取的数据字节数
                    if available_bytes > 0:
                        buffer, bytes_read = ser.read(available_bytes)  # 读取数据
                        # 将 list 转换为 bytearray
                        byte_buffer = bytearray(buffer)

                        serial_port.enqueue_data(byte_buffer)  # 放入接收队列
                    else:
                        time.sleep(0.1)  # 无数据时稍作等待，避免高 CPU 占用
                except Exception as e:
                    logging.exception("读取串口数据时出错: %s", e)
                    asyncio.create_task(
                        error_signal.send_async("serial_error",
                                                serial_id=serial_id,
                                                message=f"Error reading serial data: {str(e)}")
                    )
                    time.sleep(1)
        except Exception as e:
            logging.exception(f"严重错误: {e}")

    def start_receiving_data(self, serial_id: str):
        """
        启动一个新的线程用于接收串口数据。
        """
        receive_thread = threading.Thread(target=self._receive_data, args=(serial_id,), daemon=True)
        self.receive_threads[serial_id] = receive_thread
        receive_thread.start()

    def start_sending_data(self, serial_id: str):
        """启动一个新的线程用于发送串口数据。"""
        serial_port = self.serial_ports[serial_id]
        send_thread = threading.Thread(target=serial_port.process_send_queue, daemon=True)
        self.send_threads[serial_id] = send_thread
        send_thread.start()
