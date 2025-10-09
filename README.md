# EnGene - The modular graphical engine for developers

This project tries to be a modular declarative engine for applications that
want to render fully customizable graphics. 

Currently the project interfaces with the GPU using OpenGL, but there are plans
for adding compatibility with different graphic APIs like Vulkan and DirectX.

### How to include this library in your project

Using CMake:
```cmake
include(FetchContent)

FetchContent_Declare(
  CoreGene 
  GIT_REPOSITORY https://github.com/The-EnGene-Project/CoreGene.git
  GIT_TAG        main # Or a specific commit hash or release tag like v1.2.0
)

FetchContent_MakeAvailable(CoreGene)
FetchContent_GetProperties(CoreGene)

include_directory(${CoreGene_SOURCE_DIR}/src)
```