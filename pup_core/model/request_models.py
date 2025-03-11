from pydantic import BaseModel, conint
from typing import Optional
from typing import List


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


# 定义请求体的数据结构
class HttpRequest(SerialRequest):
    protocol_id: conint(ge=0, le=255)


# 定义请求体的数据结构
class WriteRequest(SerialRequest):
    raw_data: str
