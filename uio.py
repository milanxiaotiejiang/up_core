import argparse
import time
import threading

import up_core as serial

VERSION = "1.0.0"


def list_serial_ports():
    """列出所有可用的串口设备，并打印信息。"""
    ports = serial.list_ports()

    if not ports:
        print("No available serial ports found.")
        return

    for port_info in ports:
        print(f"Port: {port_info.port}, Description: {port_info.description}, Hardware ID: {port_info.hardware_id}")


def open_serial(device, baudrate=9600, parity='none', databits=8, stopbits=1, flowcontrol='none', timeout=1.0):
    """
    打开串口，并根据提供的参数配置。
    :param device: 串口设备路径（如 '/dev/ttyUSB0' 或 'COM1'）
    :param baudrate: 波特率（默认 9600）
    :param parity: 奇偶校验（none, even, odd, mark, space）
    :param databits: 数据位长度（5, 6, 7, 8）
    :param stopbits: 停止位（1, 1.5, 2）
    :param flowcontrol: 流控制模式（none, rtscts, xonxoff）
    :param timeout: 读写操作的超时时间（单位：秒）
    :return: 打开的串口对象
    """
    # 设置字节长度映射
    bytesize_map = {
        5: serial.bytesize_t.fivebits,
        6: serial.bytesize_t.sixbits,
        7: serial.bytesize_t.sevenbits,
        8: serial.bytesize_t.eightbits
    }

    # 设置奇偶校验映射
    parity_map = {
        'none': serial.parity_t.parity_none,
        'odd': serial.parity_t.parity_odd,
        'even': serial.parity_t.parity_even,
        'mark': serial.parity_t.parity_mark,
        'space': serial.parity_t.parity_space
    }

    # 设置停止位映射 - 修正了键值
    stopbits_map = {
        1: serial.stopbits_t.stopbits_one,
        2: serial.stopbits_t.stopbits_two,
        1.5: serial.stopbits_t.stopbits_one_point_five
    }

    # 设置流控制映射
    flowcontrol_map = {
        'none': serial.flowcontrol_t.flowcontrol_none,
        'software': serial.flowcontrol_t.flowcontrol_software,
        'hardware': serial.flowcontrol_t.flowcontrol_hardware
    }

    # 检查输入的参数是否正确
    if databits not in bytesize_map:
        raise ValueError(f"Unsupported databits: {databits}")
    if parity not in parity_map:
        raise ValueError(f"Unsupported parity: {parity}")
    if stopbits not in stopbits_map:
        raise ValueError(f"Unsupported stopbits: {stopbits}")
    if flowcontrol not in flowcontrol_map:
        raise ValueError(f"Unsupported flowcontrol: {flowcontrol}")

    # 创建串口对象
    try:
        ser = serial.Serial(
            port=device,
            baudrate=baudrate,
            timeout=serial.Timeout.simpleTimeout(int(timeout * 1000)),  # 将超时从秒转换为毫秒
            bytesize=bytesize_map[databits],
            parity=parity_map[parity],
            stopbits=stopbits_map[stopbits],
            flowcontrol=flowcontrol_map[flowcontrol]
        )

        # 确保串口已打开
        if not ser.isOpen():
            ser.open()  # 如果没有打开，则尝试打开

        return ser

    except Exception as e:
        raise Exception(f"Error opening serial port: {str(e)}")


def read_from_serial(ser, stop_event):
    """独立线程，用于从串口接收数据（以字节形式处理，不进行解码）"""
    try:
        while not stop_event.is_set():
            if ser.isOpen():  # 确保串口仍然打开
                available_bytes = ser.available()
                if available_bytes > 0:
                    buffer, bytes_read = ser.read(available_bytes)

                    # 将 list 转换为 bytearray
                    byte_buffer = bytearray(buffer)

                    hex_string = byte_buffer.hex().upper()  # 转换为大写
                    formatted_hex_string = ' '.join([hex_string[i:i + 2] for i in range(0, len(hex_string), 2)])

                    # 打印格式化后的十六进制字符串
                    print(f"\n收到数据: {formatted_hex_string}")
                    print()
                    print()
                else:
                    # 如果没有数据可读，延时避免高CPU占用
                    time.sleep(0.1)
            else:
                # 如果串口关闭，退出循环
                print("串口已关闭，读取线程退出")
                break
    except Exception as e:
        print(f"读取串口数据时出错: {e}")
    finally:
        # 确保线程正常退出时也会通知主线程
        stop_event.set()


