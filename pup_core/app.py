import logging

from fastapi import FastAPI, Request
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import JSONResponse

from pup_core.http_routes import serial_router, status_router
from pup_core.model.response_models import ErrorResponse
from pup_core.ws_routes import notifications_router, message_router
from starlette.exceptions import HTTPException as StarletteHTTPException
from fastapi.exceptions import RequestValidationError

app = FastAPI()


# 全局异常处理器: 捕获所有HTTP异常 (包括404等)
@app.exception_handler(StarletteHTTPException)
async def http_exception_handler(request: Request, exc: StarletteHTTPException):
    logging.error(f"HTTP Exception: {exc.detail}")
    return JSONResponse(
        status_code=exc.status_code,
        content=ErrorResponse(
            code=exc.status_code,
            message=exc.detail
        ).dict()
    )


# 全局异常处理器: 捕获所有422请求验证错误
@app.exception_handler(RequestValidationError)
async def validation_exception_handler(request: Request, exc: RequestValidationError):
    logging.error(f"Validation Error: {exc.errors()}")
    return JSONResponse(
        status_code=422,
        content=ErrorResponse(
            code=422,
            message="Validation Error",
            data=exc.errors()  # 返回具体的字段错误信息
        ).dict()
    )


# 全局异常处理
@app.exception_handler(Exception)
async def global_exception_handler(request: Request, exc: Exception):
    """全局异常处理，兜底处理未捕获的错误"""

    # 获取请求的 URL、方法和客户端 IP
    request_info = {
        "method": request.method,
        "url": str(request.url),
        "client_ip": request.client.host if request.client else "Unknown",
    }

    # 尝试获取请求体
    try:
        body = await request.json()
    except Exception:
        body = "Unable to retrieve body (non-JSON or large file)"

    # 记录详细日志
    logging.exception(
        f"Unhandled error occurred!\n"
        f"Request Info: {request_info}\n"
        f"Request Body: {body}\n"
        f"Exception: {exc}"
    )

    # 统一返回错误响应
    error_response = ErrorResponse(code=500, message="Internal Server Error")
    return JSONResponse(
        status_code=500,
        content=error_response.dict()
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
