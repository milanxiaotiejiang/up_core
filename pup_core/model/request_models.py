from pydantic import BaseModel, conint


class SerialRequest(BaseModel):
    serial_id: str


# 定义请求体的数据结构
class HttpRequest(SerialRequest):
    protocol_id: conint(ge=0, le=255)
