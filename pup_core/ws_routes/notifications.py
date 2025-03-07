import time

from fastapi import APIRouter, WebSocket, WebSocketDisconnect
import logging
from pup_core.proto import Notification
from blinker import signal  # 引入 blinker

from pup_core.signals import error_signal

logger = logging.getLogger("uvicorn")

# WebSocket 路由
websocket_router = APIRouter()
clients = []  # 保存连接的客户端


# 异步信号处理函数
async def send_serial_error_to_clients(message: str):
    if clients:
        notification = Notification()
        notification.title = "串口错误"
        notification.message = message

        # 序列化消息为二进制
        serialized_data = notification.SerializeToString()

        # 逐一发送给每个客户端
        for client in clients:
            try:
                await client.send_bytes(serialized_data)
            except Exception as e:
                logger.error(f"Failed to send message to {client.client}: {e}")
                clients.remove(client)


@websocket_router.websocket("")
async def websocket_notifications(websocket: WebSocket):
    logger.info(f"New WebSocket connection from {websocket.client}")

    await websocket.accept()
    clients.append(websocket)

    # 定义异步信号监听器
    async def serial_signal_handler(sender, message):
        """当接收到 blinker 信号时触发，发送消息给客户端"""
        logging.info(f"Received signal: {message}")
        await send_serial_error_to_clients(message)

    # 注册信号监听器
    error_signal.connect(serial_signal_handler)

    try:
        while True:
            # 接收二进制数据
            data = await websocket.receive_bytes()

            # 将二进制数据反序列化为 protobuf 消息
            notification = Notification()
            notification.ParseFromString(data)

            # 处理接收到的消息
            logger.info(f"Received notification: {notification}")

            # # 响应客户端
            # response = Notification()
            # response.title = "Notification"
            # response.message = f"Notification received: {notification.message}"
            # response.sender = "python"
            # response.receiver = "javascript"
            #
            # # 序列化消息并发送回客户端
            # await websocket.send_bytes(response.SerializeToString())

    except WebSocketDisconnect:
        logger.info(f"WebSocket connection closed from {websocket.client}")
        clients.remove(websocket)

    finally:
        # 在 WebSocket 断开时注销信号处理器
        error_signal.disconnect(serial_signal_handler)
