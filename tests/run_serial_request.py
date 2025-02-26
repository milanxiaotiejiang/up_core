import logging

import requests

# 定义 FastAPI 服务的 URL
BASE_URL = "http://localhost:8000"  # 这里假设服务运行在本地的 8000 端口


# 发送请求以打开串口
def open_serial():
    try:
        response = requests.post(f"{BASE_URL}/serial/open")
        response.raise_for_status()  # 确保响应状态是 200
        data = response.json()

        if data["status"]:
            print(f"Serial opened successfully. Serial ID: {data['data']}")
            return data["data"]  # 返回 serial_id
        else:
            print("Failed to open serial.")
            return None
    except requests.exceptions.RequestException as e:
        print(f"Error opening serial: {e}")
        return None


def close_serial(serial_id: str):
    try:
        # 构建请求数据
        request_data = {
            "serial_id": serial_id,
        }

        # 发送 POST 请求关闭串口
        response = requests.post(f"{BASE_URL}/serial/close", json=request_data)  # 使用 POST 请求

        # 确保响应状态是 200
        response.raise_for_status()

        # 解析返回的 JSON 数据
        data = response.json()

        if data["status"]:
            print(f"Serial closed successfully: {data['status']}")
        else:
            print(f"Failed to close serial. Error: {data['message']}")
    except requests.exceptions.RequestException as e:
        print(f"Error closing serial: {e}")


# 发送请求以获取版本
def get_version(serial_id, protocol_id):
    try:
        # 构建请求数据
        request_data = {
            "serial_id": serial_id,
            "protocol_id": protocol_id,
        }

        response = requests.post(f"{BASE_URL}/serial/get_version", json=request_data)
        response.raise_for_status()  # 确保响应状态是 200
        data = response.json()

        if data["status"]:
            print(f"Version data received: {data['data']}")
        else:
            print(f"Failed to get version. Error: {data['message']}")
    except requests.exceptions.RequestException as e:
        print(f"Error getting version: {e}")


# 主程序
def main():
    # 第一步：请求打开串口
    serial_id = open_serial()

    if serial_id:
        # 第二步：使用获取的 serial_id 请求获取版本
        get_version(serial_id, protocol_id=1)  # 假设协议 ID 为 1

        close_serial(serial_id)


if __name__ == "__main__":
    main()
