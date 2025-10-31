cmake_minimum_required(VERSION 3.10)
project(SimpleOpenGLClient)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(IMGUI_PATH "../../../third-party/imgui/imgui")

set(all_srcs "")

file(GLOB top_src "*.*")
source_group(\\                 FILES ${top_src})
list(APPEND all_srcs ${top_src})         


# ImGui source files (directly include)
set(imgui_src
    ${IMGUI_PATH}/imgui.cpp
    ${IMGUI_PATH}/imgui_demo.cpp
    ${IMGUI_PATH}/imgui_draw.cpp
    ${IMGUI_PATH}/imgui_tables.cpp
    ${IMGUI_PATH}/imgui_widgets.cpp
)
source_group(\\imgui            FILES ${imgui_src})
list(APPEND all_srcs ${imgui_src})

# Backend sources
set(imgui_backend_srcs
    ${IMGUI_PATH}/backends/imgui_impl_glfw.cpp
    ${IMGUI_PATH}/backends/imgui_impl_opengl3.cpp
)
source_group(\\imgui\\backend   FILES ${imgui_backend_srcs})
list(APPEND all_srcs ${imgui_backend_srcs})


file(GLOB remote_debugger_src "../remote_debugger/*.*")
source_group(\\remote_debugger  FILES ${remote_debugger_src})
list(APPEND all_srcs ${remote_debugger_src})

file(GLOB remote_debugger_imgui_src "../remote_debugger/imgui/*.*")
source_group(\\remote_debugger\\imgui  FILES ${remote_debugger_imgui_src})
list(APPEND all_srcs ${remote_debugger_imgui_src})

# Create simple OpenGL client executable
add_executable(remote-imgui-client
    ${all_srcs}
)


# Include directories
target_include_directories(remote-imgui-client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ../../../third-party/imgui
    ${IMGUI_PATH}
    ${IMGUI_PATH}/backends
    ../../../third-party/
    ../../../third-party/imgui/imgui/examples/libs/glfw/include
)

# Windows specific libraries and settings
if(WIN32)
    target_link_libraries(remote-imgui-client
        ws2_32
        wsock32
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../third-party/imgui/imgui/examples/libs/glfw/lib-vc2010-64/glfw3.lib
        opengl32
    )

    # opengl32
    # IMGUI_IMPL_OPENGL_LOADER_CUSTOM
    target_compile_definitions(remote-imgui-client PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
endif()


target_link_libraries(remote-imgui-client
    event_core
    event
)

# Compiler options
if(MSVC)
    target_compile_options(remote-imgui-client PRIVATE /W4)
else()
    target_compile_options(remote-imgui-client PRIVATE -Wall -Wextra -Wformat)
endif()

# Install
install(TARGETS remote-imgui-client
    RUNTIME DESTINATION bin
)

message(STATUS "Simple OpenGL Client configuration completed")
message(STATUS "Built with basic ImGui and headless rendering")