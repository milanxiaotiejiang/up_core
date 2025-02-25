import asyncio
import websockets
import json
from pup_core.proto import Request
from pup_core.proto import Response


# async def connect():
#     uri = "ws://127.0.0.1:8000/ws/notifications"  # WebSocket 服务的 URL
#     async with websockets.connect(uri) as websocket:
#         # 发送消息
#         await websocket.send("Hello, server!")
#         print("Message sent")
#
#         # 接收消息
#         response = await websocket.recv()
#         print(f"Message from server: {response}")
#
#
# # 运行客户端
# asyncio.get_event_loop().run_until_complete(connect())


async def send_notification():
    uri = "ws://localhost:8000/ws/notifications"
    async with websockets.connect(uri) as websocket:
        # 创建一个 Request 消息
        request = Request(
            id="12345",
            data=json.dumps({"message": "Hello, server!"}).encode('utf-8'),
            timestamp=1638463712,
            data_type="json"
        )

        # 序列化 Request 消息
        await websocket.send(request.SerializeToString())

        # 接收响应
        response_data = await websocket.recv()

        # 反序列化为 Response 消息
        response = Response()
        response.ParseFromString(response_data)

        print(f"Received Response: ID={response.id}, Data={response.data}, Timestamp={response.timestamp}")


# 启动客户端
asyncio.get_event_loop().run_until_complete(send_notification())
