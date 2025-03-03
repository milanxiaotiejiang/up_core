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
HEARTBEAT_TIMEOUT = 30


# 心跳包检查器：每隔一段时间检查客户端心跳包是否超时
async def heartbeat_checker(websocket: WebSocket):
    while websocket in clients:
        await asyncio.sleep(HEARTBEAT_TIMEOUT)

        # 检查客户端的最后心跳时间
        last_heartbeat = clients.get(websocket)
        if last_heartbeat and time.time() - last_heartbeat > HEARTBEAT_TIMEOUT:
            logging.warning(f"Client {websocket.client.host} heartbeat timeout, disconnecting...")
            await websocket.close()  # 超时，关闭 WebSocket 连接
            clients.pop(websocket, None)
            break


@websocket_router.websocket("")
async def websocket_message(websocket: WebSocket):
    await websocket.accept()
    # 添加客户端到clients字典并初始化最后心跳时间
    clients[websocket] = time.time()

    # 启动心跳检查器
    asyncio.create_task(heartbeat_checker(websocket))

    try:
        while True:
            try:
                # 设置超时时间为 HEARTBEAT_TIMEOUT
                message = await asyncio.wait_for(websocket.receive(), timeout=HEARTBEAT_TIMEOUT)

                # 处理收到的消息
                if "text" in message:
                    message_data = message["text"]

                    # 处理心跳请求：收到 "ping" 返回 "pong"
                    if message_data == "ping":
                        await websocket.send_text("pong")
                        clients[websocket] = time.time()  # 更新心跳时间
                        logging.info("收到心跳请求 ping，返回 pong")
                        continue
                    else:
                        # 处理其他文本消息
                        logging.info(f"收到文本消息：{message_data}")
                        continue

                elif "bytes" in message:

                    message_data = message["bytes"]

                    # 反序列化为 Request 对象
                    request = Request()
                    request.ParseFromString(message_data)

                    # 解析 data_type 并处理数据
                    data_type = request.data_type
                    data = request.data
                    if data_type == "json":
                        # 假设 data 是 JSON 格式的字节数据，解码为 JSON 字典
                        data_dict = json.loads(data.decode("utf-8"))
                    else:
                        # 默认处理文本数据
                        data_dict = data.decode("utf-8")

                    logging.info(f"Received data: {data_dict} (type: {data_type})")

                    # 创建响应的 Response 对象
                    response = Response(
                        id=request.id,  # 使用相同的 id
                        timestamp=int(time.time()),  # 当前时间戳
                        data_type=data_type,  # 保持相同的数据类型
                    )

                    # 根据处理结果设置响应的 data 和 error_code
                    if data_type == "json":
                        # 假设我们返回一些 JSON 数据作为示例
                        response_data = "收到了你的消息，这是一个示例响应，你发送的数据是：" + json.dumps(data_dict)
                        response.data = json.dumps(response_data).encode('utf-8')
                        response.error_code = ""  # 没有错误
                    else:
                        # 如果是其他类型的消息
                        response.data = f"Processed {data}".encode('utf-8')
                        response.error_code = ""  # 没有错误

                    # 序列化响应并发送给客户端
                    await websocket.send_bytes(response.SerializeToString())

                    continue

            except asyncio.TimeoutError:
                logging.warning("心跳超时，关闭连接")
                await websocket.close()
                break

    except WebSocketDisconnect:
        clients.pop(websocket, None)  # 移除断开连接的客户端
        logging.info("客户端断开连接")
