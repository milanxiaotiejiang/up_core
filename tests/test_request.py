import time
from pup_core.proto import Request
import json


def create_request(id: str, data: dict, data_type: str):
    """创建一个 Request 消息"""
    # 将字典转换为 JSON 字符串，并编码为二进制
    data_bytes = json.dumps(data).encode('utf-8')

    request = Request(
        id=id,
        data=data_bytes,  # 使用二进制数据
        timestamp=int(time.time()),  # 使用当前时间戳
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
        # 如果数据类型是 JSON，则将字节解码为 JSON 对象
        return json.loads(data_bytes.decode('utf-8'))
    else:
        return data_bytes.decode('utf-8')  # 默认为字符串处理


# 示例使用
data = {"status": "urgent", "details": "Please check your email for updates."}
request = create_request(
    id="12345",
    data=data,  # 数据作为字典传递
    data_type="json"  # 指定数据类型为 JSON
)

# 打印原始请求数据
print(f"Original Request:\n"
      f"ID: {request.id}\n"
      f"Data: {request.data}\n"
      f"Timestamp: {request.timestamp}\n"
      f"Data Type: {request.data_type}")

# 序列化请求对象
serialized_request = serialize_request(request)

# 反序列化请求对象
deserialized_request = deserialize_request(serialized_request)

# 打印反序列化后的请求数据
print(f"\nDeserialized Request:\n"
      f"ID: {deserialized_request.id}\n"
      f"Data: {deserialize_data(deserialized_request.data, deserialized_request.data_type)}\n"
      f"Timestamp: {deserialized_request.timestamp}\n"
      f"Data Type: {deserialized_request.data_type}")
