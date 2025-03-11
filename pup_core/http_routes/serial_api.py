import binascii
import logging

from fastapi import APIRouter, Depends, HTTPException

import up_core as serial
from up_core import ServoProtocol
from up_core import ServoManager

from ..dependencies import get_serial_manager
from ..exceptions import handle_exceptions
from ..model.request_models import HttpRequest, SerialRequest, OpenSerialRequest, WriteRequest, SearchIdRequest
from ..model.response_models import SuccessResponse
from ..model.response_models import ErrorResponse
from ..servo_parser import perform_serial_data, perform_version
from ..servo_parser import ServoError

logger = logging.getLogger("serial_api")

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


@router.post("/write")
@handle_exceptions
async def sync_write(request: WriteRequest, serial_manager=Depends(get_serial_manager)):
    raw_data_hex = request.raw_data
    try:
        raw_data = binascii.unhexlify(raw_data_hex.replace(" ", ""))

        success = serial_manager.write(request.serial_id, raw_data)
        if success:
            return SuccessResponse(status=True)
        else:
            return ErrorResponse(status=False, message="Failed to write data.")
    except (binascii.Error, KeyError):
        raise HTTPException(status_code=400, detail="Invalid HEX data")


@router.post("/async_write")
@handle_exceptions
async def async_write(request: WriteRequest, serial_manager=Depends(get_serial_manager)):
    raw_data_hex = request.raw_data
    try:
        raw_data = binascii.unhexlify(raw_data_hex.replace(" ", ""))

        byte_buffer = serial_manager.write_wait(request.serial_id, raw_data)

        if byte_buffer is None:
            return ErrorResponse(status=False, message="No response received.")

        response_data = binascii.hexlify(byte_buffer).decode('utf-8')

        # 每两个字符添加一个空格，便于查看
        response_data_with_spaces = ' '.join([response_data[i:i + 2] for i in range(0, len(response_data), 2)])

        return SuccessResponse(status=True, data=response_data_with_spaces)
    except (binascii.Error, KeyError):
        raise HTTPException(status_code=400, detail="Invalid HEX data")


@router.post("/start_search")
@handle_exceptions
async def async_write(request: SearchIdRequest):
    pass


@router.post("/stop_search")
@handle_exceptions
async def async_write(request: WriteRequest):
    pass


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
    return SuccessResponse(status=True, data=list_serial_ports())
