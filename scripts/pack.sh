#!/bin/bash

echo "Installing the project..."

# 查找生成的 .so 文件
so_file=$(find build -type f -name "up_tech*.so")

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

echo "Installation completed successfully."

# 退出虚拟环境
deactivate

echo "==================================================== end =============================================================="

# 检查构建目录是否存在
if [ -d "_output" ]; then
    # 删除旧的构建目录
    echo "Removing old _output directory..."
    rm -rf _output
    echo "Old _output directory removed."
fi

mkdir -p _output

cp dist/*.whl _output/

# 引入 clean.sh
source ./scripts/clean.sh