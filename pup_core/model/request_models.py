from pydantic import BaseModel, conint


# 定义请求体的数据结构
class HttpRequest(BaseModel):
    serial_id: int
    protocol_id: conint(ge=0, le=255)
