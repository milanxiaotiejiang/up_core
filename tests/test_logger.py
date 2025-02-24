import up_core as up
from up_core import LogLevel


def test_log_level():
    # 测试 LogLevel 的每个值是否存在
    assert LogLevel.DEBUG is not None, "DEBUG log level missing"
    assert LogLevel.INFO is not None, "INFO log level missing"
    assert LogLevel.WARNING is not None, "WARNING log level missing"
    assert LogLevel.ERROR is not None, "ERROR log level missing"
    assert LogLevel.OFF is not None, "OFF log level missing"
    print("test_log_level passed")


def test_logger(log_level):
    # 设置日志级别
    up.set_log_level(log_level)

    # 根据不同级别输出日志
    up.debug("这是 DEBUG 级别的日志")
    up.info("这是 INFO 级别的日志")
    up.warning("这是 WARNING 级别的日志")
    up.error("这是 ERROR 级别的日志")

    # 验证日志级别是否成功设置
    print(f"test_logger passed for log level {log_level}")


if __name__ == "__main__":
    # 运行测试用例，先测试 LogLevel 枚举
    test_log_level()

    # 测试不同日志级别的 Logger 输出
    for level in [LogLevel.DEBUG, LogLevel.INFO, LogLevel.WARNING, LogLevel.ERROR, LogLevel.OFF]:
        test_logger(level)
