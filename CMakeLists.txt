cmake_minimum_required(VERSION 2.8.3)
project(up_core)

# Find catkin
find_package(catkin REQUIRED)

if (APPLE)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(FOUNDATION_LIBRARY Foundation)
endif ()

if (UNIX AND NOT APPLE)
    # If Linux, add rt and pthread
    set(rt_LIBRARIES rt)
    set(pthread_LIBRARIES pthread)
    catkin_package(
            LIBRARIES ${PROJECT_NAME}
            INCLUDE_DIRS include
            DEPENDS rt pthread
    )
else ()
    # Otherwise normal call
    catkin_package(
            LIBRARIES ${PROJECT_NAME}
            INCLUDE_DIRS include
    )
endif ()

## Sources
set(up_core_SRCS
        src/serial.cc
        include/serial/serial.h
        include/serial/v8stdint.h
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

## Add serial library
add_library(${PROJECT_NAME} ${up_core_SRCS})
if (APPLE)
    target_link_libraries(${PROJECT_NAME} ${FOUNDATION_LIBRARY} ${IOKIT_LIBRARY})
elseif (UNIX)
    target_link_libraries(${PROJECT_NAME} rt pthread)
else ()
    target_link_libraries(${PROJECT_NAME} setupapi)
endif ()

## Uncomment for example
add_executable(up_core_example examples/serial_example.cc)
add_dependencies(up_core_example ${PROJECT_NAME})
target_link_libraries(up_core_example ${PROJECT_NAME})

## Include headers
include_directories(include)

## Install executable
install(TARGETS ${PROJECT_NAME}
        ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        RUNTIME DESTINATION ${CATKIN_GLOBAL_BIN_DESTINATION}
)

## Install headers
install(FILES include/serial/serial.h include/serial/v8stdint.h
        DESTINATION ${CATKIN_GLOBAL_INCLUDE_DESTINATION}/serial)

## Tests
if (CATKIN_ENABLE_TESTING)
    add_subdirectory(tests)
endif ()
