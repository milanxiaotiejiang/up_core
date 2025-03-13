import binascii
import logging

from fastapi import APIRouter, Depends
from up_core import ServoProtocol, ServoError

from pup_core.dependencies import get_serial_manager
from pup_core.exceptions import handle_exceptions
from pup_core.model.request_models import HttpRequest, MotorSpeedRequest
from pup_core.model.response_models import ErrorResponse, SuccessResponse
from pup_core.servo_parser import perform_serial_data

logger = logging.getLogger("motor_api")

router = APIRouter()


@router.post("/")
@handle_exceptions
async def motor(request: HttpRequest, serial_manager=Depends(get_serial_manager)):
    servoProtocol = ServoProtocol(request.protocol_id)
    data = servoProtocol.motor.buildMotorMode()

    # ff ff 01 07 03 06 00 00 00 00 ee
    # ff ff 00 07 03 06 00 00 00 00 ef
    data_bytes = bytes(data)
    # 将 bytes 转换为十六进制字符串
    logger.info(f"Motor mode hex data: {binascii.hexlify(data_bytes).decode('utf-8')}")

    byte_buffer = serial_manager.write_wait(request.serial_id, data)
    if byte_buffer is None:
        return ErrorResponse(status=False, message="No response received.")
    error_code, payload = perform_serial_data(byte_buffer)
    if error_code == ServoError.NO_ERROR:
        return SuccessResponse(status=True)
    else:
        return ErrorResponse(status=False, message="Parsing failed, servo error code: " + str(error_code))


@router.post("/speed")
@handle_exceptions
async def angle(request: MotorSpeedRequest, serial_manager=Depends(get_serial_manager)):
    servoProtocol = ServoProtocol(request.protocol_id)
    data = servoProtocol.motor.buildSetMotorSpeed(request.rpm)
    byte_buffer = serial_manager.write_wait(request.serial_id, data)
    if byte_buffer is None:
        return ErrorResponse(status=False, message="No response received.")
    error_code, payload = perform_serial_data(byte_buffer)
    if error_code == ServoError.NO_ERROR:
        return SuccessResponse(status=True)
    else:
        return ErrorResponse(status=False, message="Parsing failed, servo error code: " + str(error_code))
