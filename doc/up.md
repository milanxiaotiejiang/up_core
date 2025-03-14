## up 相关

### 概述

UP 是一个用于构建和管理项目的命令行工具，支持构建 C++ 共享库和 Python 包，并提供清理构建目录的功能。

### 功能特点

- 构建 C++ 共享库 (.so 文件)
- 构建 Python 包 (.whl 文件)
- 清理构建目录和 Python 包目录

### 安装要求

- Python 3.7+
- CMake
- Make

### 基本使用方法

#### 构建 C++ 共享库

```shell
up build so
```

这将生成 C++ 共享库 (.so 文件)。

#### 构建 Python 包

```shell
up build whl
```

这将生成 Python 包 (.whl 文件)。

#### 清理构建目录

```shell
up clean
```

这将清理构建目录和 Python 包目录。

### 完整的命令行选项

```shell
up [命令] [选项]
```

命令说明：

- `build`：构建项目。
    - `so`：构建 C++ 共享库。
    - `whl`：构建 Python 包。
- `clean`：清理构建目录和 Python 包目录。

### 示例

#### 构建 C++ 共享库

```shell
up build so
```

#### 构建 Python 包

```shell
up build whl
```

#### 清理构建目录

```shell
up clean
```

### 注意事项

- 确保已安装 CMake 和 Make 工具。
- 构建前请确保所有依赖项已正确安装。