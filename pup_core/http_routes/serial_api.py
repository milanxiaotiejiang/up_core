import logging
from urllib.request import Request

import up_core as serial
from fastapi import APIRouter, Depends, HTTPException
from ..dependencies import get_serial_manager
from up_core import Servo
from up_core import ServoProtocol
from pup_core.proto import Response
from pup_core.model import request_models
import time

from ..exceptions import handle_exceptions
from ..model.request_models import HttpRequest, SerialRequest, OpenSerialRequest
from ..model.response_models import BaseResponse
from ..model.response_models import SuccessResponse
from ..model.response_models import ErrorResponse
from ..servo_parser import perform_serial_data, perform_version
from ..servo_parser import ServoError

router = APIRouter()


@router.post("/open")
@handle_exceptions
async def open(request: OpenSerialRequest, serial_manager=Depends(get_serial_manager)):
    serial_id = serial_manager.open(request.device, request.baudrate, request.parity, request.databits,
                                    request.stopbits, request.flowcontrol, request.timeout)
    return SuccessResponse(status=True, data=serial_id)


@router.post("/close")
@handle_exceptions
async def close(request: SerialRequest, serial_manager=Depends(get_serial_manager)):
    serial_manager.close(request.serial_id)
    return SuccessResponse(status=True)


@router.post("/get_version")
@handle_exceptions
async def get_version(request: HttpRequest, serial_manager=Depends(get_serial_manager)):
    # 创建 ServoProtocol 实例
    servoProtocol = ServoProtocol(request.protocol_id)

    # 使用协议构建数据，这里假设是获取版本的命令
    data = servoProtocol.eeprom.buildGetSoftwareVersion()

    # 发送数据并获取返回数据
    byte_buffer = serial_manager.write_wait(request.serial_id, data)

    if byte_buffer is None:
        return ErrorResponse(status=False, message="No response received.")

    error_code, payload = perform_serial_data(byte_buffer)

    if error_code == ServoError.NO_ERROR:
        error_code, version = perform_version(payload)
        return SuccessResponse(status=True, data=version)
    else:
        return ErrorResponse(status=False, message="Failed to parse the response data.")


def list_serial_ports():
    """列出所有可用的串口设备，并打印信息。"""
    ports = serial.list_ports()

    if not ports:
        return {"message": "No available serial ports found."}

    port_list = []
    for port_info in ports:
        port_list.append({
            "port": port_info.port,
            "description": port_info.description,
            "hardware_id": port_info.hardware_id
        })

    return port_list


# 示例路由
@router.get("/list_serial_ports")
def read_root():
    return list_serial_ports()
