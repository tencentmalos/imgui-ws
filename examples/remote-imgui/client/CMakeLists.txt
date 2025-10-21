cmake_minimum_required(VERSION 3.10)
project(RemoteImGuiClient)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../third-party/imgui/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../third-party/libevent/include
)

# ImGui 源文件
set(IMGUI_SOURCES
    ../../../third-party/imgui/imgui/imgui.cpp
    ../../../third-party/imgui/imgui/imgui_draw.cpp
    ../../../third-party/imgui/imgui/imgui_tables.cpp
    ../../../third-party/imgui/imgui/imgui_widgets.cpp
)

# ImGui 后端源文件
set(IMGUI_BACKEND_SOURCES
    ../../../third-party/imgui/imgui/backends/imgui_impl_glfw.cpp
    ../../../third-party/imgui/imgui/backends/imgui_impl_opengl3.cpp
)

# 使用服务器构建的libevent，避免重复构建
# libevent 在服务器端构建，这里不需要重复构建

# 检查 OpenGL 相关库
find_package(OpenGL QUIET)
if(OPENGL_FOUND)
    include_directories(${OPENGL_INCLUDE_DIR})
    set(OPENGL_LIB ${OPENGL_LIBRARIES})
else()
    if(WIN32)
        set(OPENGL_LIB opengl32)
    else()
        set(OPENGL_LIB GL)
    endif()
endif()

# 查找 GLFW 和 GLEW
set(FOUND_GLFW_DEPS FALSE)
find_package(glfw3 QUIET)
find_package(GLEW QUIET)

if(glfw3_FOUND AND GLEW_FOUND)
    set(FOUND_GLFW_DEPS TRUE)
    message(STATUS "Found GLFW and GLEW via find_package")
else()
    # 尝试使用项目内的第三方库
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../third-party/imgui/examples/libs/glfw" AND
       EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../third-party/glew")
        set(FOUND_GLFW_DEPS TRUE)
        message(STATUS "Using third-party GLFW and GLEW from project")

        include_directories(
            ${CMAKE_CURRENT_SOURCE_DIR}/../../../third-party/imgui/examples/libs/glfw/include
            ${CMAKE_CURRENT_SOURCE_DIR}/../../../third-party/glew/include
        )

        # 尝试使用项目内的第三方库
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../third-party/imgui/examples/libs/glfw" AND
           EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../third-party/glew")
            set(FOUND_GLFW_DEPS TRUE)
            message(STATUS "Using third-party GLFW and GLEW from project")

            include_directories(
                ${CMAKE_CURRENT_SOURCE_DIR}/../../../third-party/imgui/examples/libs/glfw/include
                ${CMAKE_CURRENT_SOURCE_DIR}/../../../third-party/glew/include
            )

            # 尝试使用项目内的第三方库
            if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../third-party/imgui/examples/libs/glfw" AND
               EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../third-party/glew")
                set(FOUND_GLFW_DEPS TRUE)
                message(STATUS "Using third-party GLFW and GLEW from project")

                include_directories(
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../../third-party/imgui/examples/libs/glfw/include
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../../third-party/glew/include
                    )

                # 检查是否需要编译第三方库
                set(GLFW_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../third-party/imgui/examples/libs/glfw")
                set(GLEW_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../third-party/glew")

                if(EXISTS "${GLFW_SOURCE_DIR}")
                    message(STATUS "Found GLFW source, adding as subdirectory")
                    add_subdirectory(${GLFW_SOURCE_DIR} glfw_build)
                    set(GLFW_LIBRARIES glfw)
                endif()

                if(EXISTS "${GLEW_SOURCE_DIR}")
                    message(STATUS "Found GLEW source, adding as subdirectory")
                    add_subdirectory(${GLEW_SOURCE_DIR} glew_build)
                    set(GLEW_LIBRARIES glew)
                endif()
            endif()
        else()
            message(WARNING "GLFW and GLEW not found, client will not be built")
            message(STATUS "To build client, install GLFW and GLEW or ensure third-party libraries are available")
        endif()
    endif()
endif()

# 只有在有依赖的情况下才构建客户端
if(FOUND_GLFW_DEPS)
    # Client 可执行文件
    add_executable(remote-imgui-client
        client_main.cpp
        imgui_client.cpp
        imgui_client.h
        ../network_client.cpp
        ../network_client.h
        ../deserializer.h
        ../protocol.h
        ${IMGUI_SOURCES}
        ${IMGUI_BACKEND_SOURCES}
    )

    # Client 链接库
    target_link_libraries(remote-imgui-client
        ${OPENGL_LIB}
        event_core  # libevent 核心库（源码编译）
        ${GLFW_LIBRARIES}
        ${GLEW_LIBRARIES}
    )

    # Windows 额外库
    if(WIN32)
        target_link_libraries(remote-imgui-client
            ws2_32
            wsock32
        )
        target_compile_definitions(remote-imgui-client PRIVATE
            GLEW_STATIC
            WIN32_LEAN_AND_MEAN
            NOMINMAX
        )
    endif()

    # 编译选项
    if(MSVC)
        target_compile_options(remote-imgui-client PRIVATE /W4)
        target_compile_definitions(remote-imgui-client PRIVATE
            _CRT_SECURE_NO_WARNINGS
        )
    else()
        target_compile_options(remote-imgui-client PRIVATE -Wall -Wextra -pedantic)
    endif()

    # 安装
    install(TARGETS remote-imgui-client
        RUNTIME DESTINATION bin
    )
endif()

# 测试程序 - 只有在文件存在时才构建
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../test_build.cpp")
    add_executable(test_build_client
        ../test_build.cpp
        ../protocol.h
        ../serializer.h
    )

    if(MSVC)
        target_compile_options(test_build_client PRIVATE /W4)
        target_compile_definitions(test_build_client PRIVATE
            _CRT_SECURE_NO_WARNINGS
        )
    else()
        target_compile_options(test_build_client PRIVATE -Wall -Wextra -pedantic)
    endif()
endif()

message(STATUS "Remote ImGui Client configuration completed")
message(STATUS "GLFW Found: ${FOUND_GLFW_DEPS}")