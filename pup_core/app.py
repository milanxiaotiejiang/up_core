from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from pup_core.http_routes import serial_router, status_router
from pup_core.ws_routes import notifications_router, message_router
import serial_manager as sm

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
app.include_router(message_router, prefix="/ws/message")

serial_manager = sm.SerialManager()
serial_id = serial_manager.open_serial("/dev/ttyUSB0", 1000000)
byte_buffer = serial_manager.write(serial_id, bytes.fromhex('FF FF 01 03 00 0A F1'))

hex_string = byte_buffer.hex().upper()  # 转换为大写
formatted_hex_string = ' '.join([hex_string[i:i + 2] for i in range(0, len(hex_string), 2)])

# 打印格式化后的十六进制字符串
print(f"\n收到数据: {formatted_hex_string}")
print()
print()
