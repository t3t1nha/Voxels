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

# --- Automatically add all .cpp files in src and subfolders ---
file(GLOB_RECURSE SOURCES
    src/*.cpp
)

# --- ImGui sources (if not using FetchContent above, adjust as needed) ---
set(IMGUI_SOURCES
    libs/imgui/imgui.cpp
    libs/imgui/imgui_draw.cpp
    libs/imgui/imgui_tables.cpp
    libs/imgui/imgui_widgets.cpp
    libs/imgui/backends/imgui_impl_glfw.cpp
    libs/imgui/backends/imgui_impl_opengl3.cpp
)

# Add executable
add_executable(VoxelEngine
    ${SOURCES}
    ${IMGUI_SOURCES}
)

# --- Include directories ---
target_include_directories(VoxelEngine PRIVATE
    ${OPENGL_INCLUDE_DIR}
    ${GLFW3_INCLUDE_DIR}
    ${GLEW_INCLUDE_DIR}
    ${GLM_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Engine
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Generation
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/imgui/backends
)

# --- Link libraries ---
target_link_libraries(VoxelEngine
    OpenGL::GL
    glfw
    GLEW::GLEW
)