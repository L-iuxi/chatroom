cmake_minimum_required(VERSION 3.10)
project(NetworkApp)  # 项目名称

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 启用调试符号（默认Debug模式）
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# 包含头文件目录
include_directories(
    ${PROJECT_SOURCE_DIR}/client
    ${PROJECT_SOURCE_DIR}/server
    ${PROJECT_SOURCE_DIR}/other
    /path/to/hiredis/include 
)

# 添加可执行文件
add_executable(Client
    client/client.cc
    client/client.hpp
    client/cfri.cpp
    client/cfri.hpp
    client/clientmain.cpp
    client/cgro.cpp
    client/cgro.hpp
    other/ui.hpp
    other/ui.cpp
)

add_executable(Server
    server/server.cpp
    server/server.hpp
    server/servermain.cc
    server/redis.hpp
    server/redis.cpp
    server/fri.cpp
    server/fri.hpp
    server/gro.cpp
    server/gro.hpp
    other/threadpool.hpp
)

# 链接系统库（如线程库）
find_package(Threads REQUIRED)
target_link_libraries(Client PRIVATE Threads::Threads)
target_link_libraries(Server PRIVATE Threads::Threads)

# 查找hiredis库
find_library(HIREDIS_LIB NAMES hiredis PATHS /usr/lib/x86_64-linux-gnu)
# 如果hiredis安装在这个路径下
target_link_libraries(Server PRIVATE ${HIREDIS_LIB})

# Windows特定配置
if(WIN32)
    target_link_libraries(Client ws2_32)
    target_link_libraries(Server ws2_32)
endif()

# 可选：安装目标到系统路径
install(TARGETS Client Server DESTINATION bin)
