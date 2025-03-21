# FirmwareUpdate 类架构文档

## 类概述

`FirmwareUpdate` 类是一个专用于固件升级的实用工具类，负责管理嵌入式设备（主要是舵机）的固件更新流程。
该类封装了完整的固件升级过程，包括初始化通信、读取固件文件、数据传输以及验证等步骤。

## 主要职责

- 读取和解析固件二进制文件
- 与设备建立升级通信连接
- 将固件数据分包传输给目标设备
- 处理传输过程中的错误和重试
- 确保设备成功完成升级

## 核心方法

### `upgrade` 方法

这是类的主要入口方法，协调整个固件升级流程。

```cpp
bool upgrade(std::string port_input, int baud_rate, const std::string &bin_path,
             int total_retry, int handshake_threshold, int frame_retry_count, int sign_retry_count);
```

**参数说明：**

- `port_input`: 串口设备名称（如 "/dev/ttyUSB0"）
- `baud_rate`: 与设备通信的波特率
- `bin_path`: 固件文件路径
- `total_retry`: 整个升级流程的最大重试次数
- `handshake_threshold`: 握手成功的确认次数阈值
- `frame_retry_count`: 单个数据帧的最大重试次数
- `sign_retry_count`: 结束标志的最大重试次数

**工作流程：**

1. 保存配置参数到类成员变量
2. 读取并解析固件文件，分割成数据帧
3. 执行升级主流程（最多重试 `total_retry` 次）：
    - 通过 `bootloader()` 启动设备的 Bootloader 模式
    - 通过 `firmware_upgrade()` 建立握手连接
    - 通过 `firmwareUpdate()` 传输固件数据
    - 通过 `wave()` 发送结束标志
4. 返回升级结果（成功或失败）

## 辅助方法

### 通信和协议方法

- `bootloader(uint8_t id)`: 将设备重置到 Bootloader 模式，准备接收固件
- `firmware_upgrade()`: 与设备进行握手确认，建立固件升级连接
- `firmwareUpdate(std::vector<std::vector<uint8_t>> binArray)`: 按顺序发送所有固件数据帧
- `sendFrame(const std::vector<uint8_t> &frame)`: 发送单个数据帧并等待响应
- `wave()`: 发送结束标志，通知设备固件传输完成

### 数据处理方法

- `splitBinArray(const std::string &fileName)`: 读取固件文件并分割成数据帧
- `buildFrame(const std::vector<uint8_t> &data, int packetNumber, std::vector<uint8_t> &frame)`: 构建单个数据帧
- `calculateCRC(const std::vector<uint8_t> &data)`: 计算数据的 CRC 校验值
- `readFile(const std::string &fileName, std::vector<uint8_t> &buffer)`: 读取二进制文件内容

## 重要成员变量

- `port`: 串口设备名称
- `current_baud_rate`: 当前通信波特率
- `upgradeSerial`: 串口通信对象
- `handshake_count`: 握手成功阈值
- `fire_ware_frame_retry`: 数据帧重试次数
- `wave_sign_retry`: 结束标志重试次数

## 通信线程管理

类使用多线程处理通信，主要包括：

- 发送线程：负责发送握手请求和数据帧
- 接收线程：处理设备返回的响应
- 主线程：协调整体流程和等待操作结果

## 错误处理与重试机制

- 对整个升级流程提供最多 `total_retry` 次重试
- 对每个数据帧提供最多 `fire_ware_frame_retry` 次重试
- 对结束标志提供最多 `wave_sign_retry` 次重试
- 使用日志记录各阶段的操作结果和错误信息

## 主要方法流程

## FirmwareUpdate 类核心方法流程详解

### 1. `bootloader(uint8_t id)` 流程

**目的**：将设备重置到 Bootloader 模式，准备接收新固件。

**流程**：

1. 根据设备 ID 构建 ServoProtocol 协议对象
2. 使用当前波特率打开串口连接
3. 构建特殊的复位命令数据包
4. 清空串口输入缓冲区
5. 发送复位命令到设备
6. 等待设备响应
7. 处理响应结果
8. 关闭串口连接
9. 返回操作结果

### 2. `firmware_upgrade()` 流程

**目的**：与设备建立握手确认，切换到固件升级模式。

**流程**：

1. 以固件升级专用波特率（通常 9600）打开串口
2. 初始化通信状态变量
3. 创建并启动读取线程，用于接收设备响应：
    - 循环监听串口数据
    - 检查数据是否包含握手确认信号（0x43）
    - 计数成功的握手确认
    - 当达到指定次数时设置成功标志
4. 创建并启动写入线程，用于发送握手请求：
    - 循环发送握手请求信号（0x64）
    - 每发送一次等待短暂时间
    - 最多发送预定次数或直到收到停止信号
5. 主线程等待通过条件变量接收操作结果
6. 根据握手成功计数确定操作结果
7. 等待并终止读写线程
8. 返回握手结果

### 3. `firmwareUpdate(std::vector<std::vector<uint8_t>> binArray)` 流程

**目的**：按顺序发送所有固件数据帧到设备。

**流程**：

1. 记录开始时间（用于计算传输速度）
2. 初始化统计变量（总帧数、成功帧数、失败帧数）
3. 遍历所有数据帧：
    - 对每个数据帧，尝试发送最多 `fire_ware_frame_retry` 次
    - 调用 `sendFrame()` 发送单个数据帧
    - 根据返回结果更新统计信息
    - 如果连续失败次数过多，提前终止传输
4. 计算并记录传输速度和成功率
5. 返回整体传输是否成功（成功帧数/总帧数 >= 阈值）

### 4. `sendFrame(const std::vector<uint8_t> &frame)` 流程

**目的**：发送单个数据帧并等待响应。

**流程**：

1. 清空串口输入缓冲区
2. 发送数据帧到串口
3. 设置超时时间，等待设备响应
4. 读取设备返回的响应数据
5. 验证响应是否有效：
    - 检查响应长度
    - 验证响应中的状态字节是否表示成功
    - 检查握手确认信号
6. 返回发送结果（成功/失败）

### 5. `wave()` 流程

**目的**：发送结束标志，通知设备固件传输完成。

**流程**：

1. 打开串口连接（使用固件升级专用波特率）
2. 初始化尝试计数器
3. 循环尝试发送结束标志，最多 `wave_sign_retry` 次：
    - 构建结束命令数据包（特殊字节序列）
    - 清空串口输入缓冲区
    - 发送结束命令
    - 等待设备响应
    - 验证响应是否表示操作成功
    - 如果成功则跳出循环，否则短暂延时后重试
4. 关闭串口连接
5. 返回操作结果（是否成功发送结束标志）

这些方法共同构成了一个完整的固件升级流程，通过有序的步骤和可靠的错误处理机制，确保固件能够安全、稳定地传输到设备并正确应用。

## 使用示例

```cpp
FirmwareUpdate updater;
bool success = updater.upgrade(
    "/dev/ttyUSB0",   // 串口设备
    9600,             // 波特率
    "firmware.bin",   // 固件文件
    10,               // 整体重试次数
    5,                // 握手阈值
    5,                // 数据帧重试次数
    5                 // 结束标志重试次数
);
```
