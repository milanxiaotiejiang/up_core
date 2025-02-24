#!/bin/bash

# 检查虚拟环境是否已存在
if [ ! -d "venv" ]; then
    # 创建虚拟环境
    python3.8 -m venv venv
    echo "Virtual environment created."
else
    echo "Virtual environment already exists."
fi

# 激活虚拟环境
source venv/bin/activate

# 更新 pip
pip install --upgrade pip

# 卸载 up_core
pip uninstall up_core -y

# 安装依赖
pip install pybind11 wheel setuptools
