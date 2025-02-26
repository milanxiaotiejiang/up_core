from pydantic import BaseModel
from typing import Any, Optional

from pup_core.model.up_core import UpErrorCode


class BaseResponse(BaseModel):
    status: bool  # 成功或失败，布尔类型
    code: Optional[int] = None  # 可选的状态码字段
    message: Optional[str] = None  # 可选的消息字段
    data: Optional[Any] = None  # 响应数据，可以是任意类型

    class Config:
        orm_mode = True  # 如果你使用ORM模型（例如数据库），可以启用这个选项


# 例如：成功响应的模型
class SuccessResponse(BaseResponse):
    status: bool = True  # 成功时状态为 True
    code: Optional[int] = UpErrorCode.SUCCESS  # 成功时状态码为 0


# 例如：错误响应的模型
class ErrorResponse(BaseResponse):
    status: bool = False  # 错误时状态为 False
    code: Optional[int] = 500  # 默认状态码为 500
