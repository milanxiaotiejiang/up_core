# test_serial_api.py
import pytest
from fastapi.testclient import TestClient
from pup_core.app import app

client = TestClient(app)


# 测试打开串口接口
@pytest.fixture
def serial_id():
    # 打开串口并获取 serial_id
    response = client.get("/serial/open")
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
