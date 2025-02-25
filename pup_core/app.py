from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from pup_core.http_routes import serial_router, status_router
from pup_core.ws_routes import notifications_router, chat_router

app = FastAPI()

# 允许跨域请求
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],  # 允许所有来源访问，或指定特定的来源
    allow_credentials=True,
    allow_methods=["*"],  # 允许所有 HTTP 方法
    allow_headers=["*"],  # 允许所有头部信息
)

# 包含 HTTP 路由
app.include_router(serial_router, prefix="/serial")
app.include_router(status_router, prefix="/status")

# 包含 WebSocket 路由
app.include_router(notifications_router, prefix="/ws/notifications")
app.include_router(chat_router, prefix="/ws/chat")
