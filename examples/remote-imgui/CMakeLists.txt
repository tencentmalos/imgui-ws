cmake_minimum_required(VERSION 3.10)
project(RemoteImGui)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message(STATUS "Building Remote ImGui with modular client/server structure")

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 添加子目录
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/server/CMakeLists.txt")
    add_subdirectory(server)
    message(STATUS "Added server subdirectory")
else()
    message(FATAL_ERROR "Server CMakeLists.txt not found!")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/client/CMakeLists.txt")
    add_subdirectory(client)
    message(STATUS "Added client subdirectory")
else()
    message(FATAL_ERROR "Client CMakeLists.txt not found!")
endif()

message(STATUS "=== Remote ImGui Build Configuration Complete ===")