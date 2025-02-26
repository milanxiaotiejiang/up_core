# 定义业务错误码参考示例
# 业务模块	业务错误码范围	说明
# 通用错误	1000-1099	适用于所有模块的通用错误
# 用户相关	2000-2099	登录、权限、账户相关
# 串口相关	3000-3099	串口通信、设备操作等
# 支付相关	5000-5099	交易、退款等


class UpErrorCode:
    SUCCESS = 0  # 成功
    INVALID_PARAMS = 1001  # 参数错误
    MISSING_REQUIRED_FIELD = 1002  # 缺少必填字段

    UNAUTHORIZED = 2001  # 未授权访问
    FORBIDDEN = 2002  # 权限不足
    USER_NOT_FOUND = 2003  # 用户不存在

    UNDER_ERROR = 3000  # 内部错误
    OPEN_SERIAL_FAILED = 3001  # 打开串口失败
    SERIAL_NOT_OPEN = 3002  # 串口未打开
    WRITE_DATA_FAILED = 3003  # 写入数据失败
    NO_DATA_AVAILABLE = 3004  # 没有可用数据
    SERIAL_NOT_FOUND = 3005  # 串口未找到

    PAYMENT_FAILED = 5001  # 支付失败

    UNKNOWN_ERROR = 9999  # 未知错误
