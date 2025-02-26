from fastapi import APIRouter, WebSocket, WebSocketDisconnect
from pup_core.proto import Request
from pup_core.proto import Response
import json
import time
import logging

websocket_router = APIRouter()

chat_clients = []


@websocket_router.websocket("")
async def websocket_message(websocket: WebSocket):
    await websocket.accept()
    chat_clients.append(websocket)

    try:
        while True:
            # 接收客户端的消息
            message = await websocket.receive_bytes()  # 这里接收的是二进制数据

            # 反序列化为 Request 对象
            request = Request()
            request.ParseFromString(message)

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
                response_data = {"status": "success", "message": "Data processed successfully."}
                response.data = json.dumps(response_data).encode('utf-8')
                response.error_code = ""  # 没有错误
            else:
                # 如果是其他类型的消息
                response.data = f"Processed {data}".encode('utf-8')
                response.error_code = ""  # 没有错误

            # 序列化响应并发送给客户端
            await websocket.send_bytes(response.SerializeToString())
    except WebSocketDisconnect:
        chat_clients.remove(websocket)
