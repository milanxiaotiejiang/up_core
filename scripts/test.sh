#!/bin/bash

# 引入 source.sh 来定位到项目根目录
source ./scripts/source.sh

# 引入 build.sh
source ./scripts/build.sh

echo "Running tests..."
# Assuming the test executable is called add_tests and is in the build directory
if [ -f "build/add_tests" ]; then
    ./build/add_tests || { echo "Tests failed"; exit 1; }
else
    echo "Test executable does not exist."
    exit 1
fi

echo "Testing completed successfully."
