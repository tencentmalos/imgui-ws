cmake_minimum_required(VERSION 3.10)
project(RemoteImGui)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message(STATUS "Building Remote ImGui with modular client/server structure")

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Add libevent once at top level to avoid duplicate targets
add_subdirectory(../../../imgui-ws/third-party/libevent/tnt libevent_build)

# Build remote_debugger as a static library first
add_subdirectory(remote_debugger)

# 添加子目录
add_subdirectory(server)
add_subdirectory(client)

message(STATUS "=== Remote ImGui Build Configuration Complete ===")