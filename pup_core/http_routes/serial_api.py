import binascii
import logging

from fastapi import APIRouter, Depends, HTTPException

import up_core as serial
from up_core import ServoProtocol
from up_core import ServoManager
from up_core import Base

from ..dependencies import get_serial_manager
from pup_core.exception.exceptions import handle_exceptions
from ..model.request_models import HttpRequest, SerialRequest, OpenSerialRequest, WriteRequest, SearchIdRequest, \
    AngleRequest, LoadInfoRequest
from ..model.response_models import SuccessResponse
from ..model.response_models import ErrorResponse
from pup_core.serial.servo_parser import preview_data, perform_version
from pup_core.serial.servo_parser import ServoError
from ..serial.load_info_controller import start_info_task, stop_all_info_tasks, stop_info_task_for_serial_id, \
    stop_info_task
from ..utils.resolve import identify_mode

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


@router.post("/reset")
@handle_exceptions
async def close(request: HttpRequest, serial_manager=Depends(get_serial_manager)):
    servoProtocol = Base(request.protocol_id)
    data = servoProtocol.buildResetPacket()
    success = await serial_manager.write(request.serial_id, data)
    if success:
        return SuccessResponse(status=True)
    else:
        return ErrorResponse(status=False, message="Failed to reset servo.")


@router.post("/get_version")
@handle_exceptions
async def get_version(request: HttpRequest, serial_manager=Depends(get_serial_manager)):
    # 创建 ServoProtocol 实例
    servoProtocol = ServoProtocol(request.protocol_id)

    # 使用协议构建数据，这里假设是获取版本的命令
    data = servoProtocol.eeprom.buildGetSoftwareVersion()

    # 发送数据并获取返回数据
    byte_buffer = await serial_manager.write_wait(request.serial_id, data)

    if byte_buffer is None:
        return ErrorResponse(status=False, message="No response received.")

    error_code, payload = preview_data(byte_buffer)

    if error_code == ServoError.NO_ERROR:
        error_code, version = perform_version(payload)
        return SuccessResponse(status=True, data=version)
    else:
        return ErrorResponse(status=False, message="Parsing failed, servo error code: " + str(error_code))


@router.post("/write")
@handle_exceptions
async def sync_write(request: WriteRequest, serial_manager=Depends(get_serial_manager)):
    raw_data_hex = request.raw_data
    try:
        raw_data = binascii.unhexlify(raw_data_hex.replace(" ", ""))

        success = await serial_manager.write(request.serial_id, raw_data)
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

        byte_buffer = await serial_manager.write_wait(request.serial_id, raw_data)

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


@router.post("/mode")
@handle_exceptions
async def mode(request: HttpRequest, serial_manager=Depends(get_serial_manager)):
    servoProtocol = ServoProtocol(request.protocol_id)
    data = servoProtocol.eeprom.buildGetCwAngleLimit()
    byte_buffer = await serial_manager.write_wait(request.serial_id, data)

    if byte_buffer is None:
        return ErrorResponse(status=False, message="No response received.")

    error_code, payload = preview_data(byte_buffer)

    if error_code == ServoError.NO_ERROR:
        return SuccessResponse(status=True, data=identify_mode(payload))
    else:
        return ErrorResponse(status=False, message="Parsing failed, servo error code: " + str(error_code))


@router.post("/load_info/start")
async def start_task(request: LoadInfoRequest, serial_manager=Depends(get_serial_manager)):
    if serial_manager.is_open(request.serial_id):
        await start_info_task(serial_manager, request.serial_id, request.protocol_id, request.interval)
        return SuccessResponse(status=True, message="Task started.")
    else:
        return ErrorResponse(status=False, message="Serial port is not open.")


@router.post("/load_info/stop/all")
async def stop_task():
    await stop_all_info_tasks()
    return SuccessResponse(status=True, message="All tasks stopped.")


@router.post("/load_info/stop/serial")
async def stop_task(request: HttpRequest):
    await stop_info_task_for_serial_id(request.serial_id)
    return SuccessResponse(status=True, message="Task stopped.")


@router.post("/load_info/stop")
async def stop_task(request: HttpRequest):
    await stop_info_task(request.serial_id, request.protocol_id)
    return SuccessResponse(status=True, message="Task stopped.")


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
