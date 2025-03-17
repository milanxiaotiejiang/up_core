# SerialManager 设计文档

## 概述

`SerialManager` 是一个用于管理多个串口通信的类，封装了串口的打开、关闭、读写等操作，并提供了异步支持。
它基于 `asyncio` 实现，能够高效地处理多个串口的并发通信任务。

本文档将重点介绍 `SerialManager` 的设计思路、核心功能以及如何与底层串口操作（如 `open_serial`、`write`、`read` 等）结合使用。

---

## 设计目标

1. **统一管理多个串口**：支持同时打开和管理多个串口设备。
2. **异步支持**：通过 `asyncio` 实现异步读写操作，避免阻塞主线程。
3. **线程安全**：使用线程锁（`threading.Lock`）确保多线程环境下的数据安全。
4. **易用性**：提供简洁的 API，方便开发者快速集成和使用。
5. **健壮性**：完善的异常处理和日志记录，确保程序的稳定性。

---

## 核心设计

### 1. **类结构**

`SerialManager` 的核心类包括：

- **`SerialPort`**：封装单个串口的操作，包括数据发送、接收、关闭等。
- **`SerialManager`**：管理多个 `SerialPort` 实例，提供统一的 API 供外部调用。

#### `SerialPort` 类

- **职责**：
    - 管理单个串口的打开、关闭、读写操作。
    - 通过 `asyncio.Queue` 实现数据的异步接收。
    - 通过 `asyncio.Future` 实现异步响应的等待。
- **核心属性**：
    - `ser`：底层串口对象。
    - `received_data_queue`：接收数据的队列。
    - `pending_response`：等待响应的 Future 对象。
    - `task`：数据接收任务。
- **核心方法**：
    - `send_command`：发送命令并等待响应。
    - `receive_data`：持续接收串口数据。
    - `close`：关闭串口并清理资源。

#### `SerialManager` 类

- **职责**：
    - 管理多个串口设备，提供统一的 API。
    - 确保线程安全（通过 `threading.Lock`）。
    - 封装底层串口操作（如 `open_serial`、`write`、`read`）。
- **核心属性**：
    - `serial_ports`：存储所有打开的串口设备（键为串口 ID，值为 `SerialPort` 实例）。
    - `_lock`：线程锁，确保多线程环境下的数据安全。
- **核心方法**：
    - `open`：打开串口并返回唯一的串口 ID。
    - `close`：关闭指定串口。
    - `write`：向串口写入数据。
    - `write_wait`：向串口写入数据并等待响应。

---

### 2. **与底层串口操作的结合**

`SerialManager` 依赖于底层串口操作函数（如 `open_serial`、`write`、`read` 等），这些函数封装了具体的串口配置和操作逻辑。

#### 底层函数说明

- **`open_serial`**：
    - 打开串口并配置参数（波特率、数据位、停止位等）。
    - 返回一个串口对象。
- **`is_open`**：
    - 检查串口是否已打开。
- **`close_serial`**：
    - 关闭串口。
- **`write`**：
    - 向串口写入数据。
- **`read`**：
    - 从串口读取数据。

#### 结合方式

- `SerialManager` 在 `open` 方法中调用 `open_serial` 打开串口，并创建 `SerialPort` 实例。
- `SerialPort` 在 `send_command` 方法中调用 `write` 发送数据。
- `SerialPort` 在 `receive_data` 方法中调用 `read` 接收数据。

---

### 3. **异步设计**

`SerialManager` 使用 `asyncio` 实现异步操作，主要包括：

- **异步发送和接收**：
    - `send_command` 方法支持异步发送命令并等待响应。
    - `receive_data` 方法持续接收数据，并通过 `asyncio.Queue` 和 `asyncio.Future` 实现异步通知。
- **任务管理**：
    - 每个 `SerialPort` 实例都有一个 `task` 属性，用于管理数据接收任务。
    - 在关闭串口时，任务会被取消。

---

### 4. **线程安全设计**

- **线程锁**：
    - `SerialManager` 使用 `threading.Lock` 确保多线程环境下的数据安全。
    - 在 `open` 和 `close` 方法中，使用锁保护 `serial_ports` 的访问。
- **异步锁**：
    - `SerialPort` 使用 `asyncio.Lock` 确保异步环境下的数据安全。
    - 在 `send_command` 方法中，使用锁保护 `pending_response` 的访问。

---

## 核心功能

### 1. **打开串口**

- **方法**：`open`
- **参数**：
    - `device`：串口设备路径（如 `/dev/ttyUSB0` 或 `COM1`）。
    - `baudrate`：波特率（默认 9600）。
    - `parity`：奇偶校验（`none`、`even`、`odd` 等）。
    - `databits`：数据位长度（5、6、7、8）。
    - `stopbits`：停止位（1、1.5、2）。
    - `flowcontrol`：流控制模式（`none`、`rtscts`、`xonxoff`）。
    - `timeout`：读写操作的超时时间（单位：秒）。
- **返回值**：唯一的串口 ID。
- **流程**：
    1. 检查串口是否已打开。
    2. 调用 `open_serial` 打开串口。
    3. 创建 `SerialPort` 实例并启动数据接收任务。
    4. 返回串口 ID。

### 2. **关闭串口**

- **方法**：`close`
- **参数**：
    - `serial_id`：串口 ID。
- **流程**：
    1. 查找对应的 `SerialPort` 实例。
    2. 调用 `SerialPort.close` 关闭串口并清理资源。
    3. 从 `serial_ports` 中移除该串口。

### 3. **写入数据**

- **方法**：`write`
- **参数**：
    - `serial_id`：串口 ID。
    - `data`：要发送的数据（字节数组）。
- **返回值**：是否成功写入。
- **流程**：
    1. 查找对应的 `SerialPort` 实例。
    2. 调用 `SerialPort.send_command` 发送数据。

### 4. **写入数据并等待响应**

- **方法**：`write_wait`
- **参数**：
    - `serial_id`：串口 ID。
    - `data`：要发送的数据（字节数组）。
- **返回值**：接收到的响应数据（字节数组），如果超时则返回 `None`。
- **流程**：
    1. 查找对应的 `SerialPort` 实例。
    2. 调用 `SerialPort.send_command` 发送数据并等待响应。

---

## 示例代码

```python
async def main():
    manager = SerialManager()
    serial_id = manager.open("/dev/ttyUSB0", baudrate=115200)

    try:
        # 写入数据
        await manager.write(serial_id, b"Hello, World!")

        # 写入数据并等待响应
        response = await manager.write_wait(serial_id, b"Ping")
        if response:
            print(f"Received response: {response}")
    finally:
        manager.close(serial_id)


if __name__ == "__main__":
    asyncio.run(main())
```

---

## 总结

`SerialManager` 通过封装底层串口操作、提供异步支持和线程安全机制，实现了高效、易用的串口管理功能。开发者可以通过简洁的 API
快速集成串口通信功能，并专注于业务逻辑的实现。