from fastapi.responses import JSONResponse
import asyncio
import logging
from functools import wraps

from pup_core.model.response_models import ErrorResponse, UpErrorCode
from pup_core.model.up_exception import SerialException

logger = logging.getLogger("exceptions")


def handle_exception(e):
    """统一异常处理"""
    exception_map = {
        SerialException: (400, lambda e: e.to_response()),
        ValueError: (400, lambda e: ErrorResponse(code=UpErrorCode.INVALID_PARAMS, message=str(e)).dict()),
        TypeError: (400, lambda _: ErrorResponse(code=400, message="Invalid input type").dict()),
        KeyError: (
        400, lambda e: ErrorResponse(code=UpErrorCode.MISSING_REQUIRED_FIELD, message=f"Missing key: {str(e)}").dict()),
        IndexError: (400, lambda _: ErrorResponse(code=400, message="Index out of range").dict()),
        AttributeError: (400, lambda _: ErrorResponse(code=400, message="Invalid attribute access").dict()),
        ZeroDivisionError: (400, lambda _: ErrorResponse(code=400, message="Cannot divide by zero").dict()),
        PermissionError: (403, lambda _: ErrorResponse(code=UpErrorCode.FORBIDDEN, message="Permission denied").dict()),
        FileNotFoundError: (404, lambda _: ErrorResponse(UpErrorCode.FILE_NOT_FOUND, message="File not found").dict()),
        TimeoutError: (408, lambda _: ErrorResponse(code=408, message="Request timed out").dict()),
        ConnectionError: (503, lambda _: ErrorResponse(code=503, message="Service unavailable").dict()),
    }

    for exc_type, (status, handler) in exception_map.items():
        if isinstance(e, exc_type):
            logging.exception(f"{exc_type.__name__}: {e}")
            return JSONResponse(status_code=status, content=handler(e))

    logging.exception(f"Unhandled error: {e}")
    return JSONResponse(status_code=500,
                        content=ErrorResponse(UpErrorCode.UNKNOWN_ERROR, "Internal Server Error").dict())


def handle_exceptions(func):
    """装饰器：统一捕获异常，并返回 ErrorResponse"""

    @wraps(func)
    async def async_wrapper(*args, **kwargs):
        try:
            return await func(*args, **kwargs)
        except Exception as e:
            return handle_exception(e)

    @wraps(func)
    def sync_wrapper(*args, **kwargs):
        try:
            return func(*args, **kwargs)
        except Exception as e:
            return handle_exception(e)

    return async_wrapper if asyncio.iscoroutinefunction(func) else sync_wrapper
