import logging

import up_core as serial
from fastapi import APIRouter, Depends, HTTPException
from ..dependencies import get_serial_manager
from up_core import Servo
from up_core import ServoProtocol
from pup_core.proto import Response
from pup_core.model import request_models
import time

from ..model.request_models import HttpRequest
from ..model.response_models import BaseResponse
from ..model.response_models import SuccessResponse
from ..model.response_models import ErrorResponse

router = APIRouter()


@router.get("/open")
def open(serial_manager=Depends(get_serial_manager)):
    try:
        serial_id = serial_manager.open_serial("/dev/ttyUSB0", 1000000)
        return SuccessResponse(status=True, data=serial_id)
    except Exception as e:
        logging.exception("serial_api open: %s", e)
        raise HTTPException(status_code=500, detail="An error occurred while processing the request.")


@router.post("/get_version")
def get_version(request: HttpRequest, serial_manager=Depends(get_serial_manager)):
    try:
        # 创建 ServoProtocol 实例
        servoProtocol = ServoProtocol(request.protocol_id)

        # 使用协议构建数据，这里假设是获取版本的命令
        data = servoProtocol.eeprom.buildGetSoftwareVersion()

        # 发送数据并获取返回数据
        byte_buffer = serial_manager.write(request.serial_id, data)

        # 返回标准的HTTP响应格式
        return SuccessResponse(status=True, data=byte_buffer.hex().upper())
    except ValueError as e:
        # 如果串口没有找到，或者写入失败等情况，返回错误
        return ErrorResponse(status=False, message=str(e))
    except Exception as e:
        raise HTTPException(status_code=500, detail="An error occurred while processing the request.")


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
