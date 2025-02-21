#!/bin/bash

# 引入 source.sh 来定位到项目根目录
source ./scripts/source.sh

# Set script to exit on any errors.
set -e

# 检查构建目录是否存在
if [ -d "build" ]; then
    # 删除旧的构建目录
    echo "Removing old build directory..."
    rm -rf build
    echo "Old build directory removed."
fi

echo "Creating a new build directory..."
# 创建一个新的构建目录
mkdir -p build
cd build

echo "Generating build system with CMake..."
# 使用 CMake 生成构建系统
cmake .. || { echo "CMake generation failed"; exit 1; }

echo "Compiling the project..."
# 编译项目
make || { echo "Make failed"; exit 1; }

echo "Build completed successfully."

# 切换回项目根目录
cd ..
