from typing import List
from typing import Optional

from pydantic import BaseModel, confloat
from pydantic import conint


class OpenSerialRequest(BaseModel):
    device: str
    baudrate: Optional[int] = 9600
    parity: Optional[str] = 'none'
    databits: Optional[int] = 8
    stopbits: Optional[int] = 1
    flowcontrol: Optional[str] = 'none'
    timeout: Optional[float] = 3.0


class SearchIdRequest(BaseModel):
    device: str
    baudrates: List[int] = []


class SerialRequest(BaseModel):
    serial_id: str


class HttpRequest(SerialRequest):
    protocol_id: conint(ge=0, le=255)


class WriteRequest(SerialRequest):
    raw_data: str


class AngleRequest(HttpRequest):
    angle: Optional[confloat(ge=0, le=300)] = 0  # 限制 angle 在 0 到 300 之间
    rpm: Optional[confloat(ge=1, le=62)] = 0


class MotorSpeedRequest(HttpRequest):
    rpm: Optional[confloat(ge=-62, le=62)] = 0


class LoadInfoRequest(HttpRequest):
    interval: Optional[int] = 1
