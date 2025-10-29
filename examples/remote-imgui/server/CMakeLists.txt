cmake_minimum_required(VERSION 3.10)
project(RemoteImGuiServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../third-party/imgui/imgui
)

# ImGui 源文件
set(IMGUI_SOURCES
    ../../../third-party/imgui/imgui/imgui.cpp
    ../../../third-party/imgui/imgui/imgui_demo.cpp
    ../../../third-party/imgui/imgui/imgui_draw.cpp
    ../../../third-party/imgui/imgui/imgui_tables.cpp
    ../../../third-party/imgui/imgui/imgui_widgets.cpp
)

# set(EVENT__DISABLE_SAMPLES ON CACHE BOOL "Disable libevent samples")
# set(EVENT__DISABLE_TESTS ON CACHE BOOL "Disable libevent tests")
# set(EVENT__DISABLE_OPENSSL ON CACHE BOOL "Disable OpenSSL support")
# set(EVENT__DISABLE_THREAD_SUPPORT ON CACHE BOOL "Disable thread support for minimal build")
# set(EVENT__DISABLE_MM_REPLACEMENT ON CACHE BOOL "Disable memory management replacement")
# set(EVENT__DISABLE_DEBUG_MODE ON CACHE BOOL "Disable debug mode")

# libevent is added at parent level to avoid duplicate targets

add_executable(remote-imgui-server
    main.cpp
    ../serializer.cpp
    ../protocol.h
    ../serializer.h
    ../common.h
    ../network_server.h
    ../network_server.cpp
    ../network_client.h
    ../network_client.cpp
    ../deserializer.h
    ../deserializer.cpp
    ${IMGUI_SOURCES}
)

# 链接库
target_link_libraries(remote-imgui-server
    event_core
    event
    # event_extra  # libevent extra libraries for networking
)

if(WIN32)
    target_link_libraries(remote-imgui-server
        ws2_32
        wsock32
    )
endif()

# # 编译选项
# if(MSVC)
#     target_compile_options(remote-imgui-server PRIVATE /W4)
#     target_compile_definitions(remote-imgui-server PRIVATE
#         _CRT_SECURE_NO_WARNINGS
#         WIN32_LEAN_AND_MEAN
#         NOMINMAX
#         EVENT__DISABLE_THREAD_SUPPORT
#     )
# else()
#     target_compile_options(remote-imgui-server PRIVATE -Wall -Wextra -pedantic)
#     target_compile_definitions(remote-imgui-server PRIVATE
#         EVENT__DISABLE_THREAD_SUPPORT
#     )
# endif()

# 安装
install(TARGETS remote-imgui-server
    RUNTIME DESTINATION bin
)

message(STATUS "Remote ImGui Server configuration completed")

