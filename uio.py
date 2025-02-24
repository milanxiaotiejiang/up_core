import argparse
import up_core as up


def list_serial_ports():
    """列出所有可用的串口设备，并打印信息。"""
    ports = up.list_ports()

    if not ports:
        print("No available serial ports found.")
        return

    for port_info in ports:
        print(f"Port: {port_info.port}, Description: {port_info.description}, Hardware ID: {port_info.hardware_id}")


def open_serial(device, baudrate, parity, databits, stopbits, flowcontrol, timeout, raw_mode):
    """根据指定的参数打开串口并进行通信。"""
    pass  # 此处省略具体实现


def main():
    """主函数，解析命令行参数并根据参数执行相应功能。"""
    parser = argparse.ArgumentParser(description="UIO Serial Communication Tool")

    # 设备参数
    parser.add_argument('device', nargs='?', help='要连接的串口设备（如 /dev/ttyUSB0, COM1）')

    # 波特率
    parser.add_argument('-b', '--baudrate', type=int, default=9600, help='设置波特率 (默认: 9600)')

    # 奇偶校验
    parser.add_argument('-p', '--parity', choices=['none', 'even', 'odd'], default='none',
                        help='设置奇偶校验模式 (默认: none)')

    # 数据位
    parser.add_argument('-d', '--databits', type=int, choices=[7, 8], default=8, help='设置数据位长度 (默认: 8)')

    # 停止位
    parser.add_argument('-s', '--stopbits', type=float, choices=[1, 1.5, 2], default=1, help='设置停止位 (默认: 1)')

    # 流控制
    parser.add_argument('-f', '--flowcontrol', choices=['none', 'rtscts', 'xonxoff'], default='none',
                        help='设置流控制模式 (默认: none)')

    # 超时时间
    parser.add_argument('-t', '--timeout', type=float, default=1.0, help='设置超时时间 (单位: 秒)')

    # 列出串口设备
    parser.add_argument('-l', '--list', action='store_true', help='列出当前系统中所有可用的串口设备')

    # 持续收发模式
    parser.add_argument('-C', '--continuous', action='store_true', help='进入持续收发模式')

    # 详细模式
    parser.add_argument('-v', '--verbose', action='store_true', help='启用详细模式，打印调试信息')

    # raw模式
    parser.add_argument('-r', '--raw', action='store_true', help='以 raw 模式打开串口')

    # 解析参数
    args = parser.parse_args()

    if args.list:
        list_serial_ports()
    elif args.device:
        open_serial(
            device=args.device,
            baudrate=args.baudrate,
            parity=args.parity,
            databits=args.databits,
            stopbits=args.stopbits,
            flowcontrol=args.flowcontrol,
            timeout=args.timeout,
            raw_mode=args.raw,
        )
    else:
        parser.print_help()


if __name__ == "__main__":
    main()
