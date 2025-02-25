import time
from pup_core.proto import Response
import json


def create_response(id: str, data: dict, data_type: str, error_code: str = ""):
    """创建一个 Response 消息"""
    # 将字典转换为 JSON 字符串，并编码为二进制
    data_bytes = json.dumps(data).encode('utf-8')

    response = Response(
        id=id,
        data=data_bytes,  # 使用二进制数据
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
        # 如果数据类型是 JSON，则将字节解码为 JSON 对象
        return json.loads(data_bytes.decode('utf-8'))
    else:
        return data_bytes.decode('utf-8')  # 默认为字符串处理


# 示例使用
data = {"status": "success", "details": "Your operation was successful."}
response = create_response(
    id="67890",
    data=data,  # 数据作为字典传递
    data_type="json",  # 指定数据类型为 JSON
    error_code=""  # 没有错误代码
)

# 打印原始响应数据
print(f"Original Response:\n"
      f"ID: {response.id}\n"
      f"Data: {response.data}\n"
      f"Timestamp: {response.timestamp}\n"
      f"Data Type: {response.data_type}\n"
      f"Error Code: {response.error_code}")

# 序列化响应对象
serialized_response = serialize_response(response)

# 反序列化响应对象
deserialized_response = deserialize_response(serialized_response)

# 打印反序列化后的响应数据
print(f"\nDeserialized Response:\n"
      f"ID: {deserialized_response.id}\n"
      f"Data: {deserialize_data(deserialized_response.data, deserialized_response.data_type)}\n"
      f"Timestamp: {deserialized_response.timestamp}\n"
      f"Data Type: {deserialized_response.data_type}\n"
      f"Error Code: {deserialized_response.error_code}")
