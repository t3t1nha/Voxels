cmake_minimum_required(VERSION 3.10)
project(VoxelEngine)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)

# Find required libraries
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)

# --- NEW: Add ImGui as a subproject ---
include(FetchContent)
FetchContent_Declare(
  imgui
  GIT_REPOSITORY "https://github.com/ocornut/imgui.git"
  GIT_TAG "v1.90"
)
FetchContent_MakeAvailable(imgui)

# Add executable
add_executable(VoxelEngine main.cpp Frustum.cpp)

find_package(glm REQUIRED)

# --- NEW: Add ImGui source files ---
target_sources(VoxelEngine PRIVATE
    libs/imgui/imgui.cpp
    libs/imgui/imgui_draw.cpp
    libs/imgui/imgui_tables.cpp
    libs/imgui/imgui_widgets.cpp
    libs/imgui/backends/imgui_impl_glfw.cpp
    libs/imgui/backends/imgui_impl_opengl3.cpp
)

# Link libraries
target_link_libraries(VoxelEngine
    OpenGL::GL
    glfw
    GLEW::GLEW
)

# Include directories
target_include_directories(VoxelEngine PRIVATE
    ${OPENGL_INCLUDE_DIR}
    ${GLFW3_INCLUDE_DIR}
    ${GLEW_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    libs/imgui
    libs/imgui/backends
)