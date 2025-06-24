cmake_minimum_required(VERSION 3.10)
project(VoxelEngine)

# Set C++ standard (required for GLM)
set(CMAKE_CXX_STANDARD 17)

# Find required libraries
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)

# Add executable (only main.cpp needed since headers are included)
add_executable(VoxelEngine main.cpp)

# Link libraries
target_link_libraries(VoxelEngine 
    OpenGL::GL
    glfw
    GLEW::GLEW
)

# Include directories (for your headers)
target_include_directories(VoxelEngine PRIVATE
    ${OPENGL_INCLUDE_DIR}
    ${GLFW3_INCLUDE_DIR}
    ${GLEW_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}  # For your .h files
)