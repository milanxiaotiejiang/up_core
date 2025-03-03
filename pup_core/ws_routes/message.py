import uuid

from fastapi import APIRouter, WebSocket, WebSocketDisconnect
from pup_core.proto import Request, Response
import json
import time
import asyncio
import logging

websocket_router = APIRouter()

# 存储客户端及其心跳包最后接收时间
clients = {}

# 设置心跳包超时时间（秒）
HEARTBEAT_TIMEOUT = 60


@websocket_router.websocket("")
async def websocket_message(websocket: WebSocket):
    await websocket.accept()

    client_key = str(uuid.uuid4())
    clients[client_key] = {"websocket": websocket, "last_heartbeat": time.time()}

    try:
        await accept_timeout(client_key, websocket)

    except WebSocketDisconnect:
        clients.pop(client_key, None)
        logging.info(f"客户端 {client_key} 断开连接")

    except RuntimeError as e:
        clients.pop(client_key, None)
        logging.error(f"连接出现异常 ({client_key}): {str(e)}")

    logging.info(f"当前在线客户端: {list(clients.keys())}")


async def accept_timeout(client_key, websocket):
    while True:
        try:
            message = await asyncio.wait_for(websocket.receive(), timeout=HEARTBEAT_TIMEOUT)

            if "text" in message:
                await handle_text(client_key, message, websocket)

            elif "bytes" in message:
                await handle_bytes(client_key, message, websocket)

        except asyncio.TimeoutError:
            logging.warning(f"心跳超时，关闭连接 ({client_key})")
            try:
                await websocket.close()
            except RuntimeError as e:
                logging.warning(f"WebSocket {client_key} 可能已经关闭: {e}")
            except Exception as e:
                logging.error(f"关闭 WebSocket 失败: {e}")
            clients.pop(client_key, None)
            break


async def handle_bytes(client_key, message, websocket):
    message_data = message["bytes"]
    request = Request()
    request.ParseFromString(message_data)
    data_type = request.data_type
    data = request.data
    if data_type == "json":
        data_dict = json.loads(data.decode("utf-8"))
    else:
        data_dict = data.decode("utf-8")
    logging.info(f"Received data from {client_key}: {data_dict} (type: {data_type})")
    response = Response(
        id=request.id,
        timestamp=int(time.time()),
        data_type=data_type,
    )
    if data_type == "json":
        response_data = "收到了你的消息，这是一个示例响应，你发送的数据是：" + json.dumps(data_dict)
        response.data = json.dumps(response_data).encode('utf-8')
        response.error_code = ""
    else:
        response.data = f"Processed {data}".encode('utf-8')
        response.error_code = ""
    await websocket.send_bytes(response.SerializeToString())


async def handle_text(client_key, message, websocket):
    message_data = message["text"]
    if message_data == "ping":
        await websocket.send_text("pong")
        clients[client_key]["last_heartbeat"] = time.time()
        logging.debug(f"收到心跳请求 ping，返回 pong ({client_key})")
    else:
        logging.info(f"收到文本消息：{message_data} ({client_key})")
