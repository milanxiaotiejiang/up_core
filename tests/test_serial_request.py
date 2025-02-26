import pytest
from fastapi.testclient import TestClient
from pup_core.app import app

client = TestClient(app)


# 测试打开串口接口
@pytest.fixture
def serial_id():
    # 打开串口并获取 serial_id
    request_data = {
        "device": "/dev/ttyUSB0",  # 假设传递设备路径
        "baudrate": 1000000,  # 设置波特率
        "parity": "none",  # 设置校验位
        "databits": 8,  # 数据位
        "stopbits": 1,  # 停止位
        "flowcontrol": "none",  # 流控
        "timeout": 1.0  # 超时
    }

    response = client.post("/serial/open", json=request_data)  # 使用 POST 请求并传递数据
    assert response.status_code == 200
    assert response.json()["status"] is True
    return response.json()["data"]  # 返回 serial_id


# 测试获取版本接口
def test_get_version(serial_id):
    # 模拟 request 数据
    request_data = {
        "serial_id": serial_id,
        "protocol_id": 1,  # 假设传递协议 ID
    }
    response = client.post("/serial/get_version", json=request_data)

    # 确认响应成功
    assert response.status_code == 200
    assert response.json()["status"] is True
    assert "data" in response.json()  # 确保响应中包含 data 字段


# 测试关闭串口接口
def test_close_serial(serial_id):
    request_data = {
        "serial_id": serial_id,
    }
    response = client.post("/serial/close", json=request_data)

    # 确认响应成功
    assert response.status_code == 200
    assert response.json()["status"] is True
