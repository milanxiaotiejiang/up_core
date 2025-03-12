import click
from pup_core.core import run_server
from up_core import LogLevel
import up_core as up
import logging

from pup_core.process.zero_mq_search import start_search

# 定义 click 的 log-level 和 LogLevel 枚举的映射
LOG_LEVEL_MAP = {
    'debug': LogLevel.DEBUG,
    'info': LogLevel.INFO,
    'warning': LogLevel.WARNING,
    'error': LogLevel.ERROR,
}


@click.command()
@click.option('--host', '-h', default='0.0.0.0', help='Host to run the server on')
@click.option('--port', '-p', default=8000, help='Port to run the server on')
@click.option('--reload', '-r', is_flag=True, help='Enable auto-reload for development')
@click.option('--log-level', '-l', default='info',
              type=click.Choice(['debug', 'info', 'warning', 'error'], case_sensitive=False), help='Set the log level')
def cli(host, port, reload, log_level):
    """启动 pup_core 服务器，支持多个参数"""

    # 配置日志系统
    logging.basicConfig(level=logging.DEBUG,  # 日志级别：DEBUG、INFO、WARNING、ERROR、CRITICAL
                        format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')  # 日志格式

    # 将 click 的 log-level 转换为 up_core 中的 LogLevel
    up.set_log_level(LOG_LEVEL_MAP[log_level])

    start_search()

    # 启动服务器
    run_server(host=host, port=port, reload=reload, log_level=log_level)


if __name__ == '__main__':
    cli()
