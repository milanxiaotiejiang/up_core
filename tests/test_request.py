import time
import json
from pup_core.proto import Request
import pytest


def create_request(id: str, data: dict, data_type: str):
    """创建一个 Request 消息"""
    data_bytes = json.dumps(data).encode('utf-8')

    request = Request(
        id=id,
        data=data_bytes,
        timestamp=int(time.time()),
        data_type=data_type
    )
    return request


def serialize_request(request):
    """将 Request 对象序列化为二进制格式"""
    return request.SerializeToString()


def deserialize_request(serialized_data):
    """将二进制数据反序列化为 Request 对象"""
    request = Request()
    request.ParseFromString(serialized_data)
    return request


def deserialize_data(data_bytes, data_type):
    """根据数据类型反序列化 data 字段"""
    if data_type == "json":
        return json.loads(data_bytes.decode('utf-8'))
    else:
        return data_bytes.decode('utf-8')


# Test functions
def test_create_request():
    data = {"status": "urgent", "details": "Please check your email for updates."}
    request = create_request(
        id="12345",
        data=data,
        data_type="json"
    )
    assert request.id == "12345"
    assert request.data == json.dumps(data).encode('utf-8')  # Ensure data is in bytes form
    assert isinstance(request.timestamp, int)  # Ensure timestamp is an integer
    assert request.data_type == "json"


def test_serialize_request():
    data = {"status": "urgent", "details": "Please check your email for updates."}
    request = create_request(
        id="12345",
        data=data,
        data_type="json"
    )
    serialized_data = serialize_request(request)
    assert isinstance(serialized_data, bytes)  # Ensure serialized data is in bytes


def test_deserialize_request():
    data = {"status": "urgent", "details": "Please check your email for updates."}
    request = create_request(
        id="12345",
        data=data,
        data_type="json"
    )
    serialized_data = serialize_request(request)
    deserialized_request = deserialize_request(serialized_data)

    assert deserialized_request.id == request.id
    assert deserialized_request.data == request.data
    assert deserialized_request.timestamp == request.timestamp
    assert deserialized_request.data_type == request.data_type


def test_deserialize_data_json():
    data = {"status": "urgent", "details": "Please check your email for updates."}
    data_bytes = json.dumps(data).encode('utf-8')
    deserialized_data = deserialize_data(data_bytes, "json")
    assert deserialized_data == data  # Ensure the deserialized data matches the original data


def test_deserialize_data_string():
    data_str = "Hello, world!"
    deserialized_data = deserialize_data(data_str.encode('utf-8'), "string")
    assert deserialized_data == data_str  # Ensure the deserialized data matches the original string