def continuous_mode(ser):
    """持续收发模式，支持十六进制输入"""
    stop_event = threading.Event()  # 用于控制线程退出
    read_thread = threading.Thread(target=read_from_serial, args=(ser, stop_event))
    read_thread.daemon = True  # 设置为守护线程，这样主线程退出时，该线程也会自动退出
    read_thread.start()  # 启动串口读取线程

    try:
        print("进入持续收发模式 (按 Ctrl+C 退出)")
        while not stop_event.is_set():  # 检查停止事件，确保读取线程异常退出时主线程也能退出
            # 用户输入数据并发送（支持十六进制输入）
            user_input = input("输入要发送的 hex 数据 (按 Ctrl+C 退出): ").strip()

            if not ser.isOpen():
                print("串口已关闭，无法发送数据")
                break

            try:
                # 将用户输入的十六进制字符串转换为字节
                byte_data = bytes.fromhex(user_input)
                ser.write(byte_data)

                hex_string = byte_data.hex().upper()  # 转换为大写
                formatted_hex_string = ' '.join([hex_string[i:i + 2] for i in range(0, len(hex_string), 2)])

                print(f"发送数据: {formatted_hex_string}")

                time.sleep(0.3)
            except ValueError:
                print("无效的十六进制输入，请重新输入。")
    except KeyboardInterrupt:
        print("退出持续收发模式")
    finally:
        # 设置停止标志并等待接收线程退出
        stop_event.set()
        read_thread.join(timeout=1)  # 等待读取线程最多1秒
        if ser.isOpen():
            ser.close()
        print("串口已关闭")


def main():
    """主函数，解析命令行参数并根据参数执行相应功能。"""
    parser = argparse.ArgumentParser(description="UIO Serial Communication Tool")

    # 设备参数
    parser.add_argument('device', nargs='?', help='要连接的串口设备（如 /dev/ttyUSB0, COM1）')

    # 波特率
    parser.add_argument('-b', '--baudrate', type=int, default=9600, help='设置波特率 (默认: 9600)')

    # 奇偶校验
    parser.add_argument('-p', '--parity', choices=['none', 'odd', 'even', 'mark', 'space'], default='none',
                        help='设置奇偶校验模式 (默认: none)')

    # 数据位
    parser.add_argument('-d', '--databits', type=int, choices=[5, 6, 7, 8], default=8, help='设置数据位长度 (默认: 8)')

    # 停止位 - 修正了选项
    parser.add_argument('-s', '--stopbits', type=float, choices=[1, 1.5, 2], default=1, help='设置停止位 (默认: 1)')

    # 流控制
    parser.add_argument('-f', '--flowcontrol', choices=['none', 'software', 'hardware'], default='none',
                        help='设置流控制模式 (默认: none)')

    # 超时时间
    parser.add_argument('-t', '--timeout', type=float, default=1.0, help='设置超时时间 (单位: 秒)')

    # 列出串口设备
    parser.add_argument('-l', '--list', action='store_true', help='列出当前系统中所有可用的串口设备')

    # 持续收发模式 (raw 模式)
    parser.add_argument('-r', '--raw', action='store_true', help='进入持续收发模式')

    # 显示版本号
    parser.add_argument('-v', '--version', action='version', version=f'%(prog)s {VERSION}', help='显示版本信息')

    # 解析参数
    args = parser.parse_args()

    if args.list:
        list_serial_ports()
    elif args.device:
        try:
            ser = open_serial(
                device=args.device,
                baudrate=args.baudrate,
                parity=args.parity,
                databits=args.databits,
                stopbits=args.stopbits,
                flowcontrol=args.flowcontrol,
                timeout=args.timeout,
            )

            if ser and args.raw:
                continuous_mode(ser)
        except Exception as e:
            print(f"错误: {e}")
    else:
        parser.print_help()


if __name__ == "__main__":
    main()
