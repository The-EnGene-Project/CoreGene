cmake_minimum_required(VERSION 3.14)
project(EnGeneTest)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Create test executables
add_executable(EnGeneTest main.cpp)
add_executable(SimpleTest simple_test.cpp)

# Function to configure a test target
function(configure_test_target target_name)
    # Add EnGene include directory (from parent directory)
    target_include_directories(${target_name} PRIVATE
        "${CMAKE_SOURCE_DIR}/../core_gene/src"
    )

    # Add dependency include directories
    target_include_directories(${target_name} PRIVATE
        "${CMAKE_SOURCE_DIR}/../libs/glad/include"
        "${CMAKE_SOURCE_DIR}/../libs/glfw/include"
        "${CMAKE_SOURCE_DIR}/../libs/glm/include"
        "${CMAKE_SOURCE_DIR}/../libs/stb/include"
        "${CMAKE_SOURCE_DIR}/../libs/backtrace/include"
    )

    # Link directories for libraries
    link_directories(
        "${CMAKE_SOURCE_DIR}/../libs/glfw/lib-mingw-w64"
        "${CMAKE_SOURCE_DIR}/../libs/backtrace/lib"
    )

    # Link libraries
    target_link_libraries(${target_name}
        "${CMAKE_SOURCE_DIR}/../libs/glfw/lib-mingw-w64/libglfw3.a"
        opengl32.lib
        "${CMAKE_SOURCE_DIR}/../libs/backtrace/lib/libbacktrace.a"
    )

    # Add debug symbols
    target_compile_options(${target_name} PRIVATE -g)
endfunction()

# Configure both test targets
configure_test_target(EnGeneTest)
configure_test_target(SimpleTest)
