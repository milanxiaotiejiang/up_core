import binascii
import logging

from fastapi import APIRouter, Depends
from up_core import ServoProtocol, ServoError

from pup_core.dependencies import get_serial_manager
from pup_core.exceptions import handle_exceptions
from pup_core.model.request_models import HttpRequest, AngleRequest
from pup_core.model.response_models import ErrorResponse, SuccessResponse
from pup_core.servo_parser import perform_serial_data

logger = logging.getLogger("servo_api")

router = APIRouter()


@router.post("/")
@handle_exceptions
async def servo(request: HttpRequest, serial_manager=Depends(get_serial_manager)):
    servoProtocol = ServoProtocol(request.protocol_id)
    data = servoProtocol.motor.buildServoMode()
    byte_buffer = await serial_manager.write_wait(request.serial_id, data)
    if byte_buffer is None:
        return ErrorResponse(status=False, message="No response received.")
    error_code, payload = perform_serial_data(byte_buffer)
    if error_code == ServoError.NO_ERROR:
        return SuccessResponse(status=True)
    else:
        return ErrorResponse(status=False, message="Parsing failed, servo error code: " + str(error_code))


@router.post("/angle")
@handle_exceptions
async def angle(request: AngleRequest, serial_manager=Depends(get_serial_manager)):
    servoProtocol = ServoProtocol(request.protocol_id)
    data = servoProtocol.ram.buildMoveToWithSpeedRpm(request.angle, request.rpm)
    byte_buffer = await serial_manager.write_wait(request.serial_id, data)
    if byte_buffer is None:
        return ErrorResponse(status=False, message="No response received.")
    error_code, payload = perform_serial_data(byte_buffer)
    if error_code == ServoError.NO_ERROR:
        return SuccessResponse(status=True)
    else:
        return ErrorResponse(status=False, message="Parsing failed, servo error code: " + str(error_code))
