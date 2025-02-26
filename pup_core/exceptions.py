from fastapi import HTTPException
import logging
from functools import wraps


class SerialException(Exception):
    def __init__(self, message: str):
        self.message = message
        super().__init__(message)


def handle_exceptions(func):
    @wraps(func)
    async def wrapper(*args, **kwargs):
        try:
            return await func(*args, **kwargs)
        except ValueError as e:
            logging.error(f"ValueError: {e}")
            raise HTTPException(status_code=400, detail=str(e))
        except Exception as e:
            logging.exception(f"Unexpected error: {e}")
            raise HTTPException(status_code=500, detail="An internal server error occurred.")

    return wrapper
