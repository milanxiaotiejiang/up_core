from fastapi.responses import JSONResponse
import asyncio
import logging
from functools import wraps

from pup_core.model.response_models import ErrorResponse, UpErrorCode
from pup_core.model.up_exception import SerialException


def handle_exceptions(func):
    """装饰器：统一捕获异常，并返回 ErrorResponse"""

    @wraps(func)
    async def async_wrapper(*args, **kwargs):
        try:
            return await func(*args, **kwargs)

        except SerialException as e:
            return JSONResponse(
                status_code=400,  # 业务错误一般用 400
                content=e.to_response()
            )

        except ValueError as e:
            return JSONResponse(status_code=400,
                                content=ErrorResponse(
                                    code=UpErrorCode.INVALID_PARAMS, message=str(e)).dict()
                                )

        except TypeError as e:
            return JSONResponse(status_code=400,
                                content=ErrorResponse(
                                    code=400, message="Invalid input type").dict())

        except KeyError as e:
            return JSONResponse(status_code=400,
                                content=ErrorResponse(
                                    code=UpErrorCode.MISSING_REQUIRED_FIELD,
                                    message=f"Missing key: {str(e)}").dict())

        except IndexError as e:
            return JSONResponse(status_code=400,
                                content=ErrorResponse(
                                    code=400, message="Index out of range").dict())

        except AttributeError as e:
            return JSONResponse(status_code=400,
                                content=ErrorResponse(
                                    code=400, message="Invalid attribute access").dict())

        except ZeroDivisionError as e:
            return JSONResponse(status_code=400,
                                content=ErrorResponse(
                                    code=400, message="Cannot divide by zero").dict())

        except PermissionError as e:
            return JSONResponse(status_code=403,
                                content=ErrorResponse(
                                    code=UpErrorCode.FORBIDDEN, message="Permission denied").dict())

        except FileNotFoundError as e:
            return JSONResponse(status_code=404,
                                content=ErrorResponse(
                                    code=UpErrorCode.ORDER_NOT_FOUND, message="File not found").dict())

        except TimeoutError as e:
            return JSONResponse(status_code=408,
                                content=ErrorResponse(
                                    code=408, message="Request timed out").dict())

        except ConnectionError as e:
            return JSONResponse(status_code=503,
                                content=ErrorResponse(
                                    code=503, message="Service unavailable").dict())

        # except SQLAlchemyError as e:
        #     logging.error(f"Database error: {e}")
        #     return JSONResponse(status_code=500, content=ErrorResponse(code=500, message="Database error").dict())

        except Exception as e:
            logging.exception(f"Unhandled error in {func.__name__}: {e}")
            return JSONResponse(status_code=500,
                                content=ErrorResponse(
                                    code=UpErrorCode.UNKNOWN_ERROR, message="Internal Server Error").dict()
                                )

    @wraps(func)
    def sync_wrapper(*args, **kwargs):
        try:
            return func(*args, **kwargs)

        except SerialException as e:
            return JSONResponse(
                status_code=400,  # 业务错误一般用 400
                content=e.to_response()
            )

        except ValueError as e:
            return JSONResponse(status_code=400,
                                content=ErrorResponse(
                                    code=UpErrorCode.INVALID_PARAMS, message=str(e)).dict()
                                )

        except TypeError as e:
            return JSONResponse(status_code=400,
                                content=ErrorResponse(
                                    code=400, message="Invalid input type").dict())

        except KeyError as e:
            return JSONResponse(status_code=400,
                                content=ErrorResponse(
                                    code=UpErrorCode.MISSING_REQUIRED_FIELD,
                                    message=f"Missing key: {str(e)}").dict())

        except IndexError as e:
            return JSONResponse(status_code=400,
                                content=ErrorResponse(
                                    code=400, message="Index out of range").dict())

        except AttributeError as e:
            return JSONResponse(status_code=400,
                                content=ErrorResponse(
                                    code=400, message="Invalid attribute access").dict())

        except ZeroDivisionError as e:
            return JSONResponse(status_code=400,
                                content=ErrorResponse(
                                    code=400, message="Cannot divide by zero").dict())

        except PermissionError as e:
            return JSONResponse(status_code=403,
                                content=ErrorResponse(
                                    code=UpErrorCode.FORBIDDEN, message="Permission denied").dict())

        except FileNotFoundError as e:
            return JSONResponse(status_code=404,
                                content=ErrorResponse(
                                    code=UpErrorCode.ORDER_NOT_FOUND, message="File not found").dict())

        except TimeoutError as e:
            return JSONResponse(status_code=408,
                                content=ErrorResponse(
                                    code=408, message="Request timed out").dict())

        except ConnectionError as e:
            return JSONResponse(status_code=503,
                                content=ErrorResponse(
                                    code=503, message="Service unavailable").dict())

        # except SQLAlchemyError as e:
        #     logging.error(f"Database error: {e}")
        #     return JSONResponse(status_code=500, content=ErrorResponse(code=500, message="Database error").dict())

        except Exception as e:
            logging.exception(f"Unhandled error in {func.__name__}: {e}")
            return JSONResponse(
                status_code=500,
                content=ErrorResponse(code=UpErrorCode.UNKNOWN_ERROR, message="Internal Server Error").dict()
            )

    return async_wrapper if asyncio.iscoroutinefunction(func) else sync_wrapper
