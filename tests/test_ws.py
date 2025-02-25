import asyncio
import websockets

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


from pup_core.proto import message_pb2  # 导入生成的 protobuf 文件


async def send_notification():
    uri = "ws://localhost:8000/ws/notifications"
    async with websockets.connect(uri) as websocket:
        # 创建一个 Notification 消息
        notification = message_pb2.Notification()
        notification.message = "Hello, server!"
        notification.timestamp = 1625194261

        # 序列化为二进制数据
        serialized_data = notification.SerializeToString()

        # 发送二进制数据
        await websocket.send(serialized_data)

        # 接收服务器的响应
        response = await websocket.recv()

        # 反序列化服务器返回的数据
        response_message = message_pb2.Notification()
        response_message.ParseFromString(response)

        print(f"Received from server: {response_message.message}")


# 启动客户端
asyncio.get_event_loop().run_until_complete(send_notification())
