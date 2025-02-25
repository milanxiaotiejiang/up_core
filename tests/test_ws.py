import asyncio
import websockets


async def connect():
    uri = "ws://127.0.0.1:8000/ws/notifications"  # WebSocket 服务的 URL
    async with websockets.connect(uri) as websocket:
        # 发送消息
        await websocket.send("Hello, server!")
        print("Message sent")

        # 接收消息
        response = await websocket.recv()
        print(f"Message from server: {response}")


# 运行客户端
asyncio.get_event_loop().run_until_complete(connect())
