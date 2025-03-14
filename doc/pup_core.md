## pup_core 相关

### 概述

PUP Core 是一个基于 FastAPI 的核心库，提供了串口通信和 WebSocket 通信的功能。该库支持全局异常处理和跨域请求，并包含多个
HTTP 和 WebSocket 路由。

### 功能特点

- 提供串口通信的 HTTP 接口
- 提供 WebSocket 通信接口
- 全局异常处理
- 支持跨域请求

### 安装要求

- Python 3.7+
- FastAPI
- Uvicorn
- `up_core` 库

### 基本使用方法

#### 启动 PUP Core 服务器

```shell
pup_core --host 0.0.0.0 --port 8000 --reload --log-level info
```

这将启动 PUP Core 服务器，监听 `0.0.0.0` 地址的 `8000` 端口，并启用自动重载和信息级别的日志。

#### 完整的命令行选项

```shell
pup_core [选项]
```

选项说明：

- `--host, -h`：设置服务器运行的主机地址（默认: `0.0.0.0`）
- `--port, -p`：设置服务器运行的端口（默认: `8000`）
- `--reload, -r`：启用开发模式下的自动重载
- `--log-level, -l`：设置日志级别（可选值: `debug`, `info`, `warning`, `error`）

### 示例

#### 启动服务器并启用自动重载

```shell
pup_core --host 127.0.0.1 --port 8080 --reload --log-level debug
```

#### 启动服务器并设置日志级别为错误

```shell
pup_core --host 0.0.0.0 --port 8000 --log-level error
```

### 错误处理

- 如果在请求处理过程中发生未捕获的异常，服务器会记录详细的错误日志并返回统一的错误响应。
- 全局异常处理器会捕获所有未处理的异常，并返回 HTTP 500 错误响应。

### 注意事项

- 确保已安装 FastAPI 和 Uvicorn。
- 启动服务器前，请确保所有依赖项已正确安装。
- 在生产环境中运行时，请禁用自动重载功能。