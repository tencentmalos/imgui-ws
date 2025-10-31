# Remote Debugger Library CMake Configuration
# This creates a static library containing network protocol and ImGui serialization

cmake_minimum_required(VERSION 3.10)
project(RemoteDebugger
    VERSION 1.0.0
    DESCRIPTION "Network Protocol and ImGui Serialization for Remote Debugging"
    LANGUAGES CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(IMGUI_PATH "../../../third-party/imgui/imgui")

set(all_srcs "")

file(GLOB top_src "*.*")
source_group(\\remote_debuger                 FILES ${top_src})
list(APPEND all_srcs ${top_src})         

file(GLOB remote_debugger_imgui_src "imgui/*.*")
source_group(\\remote_debuger\\imgui          FILES ${remote_debugger_imgui_src})
list(APPEND all_srcs ${remote_debugger_imgui_src})        


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


# Create the static library
add_library(remote_debugger STATIC
    ${all_srcs}
)


# Include directories
target_include_directories(remote_debugger
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Add ImGui include directory for this project structure
target_include_directories(remote_debugger PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../third-party/imgui
    ${IMGUI_PATH}
    ${IMGUI_PATH}/backends
)

# Compiler-specific options
if(MSVC)
    target_compile_options(remote_debugger PRIVATE /W4)
else()
    target_compile_options(remote_debugger PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Set library properties
set_target_properties(remote_debugger PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)

# Export library symbols
target_compile_definitions(remote_debugger PRIVATE REMOTE_DEBUGGER_STATIC)


