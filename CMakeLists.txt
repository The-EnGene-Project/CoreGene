# Versão mínima do CMake
cmake_minimum_required(VERSION 3.14)

# Nome do projeto
project(Proj2)

include(FetchContent)

FetchContent_Declare(
  CoreGene # Give it a name
  GIT_REPOSITORY https://github.com/The-EnGene-Project/CoreGene.git
  GIT_TAG        main # Or a specific commit hash or release tag like v1.2.0
)

FetchContent_Populate(CoreGene)
# FetchContent_MakeAvailable(CoreGene)
# FetchContent_GetProperties(CoreGene)

# Adiciona o executável
add_executable(${PROJECT_NAME} src/proj2.cpp)
target_compile_options(${PROJECT_NAME} PRIVATE -g)

# Adiciona o caminho para as bibliotecas de inclusão
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_SOURCE_DIR}/libs/glad/include"
    "${CMAKE_SOURCE_DIR}/libs/glfw/include"
    "${CMAKE_SOURCE_DIR}/libs/glm/include"
    "${CMAKE_SOURCE_DIR}/libs/stb/include"
    "${CMAKE_SOURCE_DIR}/libs/backtrace/include"
    "${coregene_SOURCE_DIR}/core_gene/src"
)

# Adiciona o caminho para as bibliotecas de linkagem
link_directories(
    "${CMAKE_SOURCE_DIR}/libs/glfw/lib-mingw-w64" # Mude para a sua pasta de libs da GLFW
    "${CMAKE_SOURCE_DIR}/libs/backtrace/lib"
)

# Linka as bibliotecas com o executável
target_link_libraries(${PROJECT_NAME}
    "${CMAKE_SOURCE_DIR}/libs/glfw/lib-mingw-w64/libglfw3.a"
    opengl32.lib
    "${CMAKE_SOURCE_DIR}/libs/backtrace/lib/libbacktrace.a"
)


# This creates a command that will overwrite the local copy with the remote
add_custom_target(refetch_coregene
  # Step 1: Download all the latest info from the remote server
  COMMAND ${GIT_EXECUTABLE} fetch origin
  # Step 2: Force the local main branch to match the remote main branch
  COMMAND ${GIT_EXECUTABLE} reset --hard origin/main
  # The directory to run the command in
  WORKING_DIRECTORY ${coregene_SOURCE_DIR} 
  # A helpful message to display when you run it
  COMMENT "Force-updating CoreGene to the latest version from the main branch..."
  # This makes sure the output of git is shown
  USES_TERMINAL
)