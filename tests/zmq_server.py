# ZeroMQ 相关设置
import logging
import time

import zmq

context = zmq.Context()
socket = context.socket(zmq.PUB)
socket.bind("tcp://*:5555")  # 发布到端口 5555

try:
    # 通过 Ctrl+C 停止搜索
    while True:
        time.sleep(1)  # 在此处加上短暂休眠，避免 CPU 高负载

        # 发送消息之前打印日志
        logging.info("正在发送消息...")

        # 测试用，发送模拟的舵机数据
        socket.send_json({"baud": 9600, "servo_id": 1, "error_code": 0})

        # 消息发送后打印日志
        logging.info("消息已发送: 波特率 9600, 舵机 ID 1, 错误码 0")

except KeyboardInterrupt:
    # Ctrl+C 中断信号处理
    logging.info("搜索中断")
