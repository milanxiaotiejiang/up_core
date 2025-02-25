from fastapi import APIRouter, WebSocket, WebSocketDisconnect

websocket_router = APIRouter()

chat_clients = []


@websocket_router.websocket("")
async def websocket_chat(websocket: WebSocket):
    await websocket.accept()
    chat_clients.append(websocket)

    try:
        while True:
            message = await websocket.receive_text()
            await websocket.send_text(f"Message received: {message}")
    except WebSocketDisconnect:
        chat_clients.remove(websocket)
