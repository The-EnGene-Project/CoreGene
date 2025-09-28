# Versão mínima do CMake
cmake_minimum_required(VERSION 3.10)

# Nome do projeto
project(EnGene)

# Adiciona o caminho para as bibliotecas de inclusão
include_directories(
    "${CMAKE_SOURCE_DIR}/libs/glad/include"
    "${CMAKE_SOURCE_DIR}/libs/glfw/include"
    "${CMAKE_SOURCE_DIR}/libs/glm/include"
)

# Adiciona o caminho para as bibliotecas de linkagem
link_directories(
    "${CMAKE_SOURCE_DIR}/libs/glfw/lib-mingw-w64" # Mude para a sua pasta de libs da GLFW
)

# Adiciona o executável
add_executable(${PROJECT_NAME} core_gene/esqueleto.cpp)

# Linka as bibliotecas com o executável
target_link_libraries(${PROJECT_NAME}
    glfw3.lib
    opengl32.lib
)