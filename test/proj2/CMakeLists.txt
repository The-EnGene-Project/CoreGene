cmake_minimum_required(VERSION 3.14)
project(Proj2Mix)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add EnGene include directory
include_directories(
    "${CMAKE_SOURCE_DIR}/../../core_gene/src"
    "${CMAKE_SOURCE_DIR}/../../libs/glad/include"
    "${CMAKE_SOURCE_DIR}/../../libs/glfw/include"
    "${CMAKE_SOURCE_DIR}/../../libs/glm/include"
    "${CMAKE_SOURCE_DIR}/../../libs/stb/include"
    "${CMAKE_SOURCE_DIR}/../../libs/backtrace/include"
)

# Link directories for libraries (Windows MinGW)
link_directories(
    "${CMAKE_SOURCE_DIR}/../../libs/glfw/lib-mingw-w64"
    "${CMAKE_SOURCE_DIR}/../../libs/backtrace/lib"
)

# Create executables
add_executable(Proj2Mix proj2-mix.cpp)

# Link libraries for all executables
foreach(target Proj2Mix)
    target_link_libraries(${target}
        "${CMAKE_SOURCE_DIR}/../../libs/glfw/lib-mingw-w64/libglfw3.a"
        opengl32
        "${CMAKE_SOURCE_DIR}/../../libs/backtrace/lib/libbacktrace.a"
    )
endforeach()

# Copy shaders to build directory
file(COPY ${CMAKE_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR})
