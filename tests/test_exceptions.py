import asyncio

import pytest
from fastapi.responses import JSONResponse
from pup_core.exceptions import handle_exceptions, SerialException, ErrorResponse, UpErrorCode


@handle_exceptions
def sync_test_func(exception_type=None):
    if exception_type:
        raise exception_type("test error")
    return "success"


@handle_exceptions
async def async_test_func(exception_type=None):
    if exception_type:
        raise exception_type("test error")
    return "success"


@pytest.mark.parametrize("test_func", [sync_test_func, async_test_func])
@pytest.mark.parametrize("exception, status_code, expected_message", [
    (SerialException, 400, "test error"),
    (ValueError, 400, "test error"),
    (TypeError, 400, "Invalid input type"),
    (KeyError, 400, "Missing key: 'test error'"),
    (IndexError, 400, "Index out of range"),
    (AttributeError, 400, "Invalid attribute access"),
    (ZeroDivisionError, 400, "Cannot divide by zero"),
    (PermissionError, 403, "Permission denied"),
    (FileNotFoundError, 404, "File not found"),
    (TimeoutError, 408, "Request timed out"),
    (ConnectionError, 503, "Service unavailable"),
    (Exception, 500, "Internal Server Error"),
])
@pytest.mark.asyncio
async def test_handle_exceptions(test_func, exception, status_code, expected_message):
    response = await test_func(exception) if asyncio.iscoroutinefunction(test_func) else test_func(exception)
    assert isinstance(response, JSONResponse)
    assert response.status_code == status_code
    assert expected_message in response.body.decode()
