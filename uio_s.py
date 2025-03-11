import argparse
import queue
import time
import logging
import sys
import zmq
import signal

from up_core import ServoManager

VERSION = "1.0.0"

# ZeroMQ 相关设置
context = zmq.Context()
socket = context.socket(zmq.PUB)
socket.bind("tcp://*:5555")  # 发布到端口 5555

data_queue = queue.Queue()

logging.basicConfig(level=logging.INFO)


# 中断信号处理函数
def signal_handler(signal_number, frame):
    logging.info("收到中断信号，正在关闭进程...")

    logging.info("\n中断搜索...")
    ServoManager.instance().stopSearchServoID()

    time.sleep(1)

    sys.exit(0)  # 正常退出程序


# 启动uio search功能
def search(device, baudrates):
    servo_manager = ServoManager.instance()

    if servo_manager.searching():
        print("搜索已经在进行中...")
        return

    # 设置回调函数
    def callback(baud, servo_id, error_code):
        data = (baud, servo_id, error_code)
        data_queue.put(data)  # 将回调数据放入队列中

    # 设置回调函数
    servo_manager.setCallback(callback)

    # 启动搜索线程
    servo_manager.startSearchServoID(device, baudrates)

    logging.info(f"开始在设备 {device} 上搜索舵机，波特率: {', '.join(map(str, baudrates))}...")

    try:
        # 通过 Ctrl+C 停止搜索
        while True:
            # 检查队列中是否有回调数据
            try:
                data = data_queue.get_nowait()  # 非阻塞读取
                baud, servo_id, error_code = data
                logging.info(f"波特率: {baud}, 舵机 ID: {servo_id}, 错误码: {error_code}")

                # 通过 ZeroMQ 发布消息
                socket.send_json({"baud": baud, "servo_id": servo_id, "error_code": error_code})
            except queue.Empty:
                # 如果队列为空，继续等待
                time.sleep(1)
    except KeyboardInterrupt:
        # Ctrl+C 中断信号处理
        logging.info("搜索中断")
        servo_manager.stopSearchServoID()


def main():
    """主函数，解析命令行参数并根据参数执行相应功能。"""
    parser = argparse.ArgumentParser(description="Us Search Servo ID Tool")

    # 设备参数
    parser.add_argument('device', nargs='?', help='要连接的串口设备（如 /dev/ttyUSB0, COM1）')

    # 显示版本号
    parser.add_argument('-v', '--version', action='version', version=f'%(prog)s {VERSION}', help='显示版本信息')

    # up search 模式
    parser.add_argument('--search', action='store_true', help='启动us搜索模式')

    # 波特率（支持多个波特率）
    parser.add_argument('-bs', '--baudrates', type=str, default="9600",
                        help='设置波特率（支持多个波特率，逗号分隔，默认: 9600）')

    # 解析参数
    args = parser.parse_args()

    if args.search and args.device:
        # 处理多个波特率，逗号分隔
        baudrates = [int(baudrate) for baudrate in args.baudrates.split(',')]
        search(args.device, baudrates)
    else:
        parser.print_help()


if __name__ == "__main__":
    # 注册信号处理
    signal.signal(signal.SIGINT, signal_handler)  # 捕捉 Ctrl+C
    signal.signal(signal.SIGTERM, signal_handler)  # 捕捉 SIGTERM

    main()
