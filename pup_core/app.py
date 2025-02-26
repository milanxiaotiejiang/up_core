import logging

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from pup_core.http_routes import serial_router, status_router
from pup_core.model.response_models import ErrorResponse
from pup_core.ws_routes import notifications_router, message_router

app = FastAPI()


# 全局异常处理
@app.exception_handler(Exception)
async def general_exception_handler(request, exc):
    # 这里可以根据异常类型进行处理，也可以直接返回通用的错误信息
    logging.exception(f"Unhandled error: {exc}")
    return ErrorResponse(
        status_code=500,
        content={"detail": "An internal server error occurred."}
    )


@app.exception_handler(ValueError)
async def value_error_exception_handler(request, exc):
    logging.error(f"ValueError: {exc}")
    return ErrorResponse(
        status_code=400,
        content={"detail": str(exc)}
    )


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
