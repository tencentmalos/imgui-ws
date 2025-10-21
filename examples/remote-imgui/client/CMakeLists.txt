cmake_minimum_required(VERSION 3.10)
project(RemoteImGuiClient)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 参考 ImGui example_glfw_opengl3 的 Makefile 结构
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../third-party/imgui/imgui)
set(IMGUI_BACKEND_DIR ${IMGUI_DIR}/backends)

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${IMGUI_DIR}
    ${IMGUI_BACKEND_DIR}
)

# ImGui 源文件 - 参考 example_glfw_opengl3/Makefile
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
)

# ImGui 后端源文件
set(IMGUI_BACKEND_SOURCES
    ${IMGUI_BACKEND_DIR}/imgui_impl_glfw.cpp
    ${IMGUI_BACKEND_DIR}/imgui_impl_opengl3.cpp
)

# 尝试构建客户端 - 忽略libevent依赖，先确保基本的ImGui渲染能工作
message(STATUS "Building client without libevent dependency for testing")

# 简化版本的客户端 - 仅使用ImGui核心库
add_executable(remote-imgui-client-simple
    simple_client.cpp
    ${IMGUI_SOURCES}
)

# 简单测试程序
add_executable(simple-test
    ../simple_test.cpp
    ${IMGUI_SOURCES}
)

# 基础库 - 简化版本不需要OpenGL链接
# target_link_libraries(remote-imgui-client-simple)

# Windows 额外库 - 仅基础系统库
if(WIN32)
    target_link_libraries(remote-imgui-client-simple
        kernel32
    )
    target_compile_definitions(remote-imgui-client-simple PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
        IMGUI_IMPL_OPENGL_LOADER_CUSTOM  # 自定义OpenGL加载器（不使用）
    )

    target_link_libraries(simple-test
        kernel32
    )
    target_compile_definitions(simple-test PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
endif()

# 编译选项 - 参考 ImGui examples
if(MSVC)
    target_compile_options(remote-imgui-client-simple PRIVATE /W4)
    target_compile_definitions(remote-imgui-client-simple PRIVATE
        _CRT_SECURE_NO_WARNINGS
    )

    target_compile_options(simple-test PRIVATE /W4)
    target_compile_definitions(simple-test PRIVATE
        _CRT_SECURE_NO_WARNINGS
    )
else()
    target_compile_options(remote-imgui-client-simple PRIVATE -Wall -Wextra -Wformat)
    target_compile_options(simple-test PRIVATE -Wall -Wextra -Wformat)
endif()

# 临时：为网络客户端创建简单的stub，避免libevent依赖
target_compile_definitions(remote-imgui-client-simple PRIVATE
    DISABLE_NETWORKING=1  # 禁用网络功能进行测试
)

# 安装
install(TARGETS remote-imgui-client-simple
    RUNTIME DESTINATION bin
)

message(STATUS "Remote ImGui Client configuration completed")
message(STATUS "Built simple version without networking for testing")