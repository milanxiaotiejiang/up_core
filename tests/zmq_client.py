import zmq

context = zmq.Context()
socket = context.socket(zmq.SUB)
socket.connect("tcp://localhost:5555")
socket.subscribe('')  # 订阅所有消息

print("正在监听 ZeroMQ 消息...")

while True:
    msg = socket.recv_json()
    print("收到消息:", msg)
