cmake_minimum_required(VERSION 3.10)
project(SimpleOpenGLClient)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ImGui source files (directly include)
set(IMGUI_SOURCES
    ../../../third-party/imgui/imgui/imgui.cpp
    ../../../third-party/imgui/imgui/imgui_demo.cpp
    ../../../third-party/imgui/imgui/imgui_draw.cpp
    ../../../third-party/imgui/imgui/imgui_tables.cpp
    ../../../third-party/imgui/imgui/imgui_widgets.cpp
)

# Create simple OpenGL client executable
add_executable(simple-opengl-client
    simple_opengl_client.cpp
    ../deserializer.cpp
    ${IMGUI_SOURCES}
)

# Include directories
target_include_directories(simple-opengl-client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ../../../third-party/imgui
    ../../../third-party/imgui/imgui
)

# Windows specific libraries and settings
if(WIN32)
    target_link_libraries(simple-opengl-client
        ws2_32
        wsock32
    )
    target_compile_definitions(simple-opengl-client PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
endif()

# Add libevent for networking
target_link_libraries(simple-opengl-client
    event_core
)

# Compiler options
if(MSVC)
    target_compile_options(simple-opengl-client PRIVATE /W4)
else()
    target_compile_options(simple-opengl-client PRIVATE -Wall -Wextra -Wformat)
endif()

# Install
install(TARGETS simple-opengl-client
    RUNTIME DESTINATION bin
)

message(STATUS "Simple OpenGL Client configuration completed")
message(STATUS "Built with basic ImGui and simulated OpenGL backend")