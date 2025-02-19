cmake_minimum_required(VERSION 2.8.3)
project(up_core)

# 设置使用 C 和 C++ 混编
enable_language(C CXX)

# 查找 Python 解释器和 pybind11
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 CONFIG REQUIRED)

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
include_directories(include  # 添加通用的头文件目录
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
        src/main.cpp
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

add_library(${PROJECT_NAME} ${up_core_SRCS})

set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

# 添加 Python 模块
pybind11_add_module(${PROJECT_NAME} bind/wrapper.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME})

if (APPLE)
    target_link_libraries(${PROJECT_NAME} ${FOUNDATION_LIBRARY} ${IOKIT_LIBRARY})
elseif (UNIX)
    target_link_libraries(${PROJECT_NAME} rt pthread ${GPIOD_LIBRARIES})
else ()
    target_link_libraries(${PROJECT_NAME} setupapi)
endif ()

# 指定 C++ 标准
set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

# 检测 Google Test
enable_testing()
find_package(GTest REQUIRED)

include_directories(${GTEST_INCLUDE_DIRS})

message(STATUS "GTEST_INCLUDE_DIRS:" ${GTEST_INCLUDE_DIRS})
message(STATUS "GTEST_BOTH_LIBRARIES:" ${GTEST_BOTH_LIBRARIES})

## Tests
# 创建一个测试可执行文件
add_executable(serial_tests tests/test_serial.cpp)
target_link_libraries(serial_tests GTest::GTest GTest::Main ${PROJECT_NAME})
gtest_discover_tests(test_serial)
