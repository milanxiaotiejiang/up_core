import json
import time

from fastapi import APIRouter, WebSocket, WebSocketDisconnect
import logging

from pup_core.model.EEPROMData import EEPROMData
from pup_core.model.RAMData import RAMData
from pup_core.model.notification_type import NotificationType
from pup_core.proto import Notification

from pup_core.signals import error_signal, search_signal, eeprom_signal, ram_signal

logger = logging.getLogger("notifications")

# WebSocket 路由
websocket_router = APIRouter()
clients = []  # 保存连接的客户端


async def notify_search_clients(baud, servo_id, error_code):
    if clients:
        notification = Notification()
        notification.type = NotificationType.SearchId
        notification.message = (json.dumps({"baud": baud, "servo_id": servo_id, "error_code": error_code})
                                .encode('utf-8'))

        await notify_clients(notification)


# 异步信号处理函数
async def notify_error_clients(serial_id: str, message: str):
    if clients:
        notification = Notification()
        notification.type = NotificationType.SerialError
        notification.message = json.dumps({"serial_id": serial_id, "message": message}).encode('utf-8')

        await notify_clients(notification)


async def notify_clients(notification: Notification):
    notification.timestamp = int(time.time())

    # 逐一发送给每个客户端
    for client in clients:
        try:
            await client.send_bytes(notification.SerializeToString())
        except Exception as e:
            logger.error(f"Failed to send message to {client.client}: {e}")
            clients.remove(client)


async def notify_eeprom_data_clients(serial_id: str, eeprom_data: EEPROMData):
    if clients:
        # 将 EEPROMData 对象转换为字典
        eeprom_data_dict = eeprom_data.to_dict()

        # 创建通知对象
        notification = Notification()
        notification.type = NotificationType.EEPROMData  # 假设有一个适当的通知类型
        notification.message = json.dumps({
            "serial_id": serial_id,
            "eeprom_data": eeprom_data_dict
        }).encode('utf-8')

        # 发送通知
        await notify_clients(notification)


async def notify_ram_data_clients(serial_id: str, ram_data: RAMData):
    if clients:
        # 将 RAMData 对象转换为字典
        ram_data_dict = ram_data.to_dict()

        # 创建通知对象
        notification = Notification()
        notification.type = NotificationType.RAMData  # 假设有一个适当的通知类型
        notification.message = json.dumps({
            "serial_id": serial_id,
            "ram_data": ram_data_dict
        }).encode('utf-8')

        # 发送通知
        await notify_clients(notification)


@websocket_router.websocket("")
async def websocket_notifications(websocket: WebSocket):
    logger.info(f"New WebSocket connection from {websocket.client}")

    await websocket.accept()
    clients.append(websocket)

    async def search_signal_handler(sender, baud, servo_id, error_code):
        logger.info(f"Received search signal: {baud}, {servo_id}, {error_code}")
        await notify_search_clients(baud, servo_id, error_code)

    search_signal.connect(search_signal_handler)

    # 定义异步信号监听器
    async def error_signal_handler(sender, serial_id, message):
        await notify_error_clients(serial_id, message)

    # 注册信号监听器
    error_signal.connect(error_signal_handler)

    async def eeprom_signal_handler(sender, serial_id, eeprom_data: EEPROMData):
        await notify_eeprom_data_clients(serial_id, eeprom_data)

    eeprom_signal.connect(eeprom_signal_handler)

    async def ram_signal_handler(sender, serial_id, ram_data: RAMData):
        await notify_ram_data_clients(serial_id, ram_data)

    ram_signal.connect(ram_signal_handler)

    try:
        while True:
            # 接收二进制数据
            data = await websocket.receive_bytes()

            # 将二进制数据反序列化为 protobuf 消息
            notification = Notification()
            notification.ParseFromString(data)

            # 处理接收到的消息
            logger.info(f"Received notification: {notification}")

    except WebSocketDisconnect:
        logger.info(f"WebSocket connection closed from {websocket.client}")
        clients.remove(websocket)

    finally:
        # 在 WebSocket 断开时注销信号处理器
        error_signal.disconnect(error_signal_handler)
        search_signal.disconnect(search_signal_handler)
        eeprom_signal.disconnect(eeprom_signal_handler)
        ram_signal.disconnect(ram_signal_handler)
