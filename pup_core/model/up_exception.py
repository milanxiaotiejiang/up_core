from pup_core.model.response_models import ErrorResponse


class UpException(Exception):
    """ 业务异常基类 """

    def __init__(self, code: int, message: str):
        self.code = code
        self.message = message
        super().__init__(message)

    def to_response(self):
        """ 转换为标准错误响应格式 """
        return ErrorResponse(status=False, code=self.code, message=self.message).dict()


class InvalidParamsException(UpException):
    """ 参数错误异常 """

    def __init__(self, message: str = "Invalid parameters"):
        super().__init__(code=1001, message=message)


class UnauthorizedException(UpException):
    """ 未授权访问异常 """

    def __init__(self, message: str = "Unauthorized"):
        super().__init__(code=2001, message=message)


class ForbiddenException(UpException):
    """ 权限不足异常 """

    def __init__(self, message: str = "Forbidden access"):
        super().__init__(code=2002, message=message)


class NotFoundException(UpException):
    """ 资源未找到异常 """

    def __init__(self, message: str = "Resource not found"):
        super().__init__(code=3001, message=message)


class SerialException(Exception):
    """ 业务异常基类 """

    def __init__(self, code: int, message: str):
        self.code = code
        self.message = message
        super().__init__(message)

    def to_response(self):
        """ 转换为标准错误响应格式 """
        return ErrorResponse(status=False, code=self.code, message=self.message).dict()
