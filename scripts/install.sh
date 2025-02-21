#!/bin/bash

# 引入 source.sh 来定位到项目根目录
source ./scripts/source.sh

# 引入 build.sh
source ./scripts/build.sh

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

# 安装 pybind11 和 wheel
pip install pybind11 wheel

echo "Installing the project..."

# 查找生成的 .so 文件
so_file=$(find build -type f -name "up_core*.so")

# 拷贝 .so 文件到虚拟环境的 lib 目录中
if [ -n "$so_file" ]; then
    cp "$so_file" "venv/lib/python3.8/site-packages/"
    echo "Copied $so_file to venv/lib/python3.8/site-packages/"
else
    echo "No .so file found."
fi

echo "==================================================== start =============================================================="
echo "Building the wheel distribution package..."

# 构建 wheel 分发包
python setup.py bdist_wheel

# 使用 setup_simple.py 安装这个包
python setup.py install

echo "Installation completed successfully."

# 退出虚拟环境
deactivate

echo "==================================================== end =============================================================="

# 引入 clean.sh
source ./scripts/clean.sh