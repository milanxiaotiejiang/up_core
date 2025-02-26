import time
import json
from pup_core.proto import Response


def create_response(id: str, data: dict, data_type: str, error_code: str = ""):
    """创建一个 Response 消息"""
    data_bytes = json.dumps(data).encode('utf-8')
    response = Response(
        id=id,
        data=data_bytes,
        timestamp=int(time.time()),  # 使用当前时间戳
        data_type=data_type,
        error_code=error_code
    )
    return response


def serialize_response(response):
    """将 Response 对象序列化为二进制格式"""
    return response.SerializeToString()


def deserialize_response(serialized_data):
    """将二进制数据反序列化为 Response 对象"""
    response = Response()
    response.ParseFromString(serialized_data)
    return response


def deserialize_data(data_bytes, data_type):
    """根据数据类型反序列化 data 字段"""
    if data_type == "json":
        return json.loads(data_bytes.decode('utf-8'))
    else:
        return data_bytes.decode('utf-8')


# 测试函数：创建和序列化/反序列化响应
def test_create_serialize_deserialize_response():
    data = {"status": "success", "details": "Your operation was successful."}
    response = create_response(
        id="67890",
        data=data,
        data_type="json",
        error_code=""
    )

    # 序列化响应对象
    serialized_response = serialize_response(response)

    # 反序列化响应对象
    deserialized_response = deserialize_response(serialized_response)

    # 检查反序列化后的响应数据是否与原始数据一致
    assert deserialized_response.id == response.id
    assert deserialized_response.timestamp == response.timestamp
    assert deserialized_response.data_type == response.data_type
    assert deserialized_response.error_code == response.error_code

    # 检查反序列化后的 data 字段是否正确
    deserialized_data = deserialize_data(deserialized_response.data, deserialized_response.data_type)
    assert deserialized_data == data


# 测试函数：验证序列化和反序列化的二进制数据
def test_serialize_deserialize_response_data():
    data = {"status": "success", "details": "Your operation was successful."}
    response = create_response(
        id="67890",
        data=data,
        data_type="json",
        error_code=""
    )

    # 序列化响应对象
    serialized_response = serialize_response(response)

    # 反序列化响应对象
    deserialized_response = deserialize_response(serialized_response)

    # 反序列化后的 data 字段
    deserialized_data = deserialize_data(deserialized_response.data, deserialized_response.data_type)

    # 验证反序列化数据是否正确
    assert deserialized_data == data
