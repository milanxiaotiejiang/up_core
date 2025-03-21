import argparse
import up_core as up
from up_core import LogLevel, FirmwareUpdate

VERSION = "1.0.0"


# 固件升级的主要逻辑
def upgrade_firmware(device, baudrate, bin_path, servo_id, total_retry, handshake_count, frame_retry, sign_retry):
    """调用固件升级逻辑"""
    fw_update = FirmwareUpdate()
    success = fw_update.upgrade(device, baudrate, bin_path, servo_id, total_retry, handshake_count, frame_retry,
                                sign_retry)
    return success


def hex_int(x):
    return int(x, 16)


def main():
    """主函数，解析命令行参数并根据参数执行固件升级。"""
    parser = argparse.ArgumentParser(description="固件升级工具")

    # 设备参数
    parser.add_argument('device', nargs='?', help='要连接的串口设备（如 /dev/ttyUSB0, COM1）')

    # 固件文件路径
    parser.add_argument('bin_path', nargs='?', help='要升级的固件文件路径')

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

    # 调用固件升级
    if args.device and args.bin_path:
        success = upgrade_firmware(
            args.device, args.baudrate, args.bin_path, args.servo_id,
            args.total_retry, args.handshake_count,
            args.frame_retry, args.sign_retry
        )
        print("升级成功" if success else "升级失败")
    else:
        parser.print_help()


if __name__ == "__main__":
    main()
