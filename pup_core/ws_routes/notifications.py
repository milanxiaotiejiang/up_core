from fastapi import APIRouter, WebSocket, WebSocketDisconnect
import logging
from pup_core.proto import Notification

logger = logging.getLogger("uvicorn")

websocket_router = APIRouter()
clients = []


@websocket_router.websocket("")
async def websocket_notifications(websocket: WebSocket):
    logger.info(f"New WebSocket connection from {websocket.client}")

    await websocket.accept()
    clients.append(websocket)

    try:
        while True:
            # 接收二进制数据
            data = await websocket.receive_bytes()

            # 将二进制数据反序列化为 protobuf 消息
            notification = Notification()
            notification.ParseFromString(data)

            # 处理接收到的消息（可以根据需要修改）
            logger.info(f"Received notification: {notification.message}")

            # 响应客户端（可以根据需要修改）
            response = Notification()
            response.message = f"Notification received: {notification.message}"

            # 序列化消息并发送回客户端
            await websocket.send_bytes(response.SerializeToString())

    except WebSocketDisconnect:
        logger.info(f"WebSocket connection closed from {websocket.client}")
        clients.remove(websocket)
