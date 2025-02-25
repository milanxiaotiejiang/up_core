from fastapi import APIRouter, WebSocket, WebSocketDisconnect
import logging

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
            data = await websocket.receive_text()
            await websocket.send_text(f"Notification received: {data}")
    except WebSocketDisconnect:
        logger.info(f"WebSocket connection closed from {websocket.client}")
        clients.remove(websocket)
