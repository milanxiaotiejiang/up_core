from fastapi import APIRouter
import logging

from pup_core.exceptions import handle_exceptions
from pup_core.model.response_models import SuccessResponse, ErrorResponse
from pup_core.model.up_exception import UnauthorizedException, ForbiddenException

logger = logging.getLogger("status_api")

router = APIRouter()


@router.get("/status")
def get_status():
    return {"status": "Server is running"}


@router.get("/success")
@handle_exceptions
async def success():
    return SuccessResponse(status=True, data="test success response")


@router.get("/fail")
@handle_exceptions
async def fail():
    return ErrorResponse(code=-1, data="test fail response")


@router.get("/error")
@handle_exceptions
async def error():
    raise ValueError("test error response")


@router.get("/serial")
@handle_exceptions
async def error():
    raise ForbiddenException("您没有权限执行此操作")
