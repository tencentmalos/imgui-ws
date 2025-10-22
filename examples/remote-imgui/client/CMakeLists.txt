cmake_minimum_required(VERSION 3.10)
project(SimpleOpenGLClient)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(IMGUI_PATH "../../../third-party/imgui/imgui")

# ImGui source files (directly include)
set(IMGUI_SOURCES
    ${IMGUI_PATH}/imgui.cpp
    ${IMGUI_PATH}/imgui_demo.cpp
    ${IMGUI_PATH}/imgui_draw.cpp
    ${IMGUI_PATH}/imgui_tables.cpp
    ${IMGUI_PATH}/imgui_widgets.cpp
)

# Backend sources
set(BACKEND_SOURCES
    ${IMGUI_PATH}/backends/imgui_impl_glfw.cpp
    ${IMGUI_PATH}/backends/imgui_impl_opengl3.cpp
)

# Create simple OpenGL client executable
add_executable(simple-opengl-client
    simple_opengl_client.cpp
    ../deserializer.cpp
    ${IMGUI_SOURCES}
    ${BACKEND_SOURCES}
)



# Include directories
target_include_directories(simple-opengl-client PRIVATE
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
    target_link_libraries(simple-opengl-client
        ws2_32
        wsock32
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../third-party/imgui/imgui/examples/libs/glfw/lib-vc2010-64/glfw3.lib
        opengl32
    )

    # opengl32
    # IMGUI_IMPL_OPENGL_LOADER_CUSTOM
    target_compile_definitions(simple-opengl-client PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
endif()


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
message(STATUS "Built with basic ImGui and headless rendering")