message("")
message(STATUS "CMake version: ${CMAKE_VERSION}")
message(STATUS "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")

cmake_minimum_required(VERSION 2.8.3)
project(up_core)

#set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置使用 C 和 C++ 混编
enable_language(C CXX)

# 查找 Python 解释器和 pybind11
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
set(pybind11_DIR /usr/lib/cmake/pybind11)  # 这里确保使用官方 apt 版
find_package(pybind11 REQUIRED)

# 查找 gpiod 库
find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD REQUIRED libgpiod)

if (APPLE)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(FOUNDATION_LIBRARY Foundation)
endif ()

set(rt_LIBRARIES rt)
set(pthread_LIBRARIES pthread)

message(STATUS "Python3_EXECUTABLE: ${Python3_EXECUTABLE}")
message(STATUS "Python3_INCLUDE_DIRS: ${Python3_INCLUDE_DIRS}")
message(STATUS "Python3_LIBRARIES: ${Python3_LIBRARIES}")
message(STATUS "Python3_VERSION: ${Python3_VERSION}")
message(STATUS "pybind11_INCLUDE_DIRS: ${pybind11_INCLUDE_DIRS}")
message(STATUS "gpiod_INCLUDE_DIRS: ${GPIOD_INCLUDE_DIRS}")
message(STATUS "gpiod_LIBRARIES: ${GPIOD_LIBRARIES}")

## Include headers
include_directories(
        include  # 添加通用的头文件目录
        ${Python3_INCLUDE_DIRS}
        ${GPIOD_INCLUDE_DIRS}
)
message(STATUS "Added include directories:")
message(STATUS " - include")
message(STATUS " - ${Python3_INCLUDE_DIRS}")

## Sources
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
        src/gpio.cpp
        include/gpio.h
        src/spi.cpp
        include/spi.h
        src/adc.cpp
        include/adc.h
        src/logger.cpp
        include/logger.h
        include/servo_manager.h
        src/servo_manager.cpp
)

if (APPLE)
    # If OSX
    list(APPEND up_core_SRCS src/impl/unix.cc)
    list(APPEND up_core_SRCS src/impl/list_ports/list_ports_osx.cc)
elseif (UNIX)
    # If unix
    list(APPEND up_core_SRCS src/impl/unix.cc)
    list(APPEND up_core_SRCS src/impl/list_ports/list_ports_linux.cc)
else ()
    # If windows
    list(APPEND up_core_SRCS src/impl/win.cc)
    list(APPEND up_core_SRCS src/impl/list_ports/list_ports_win.cc)
endif ()

add_library(up_core_base
        ${up_core_SRCS}
)

add_executable(up_core_main
        src/main.cpp
        ${up_core_SRCS}
)

if (APPLE)
    target_link_libraries(up_core_base ${FOUNDATION_LIBRARY} ${IOKIT_LIBRARY})
    target_link_libraries(up_core_main ${FOUNDATION_LIBRARY} ${IOKIT_LIBRARY})
elseif (UNIX)
    target_link_libraries(up_core_base rt pthread ${GPIOD_LIBRARIES})
    target_link_libraries(up_core_main rt pthread ${GPIOD_LIBRARIES})
else ()
    target_link_libraries(up_core_base setupapi)
    target_link_libraries(up_core_main setupapi)
endif ()

# 添加 Python 模块
pybind11_add_module(up_core bind/wrapper.cpp ${up_core_SRCS})

if (APPLE)
    target_link_libraries(up_core ${FOUNDATION_LIBRARY} ${IOKIT_LIBRARY})
elseif (UNIX)
    target_link_libraries(up_core up_core_base rt pthread ${GPIOD_LIBRARIES})
else ()
    target_link_libraries(up_core setupapi)
endif ()

# 指定 C++ 标准
set_target_properties(up_core PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

# 安装模块到 Python site-packages
install(TARGETS up_core
        LIBRARY DESTINATION ${PYTHON_SITE_PACKAGES})

# 检测 Google Test
enable_testing()
find_package(GTest REQUIRED)

include_directories(${GTEST_INCLUDE_DIRS})

message(STATUS "GTEST_INCLUDE_DIRS:" ${GTEST_INCLUDE_DIRS})
message(STATUS "GTEST_BOTH_LIBRARIES:" ${GTEST_BOTH_LIBRARIES})

## Tests
# 创建一个测试可执行文件
add_executable(serial_tests tests/test_add.cpp tests/test_servo_protocol.cpp)
target_link_libraries(serial_tests GTest::GTest GTest::Main up_core_base)

# 添加测试到 CTest
include(GoogleTest)
gtest_discover_tests(serial_tests)

message(STATUS "end of CMakeLists.txt")
message("")
