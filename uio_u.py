import argparse
import base64
import up_core as up
from up_core import LogLevel, FirmwareUpdate

VERSION = "1.0.0"


# 固件升级的主要逻辑
def upgrade_firmware(device, baudrate, bin_path, file_buffer, servo_id, total_retry, handshake_count, frame_retry,
                     sign_retry):
    """调用固件升级逻辑"""
    fw_update = FirmwareUpdate()

    if file_buffer:
        # 使用字节流进行升级
        success = fw_update.upgrade_stream(device, baudrate, list(file_buffer), servo_id,
                                           total_retry, handshake_count, frame_retry, sign_retry)
    else:
        # 使用文件路径进行升级
        success = fw_update.upgrade_path(device, baudrate, bin_path, servo_id,
                                         total_retry, handshake_count, frame_retry, sign_retry)

    return success


def hex_int(x):
    return int(x, 16)


def decode_base64_to_bytes(base64_str: str) -> bytes:
    """解码无头部的 Base64 字符串"""

    # 如果包含前缀，去掉它
    if base64_str.startswith('data:'):
        base64_str = base64_str.split(',')[1]

    """解码无头部的 Base64 字符串"""
    return base64.b64decode(base64_str)


def convert_file_to_base64(file_path: str) -> str:
    """将文件转换为 Base64 字符串"""
    with open(file_path, 'rb') as file:
        file_data = file.read()

    # 对文件数据进行 Base64 编码
    base64_str = base64.b64encode(file_data).decode('utf-8')

    return base64_str


def main():
    """主函数，解析命令行参数并根据参数执行固件升级。"""
    parser = argparse.ArgumentParser(description="固件升级工具")

    # 设备参数
    parser.add_argument('device', nargs='?', help='要连接的串口设备（如 /dev/ttyUSB0, COM1）')

    # 固件文件路径
    parser.add_argument('bin_path', nargs='?', help='要升级的固件文件路径')

    # fileBuffer 参数，传入 base64 编码的字节流
    parser.add_argument('--fileBufferBase64', type=str, help='固件字节流的 base64 编码')

    # 设置波特率
    parser.add_argument('-b', '--baudrate', type=int, default=9600, help='设置波特率（默认: 9600）')

    # 设置舵机
    parser.add_argument('-s', '--servo_id', type=hex_int, default=0x01, help='设置舵机ID（默认: 0x01）')

    # 总重试次数
    parser.add_argument('-r', '--total_retry', type=int, default=10, help='固件升级的总重试次数（默认: 10 次）')

    # 握手计数阈值
    parser.add_argument('-hs', '--handshake_count', type=int, default=5, help='握手成功计数阈值（默认: 5）')

    # 固件帧重试次数
    parser.add_argument('-fr', '--frame_retry', type=int, default=5, help='固件数据帧发送的最大重试次数（默认: 5 次）')

    # 结束标志重试次数
    parser.add_argument('-sr', '--sign_retry', type=int, default=5, help='结束标志发送的最大重试次数（默认: 5 次）')

    # 日志级别
    parser.add_argument('-l', '--log', type=str, default="DEBUG", help='设置日志级别（DEBUG, INFO, ERROR）')

    # 显示版本号
    parser.add_argument('-v', '--version', action='version', version=f'%(prog)s {VERSION}', help='显示版本信息')

    # 解析参数
    args = parser.parse_args()

    # 设置日志级别
    log_level_map = {
        "DEBUG": LogLevel.DEBUG,
        "INFO": LogLevel.INFO,
        "ERROR": LogLevel.ERROR
    }
    up.set_log_level(log_level_map.get(args.log.upper(), LogLevel.DEBUG))

    # 读取 base64 编码的字节流
    file_buffer = None
    if args.fileBufferBase64:
        file_buffer = decode_base64_to_bytes(args.fileBufferBase64)

    # 调用固件升级
    if args.device:
        success = upgrade_firmware(
            args.device, args.baudrate, args.bin_path, file_buffer, args.servo_id,
            args.total_retry, args.handshake_count,
            args.frame_retry, args.sign_retry
        )
        print("升级成功" if success else "升级失败")
    else:
        parser.print_help()


if __name__ == "__main__":
    main()
    # base_ = convert_file_to_base64('/home/noodles/CLionProjects/up_core/file/CDS5516_1.0.bin')
    # print(base_)
