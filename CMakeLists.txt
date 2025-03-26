message("")
message(STATUS "CMake version: ${CMAKE_VERSION}")
message(STATUS "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")

cmake_minimum_required(VERSION 3.10)
project(up_core)

# 设置 C++ 标准
#set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 启用 C 和 C++ 语言支持
enable_language(C CXX)

if (APPLE)
elseif (UNIX)
    set(Python3_EXECUTABLE /usr/bin/python3)
elseif (WIN32)
endif ()


# 查找 Python 和 pybind11
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

if (APPLE)
    set(pybind11_DIR "/usr/local/lib/cmake/pybind11")
elseif (UNIX)
    set(pybind11_DIR "/usr/lib/cmake/pybind11")
elseif (WIN32)
    set(pybind11_DIR "D:/Program Files/Python312/Lib/site-packages/pybind11/share/cmake/pybind11")
endif ()

find_package(pybind11 REQUIRED)

# 仅 Linux 需要 libgpiod
if (UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GPIOD REQUIRED libgpiod)
endif ()

# MacOS 需要 IOKit 和 Foundation
if (APPLE)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(FOUNDATION_LIBRARY Foundation)
endif ()

# 线程库（仅 Linux 需要）
set(rt_LIBRARIES rt)
set(pthread_LIBRARIES pthread)

# 输出 Python 和 pybind11 相关信息
message(STATUS "Python3_EXECUTABLE: ${Python3_EXECUTABLE}")
message(STATUS "Python3_INCLUDE_DIRS: ${Python3_INCLUDE_DIRS}")
message(STATUS "Python3_LIBRARIES: ${Python3_LIBRARIES}")
message(STATUS "Python3_VERSION: ${Python3_VERSION}")
message(STATUS "pybind11_INCLUDE_DIRS: ${pybind11_INCLUDE_DIRS}")

if (UNIX)
    message(STATUS "gpiod_INCLUDE_DIRS: ${GPIOD_INCLUDE_DIRS}")
    message(STATUS "gpiod_LIBRARIES: ${GPIOD_LIBRARIES}")
endif ()

# 头文件路径
include_directories(
        include
        ${Python3_INCLUDE_DIRS}
        ${pybind11_INCLUDE_DIRS}
)

if (UNIX)
    include_directories(${GPIOD_INCLUDE_DIRS})
endif ()

message(STATUS "Added include directories:")
message(STATUS " - include")
message(STATUS " - ${Python3_INCLUDE_DIRS}")

# 源码文件
set(up_core_SRCS
        src/add.cpp
        include/add.h
        src/serial.cc
        include/serial/serial.h
        include/serial/v8stdint.h
        src/servo_protocol.cpp
        include/servo_protocol.h
        src/servo.cpp
        include/servo.h
        src/logger.cpp
        include/logger.h
        include/servo_manager.h
        src/servo_manager.cpp
        include/firmware_update.h
        src/firmware_update.cpp
        include/servo_protocol_parse.h
        src/servo_protocol_parse.cpp
)

if (APPLE)
elseif (UNIX)
    list(APPEND up_core_SRCS
            src/gpio.cpp
            include/gpio.h
            src/spi.cpp
            include/spi.h
            src/adc.cpp
            include/adc.h
    )
elseif (WIN32)
endif ()

# 平台相关实现
if (APPLE)
    list(APPEND up_core_SRCS src/impl/unix.cc src/impl/list_ports/list_ports_osx.cc)
elseif (UNIX)
    list(APPEND up_core_SRCS src/impl/unix.cc src/impl/list_ports/list_ports_linux.cc)
elseif (WIN32)
    list(APPEND up_core_SRCS src/impl/win.cc src/impl/list_ports/list_ports_win.cc)
endif ()

# 构建静态库
add_library(up_core_base ${up_core_SRCS})

# 构建可执行文件
add_executable(up_core_main src/main.cpp ${up_core_SRCS})

# 链接库
if (APPLE)
    target_link_libraries(up_core_base ${FOUNDATION_LIBRARY} ${IOKIT_LIBRARY})
    target_link_libraries(up_core_main ${FOUNDATION_LIBRARY} ${IOKIT_LIBRARY})
elseif (UNIX)
    target_link_libraries(up_core_base rt pthread ${GPIOD_LIBRARIES})
    target_link_libraries(up_core_main rt pthread ${GPIOD_LIBRARIES})
elseif (WIN32)
    target_link_libraries(up_core_base setupapi)
    target_link_libraries(up_core_main setupapi)
endif ()

# 生成 Python 扩展模块
pybind11_add_module(up_core MODULE bind/wrapper.cpp ${up_core_SRCS})

if (APPLE)
    target_link_libraries(up_core PRIVATE ${FOUNDATION_LIBRARY} ${IOKIT_LIBRARY})
elseif (UNIX)
    target_link_libraries(up_core PRIVATE up_core_base rt pthread ${GPIOD_LIBRARIES})
elseif (WIN32)
    target_link_libraries(up_core PRIVATE up_core_base setupapi)
endif ()

# 设置 Python 扩展模块属性
set_target_properties(up_core PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

# 安装 Python 模块到 site-packages
if (WIN32)
else ()
    install(TARGETS up_core LIBRARY DESTINATION ${PYTHON_SITE_PACKAGES})
endif ()


# 查找 Google Test
if (WIN32)
    set(GTEST_ROOT "E:/workspace/vcpkg/installed/x64-windows")
    find_package(GTest REQUIRED PATHS ${GTEST_ROOT}/share/gtest)
elseif (UNIX)
    find_package(GTest REQUIRED)
endif ()

# 启用 Google Test
enable_testing()
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

message(STATUS "GTEST_INCLUDE_DIRS: ${GTEST_INCLUDE_DIRS}")
message(STATUS "GTEST_BOTH_LIBRARIES: ${GTEST_BOTH_LIBRARIES}")

# 测试
add_executable(serial_tests tests/test_add.cpp tests/test_servo_protocol.cpp)
target_link_libraries(serial_tests GTest::GTest GTest::Main up_core_base)

include(GoogleTest)
gtest_discover_tests(serial_tests)

message(STATUS "end of CMakeLists.txt")
