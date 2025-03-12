import threading
import time

import zmq
import logging
from pup_core.signals import search_signal, error_signal
import asyncio

# 获取名为 "search_listener" 的日志记录器
logger = logging.getLogger("search_listener")


# 启动搜索监听器线程
def start_search():
    search_listener_thread = threading.Thread(target=start_search_listener)
    search_listener_thread.daemon = True  # 设置为守护线程，主程序退出时该线程也会退出
    search_listener_thread.start()


# 搜索监听器的具体实现
def start_search_listener():
    context = zmq.Context()  # 创建 ZeroMQ 上下文
    socket = context.socket(zmq.SUB)  # 创建 SUB 类型的 ZeroMQ 套接字
    socket.connect("tcp://localhost:5555")  # 连接到本地的 5555 端口
    socket.subscribe('')  # 订阅所有消息

    logger.info("ZeroMQ listener started, waiting for messages...")  # 记录日志，表示监听器已启动

    try:
        while True:
            try:
                message = socket.recv_json(flags=zmq.NOBLOCK)  # 非阻塞方式接收 JSON 消息

                logger.info(f"Received message: {message}")  # 记录接收到的消息
                asyncio.run(search_signal.send_async("search_id",
                                                     baud=message['baud'],
                                                     servo_id=message['servo_id'],
                                                     error_code=message['error_code']
                                                     )
                            )  # 异步发送信号

            except zmq.Again:
                pass  # 如果没有消息，继续循环
            except Exception as e:
                logger.error(f"Error receiving ZeroMQ message: {e}")  # 记录其他异常
            time.sleep(0.1)  # 每次循环后休眠 0.1 秒
    finally:
        socket.close()  # 关闭 ZeroMQ 套接字
        context.term()  # 终止 ZeroMQ 上下文
