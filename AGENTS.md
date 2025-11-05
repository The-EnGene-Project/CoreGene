# AGENTS.md - EnGene Development Guide

> **For Library Users:** If you want to use EnGene in your application, see [README.md](README.md) for API documentation, usage examples, and integration instructions.

---

## Table of Contents

- [Quick Troubleshooting](#quick-troubleshooting)
- [Project Overview](#project-overview)
  - [Key Dependencies](#key-dependencies)
- [Development Environment Setup](#development-environment-setup)
  - [Build System](#build-system)
  - [Build Commands](#build-commands)
  - [Dependency Management](#dependency-management)
  - [Platform-Specific Notes](#platform-specific-notes)
  - [Including EnGene in Your Project](#including-engene-in-your-project)
- [Testing Protocol](#testing-protocol)
  - [Testing Framework](#testing-framework)
  - [Setting Up a Test Project](#setting-up-a-test-project)
  - [Running Tests](#running-tests)
  - [Test File Structure](#test-file-structure)
- [Core Architecture & Design Philosophy](#core-architecture--design-philosophy)
  - [Resource Management Philosophy](#resource-management-philosophy)
  - [OpenGL Wrapper Philosophy](#opengl-wrapper-philosophy)
  - [Error Handling](#error-handling)
- [Architecture Deep Dive](#architecture-deep-dive)
  - [Scene Graph Architecture](#scene-graph-architecture)
  - [Component System Architecture](#component-system-architecture)
  - [Rendering System Architecture](#rendering-system-architecture)
  - [Observer Pattern Implementation](#observer-pattern-implementation)
  - [Namespace Organization & Singleton Access](#namespace-organization--singleton-access)
  - [Threading Model](#threading-model)
- ["Other Genes" Philosophy](#other-genes-philosophy)
  - [CoreGene vs Other Genes](#coregene-vs-other-genes)
  - [What Belongs in CoreGene?](#what-belongs-in-coregene)
  - [What Belongs in Other Genes?](#what-belongs-in-other-genes)
  - [When to Extract into Separate Gene Repositories](#when-to-extract-into-separate-gene-repositories)
  - [Guidelines for New Contributions](#guidelines-for-new-contributions)
- [Coding Standards](#coding-standards)
  - [Naming Conventions](#naming-conventions)
  - [Header Include Order](#header-include-order)
  - [Header-Only Library Guidelines](#header-only-library-guidelines)
  - [Pointers & References](#pointers--references)
  - [Code Formatting](#code-formatting)
  - [Documentation Standards](#documentation-standards)
- [Critical Rules & Gotchas](#critical-rules--gotchas)
  - [DO NOT](#do-not)
  - [ALWAYS](#always)
  - [Common Gotchas](#common-gotchas)
  - [Common Integration Pitfalls (From Testing)](#common-integration-pitfalls-from-testing)
- [Development Workflow](#development-workflow)
  - [Commit Message Format](#commit-message-format)
  - [Branching Strategy](#branching-strategy)
  - [Code Review Checklist](#code-review-checklist)
  - [Pull Request Process](#pull-request-process)
  - [Development Steps](#development-steps)

---

## Quick Troubleshooting

**Compilation Errors:**
- "Incomplete type SceneNodeBuilder" → Include `<core/scene_node_builder.h>`

**Black Screen Issues:**
- Add `glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)` in render callback
- Check if custom shader has `u_model` uniform configured

**Build Issues:**
- On Windows MinGW: Use `cmake -B build -G "MinGW Makefiles"`
- Multiple GLAD definitions: Don't include GLAD separately when using `EnGene.h`

**See [Common Integration Pitfalls](#common-integration-pitfalls-from-testing) for detailed solutions.**

---

## Project Overview

**EnGene** is a modular, declarative C++ OpenGL abstraction library designed for developers who want full control over their graphics rendering pipeline. The library provides a scene graph architecture with an ECS-inspired component system, hierarchical transforms, and a sophisticated 4-tier uniform management system.

**Purpose:** 2D/3D rendering engine and OpenGL abstraction layer  
**Library Type:** Header-only library (no .cpp files, all implementation in headers)  
**C++ Standard:** C++17 minimum (C++20 recommended for aggregate initialization)  
**OpenGL Version:** 4.3 Core Profile

### Key Dependencies
- **GLFW** - Window and input management
- **GLAD** - OpenGL function loader (must define `GLAD_GL_IMPLEMENTATION` before including)
- **GLM** - Mathematics library for graphics
- **STB Image** - Image loading (stb_image.h)
- **Backtrace** (optional) - Debug stack traces and error reporting

**CRITICAL:** When including `EnGene.h`, the library automatically defines `GLAD_GL_IMPLEMENTATION`. If you include GLAD separately, ensure this is defined only once in your entire project.

---

## Development Environment Setup

### Build System

The project uses **CMake** (minimum version 3.14) with **FetchContent** for dependency management.

**IMPORTANT: EnGene is a header-only library.** There is no separate compilation step for the library itself. You only need to include the headers in your project and compile your application code.

### Build Commands

```bash
# Configure the build
cmake -B build

# Compile the library
cmake --build build

# Force update CoreGene from remote (if using as submodule)
cmake --build build --target refetch_coregene
```

### Dependency Management

- Dependencies are located in `/libs/` directory
- CoreGene can be fetched via CMake FetchContent from the main repository
- External libraries (GLFW, GLAD, GLM, STB) are managed manually in `/libs/`

### Platform-Specific Notes

**Windows (MinGW-w64):**
- GLFW library path: `libs/glfw/lib-mingw-w64/libglfw3.a`
- Links against `opengl32.lib`
- Backtrace library: `libs/backtrace/lib/libbacktrace.a`

**Linux:**
- Use system OpenGL libraries (`-lGL`)
- GLFW typically installed via package manager
- Adjust library paths in CMakeLists.txt accordingly

**macOS:**
- Link against OpenGL framework (`-framework OpenGL`)
- Use Homebrew for GLFW installation
- May require additional framework links

### Including EnGene in Your Project

**Method 1: Git Submodule (Recommended)**
```bash
# Add EnGene as a submodule to your project
git submodule add https://github.com/The-EnGene-Project/CoreGene.git external/CoreGene
git submodule update --init --recursive
```

Then in your `CMakeLists.txt`:
```cmake
# Add EnGene include directory
target_include_directories(YourTarget PRIVATE
    "${CMAKE_SOURCE_DIR}/external/CoreGene/core_gene/src"
)

# Link required dependencies
target_include_directories(YourTarget PRIVATE
    "${CMAKE_SOURCE_DIR}/libs/glad/include"
    "${CMAKE_SOURCE_DIR}/libs/glfw/include"
    "${CMAKE_SOURCE_DIR}/libs/glm/include"
    "${CMAKE_SOURCE_DIR}/libs/stb/include"
)

# Link GLFW and OpenGL
target_link_libraries(YourTarget
    glfw
    opengl32  # Windows: opengl32.lib, Linux: -lGL, macOS: -framework OpenGL
)
```

**Method 2: Manual Copy**
1. Download or clone the CoreGene repository
2. Copy the `core_gene/src` directory to your project
3. Add the include path to your CMakeLists.txt:
```cmake
target_include_directories(YourTarget PRIVATE
    "${CMAKE_SOURCE_DIR}/path/to/core_gene/src"
)
```

**Method 3: System-Wide Installation**
```bash
# Copy headers to system include directory
sudo cp -r core_gene/src/EnGene.h /usr/local/include/
sudo cp -r core_gene/src/* /usr/local/include/engene/
```

Then in your code:
```cpp
#include <EnGene.h>
// Or specific headers:
#include <engene/gl_base/shader.h>
```

---

## Testing Protocol

### Testing Framework

Currently, the project uses manual testing via example applications:
- `example_main.cpp` - Solar system demo with lighting
- `working_example_main.cpp` - Physics simulation demo
- `debug_main.cpp` - Debug and testing harness

**IMPORTANT: Testing must be done in a separate project folder outside the repository.**

### Setting Up a Test Project

Since EnGene is a header-only library, testing should be done in a separate directory:

```bash
# Create a test project outside the repository
mkdir ../EnGeneTest
cd ../EnGeneTest

# Create a simple test application
cat > main.cpp << 'EOF'
#include <EnGene.h>

int main() {
    // Your test code here
    return 0;
}
EOF

# Create CMakeLists.txt
cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.14)
project(EnGeneTest)

# Add EnGene as submodule or set path
target_include_directories(EnGeneTest PRIVATE
    "../CoreGene/core_gene/src"
)

# Add dependencies and link libraries
# (See "Including EnGene in Your Project" section)
EOF
```

### Running Tests

```bash
# From your test project directory (NOT the repository)
cmake -B build
cmake --build build
./build/EnGeneTest  # Or your executable name
```

### Test File Structure

- **DO NOT** add test `.cpp` files to the EnGene repository
- Create separate test projects in external directories
- Example applications in `core_gene/` are for demonstration only
- Each test project should demonstrate specific engine features

**Why separate folders?**
- Keeps the library repository clean and focused
- Prevents accidental commits of test executables
- Allows users to test without modifying the library
- Maintains header-only library structure

**Future:** Consider integrating GTest or Catch2 for automated testing in a dedicated test repository.

---

## Core Architecture & Design Philosophy

### Resource Management Philosophy

**STRICT RAII ENFORCEMENT:**
- All OpenGL resources (shaders, textures, VAOs, VBOs) are wrapped in RAII classes
- Destructors automatically call appropriate `glDelete*` functions
- Use `std::shared_ptr` for shared resources (textures, shaders)
- Use `std::unique_ptr` for exclusive ownership (components, internal state)
- Raw pointers are ONLY used for non-owning references (e.g., weak parent pointers)

**Memory Ownership Rules:**
- Scene graph nodes use `std::shared_ptr` for children (shared ownership)
- Components use `std::weak_ptr` to reference their owner node (avoid cycles)
- Textures use static caching with `std::shared_ptr` to prevent duplicate GPU uploads
- Transform observers use weak references to avoid circular dependencies

### OpenGL Wrapper Philosophy

**NEVER call raw OpenGL functions directly in application code.**

Instead, use the provided abstractions:
- **Shader class** - Manages shader programs and uniforms
- **Geometry class** - Wraps VAO/VBO/EBO
- **Texture class** - Handles texture loading and binding
- **Transform class** - Matrix operations
- **Material class** - Material properties

**Exception:** Low-level engine code in `gl_base/` may call OpenGL directly, but must use `GL_CHECK()` macro.

### Error Handling

**OpenGL Error Checking:**
EnGene provides two complementary error checking mechanisms:

1. **Modern Debug Callback (Recommended)**
   - Automatically enabled by `EnGene` constructor when debug context is available
   - Uses `glDebugMessageCallback` for real-time error reporting
   - Provides detailed error messages with source, type, and severity
   - High-severity errors automatically trigger stack traces and abort
   - Enable manually: `Error::EnableDebugCallback()` (called automatically by EnGene)

2. **Legacy glGetError() Checking**
   - Use `GL_CHECK("operation description")` macro after critical OpenGL calls
   - Defined in `gl_base/error.h`
   - Manually checks `glGetError()` and aborts on failure
   - Useful for pinpointing exact error locations
   - Automatically prints stack trace using libbacktrace

**Debug Context Setup:**
The EnGene constructor automatically requests an OpenGL debug context:
```cpp
glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
```
This enables the modern debug callback system. If debug context creation fails, a warning is logged.

**Stack Traces:**
- Requires libbacktrace library (optional dependency)
- Automatically printed on high-severity OpenGL errors
- Printed on `GL_CHECK()` failures
- Shows function names, file paths, and line numbers

**Exception Hierarchy:**
- `exception::EnGeneException` - Base class for all engine exceptions (inherits from `std::runtime_error`)
- `exception::ShaderException` - Shader compilation/linking errors
- `exception::NodeNotFoundException` - Scene graph lookup failures

**Error Handling Strategy:**
- Throw exceptions for unrecoverable errors (shader compilation failure, GLFW init failure)
- Log warnings to `std::cerr` for non-critical issues (missing uniforms, binding point conflicts)
- Abort with stack trace for OpenGL errors (high-severity debug messages, GL_CHECK failures)
- Validate state in debug builds, optimize out checks in release builds

---

## Architecture Deep Dive

This section provides implementation details and design rationale for EnGene's core systems. For API usage and examples, see the corresponding sections in [README.md](README.md).


### Scene Graph Architecture

**For API usage, see:** [Scene Management in README.md](README.md#scene-management)

#### Implementation Details

**Node<T>** - Generic tree structure implementing Composite pattern
- Templated on payload type for flexibility
- Maintains parent/child relationships via shared/weak pointers
- Supports visitor pattern for traversal
- Methods: `addChild()`, `removeChild()`, `visit()`, `setApplicable()`

**Design Rationale:**
The generic `Node<T>` template allows the same tree structure to be reused for different purposes. By templating on the payload type, we can have scene nodes (`Node<ComponentCollection>`), UI nodes, or any other hierarchical structure without code duplication.

**SceneGraph** - Meyers singleton managing the entire scene hierarchy
- Maintains root node and name/ID registries
- Entry point: `scene::graph()->addNode(name)`
- Provides `draw()` method to traverse and render the scene
- Automatically creates default orthographic camera

**Design Rationale:**
The singleton pattern ensures a single source of truth for the scene. The name and ID registries enable fast node lookup without traversing the entire tree. The visitor pattern in `draw()` allows flexible traversal strategies.

**SceneNode** - Type alias for `Node<ComponentCollection>`
- Fundamental building block of the scene
- Each node carries multiple components defining its behavior
- `visit()` triggers component `apply()` before children, `unapply()` after

**SceneNodeBuilder** - Fluent interface for declarative scene construction
- Chainable methods: `with<T>()`, `withNamed<T>()`, `addNode()`
- Eliminates manual node creation boilerplate
- Supports implicit conversion to `SceneNode&` and `SceneNodePtr`

**Design Rationale:**
The builder pattern makes scene construction more readable and maintainable. Method chaining creates a declarative syntax that mirrors the hierarchical structure of the scene graph.


### Component System Architecture

**For API usage, see:** [Component System in README.md](README.md#component-system)

#### ECS-Inspired Design

EnGene's component system is inspired by Entity-Component-System (ECS) architecture but adapted for a scene graph context:

- **Entities** → Scene nodes (hierarchical, not flat)
- **Components** → Modular behaviors attached to nodes
- **Systems** → Implicit via component priorities and scene traversal

**Component** - Abstract base class for all components
- Lifecycle: `apply()` (on entry) and `unapply()` (on exit)
- Each component has: unique ID, optional name, priority value, weak owner reference
- Priority determines execution order (see Priority System below)

**Design Rationale:**
The priority-based execution model provides deterministic ordering without explicit system dependencies. Components can be added/removed dynamically, and the priority system ensures correct execution order automatically.

**ComponentCollection** - Container managing all components on a node
- Three internal structures: type map, name map, priority-sorted vector
- Methods: `get<T>()`, `getAll<T>()`, removal by instance/name/ID
- `apply()` iterates forward, `unapply()` iterates reverse

**Design Rationale:**
Multiple data structures optimize different access patterns:
- Type map: Fast retrieval by component type
- Name map: Fast retrieval by component name
- Priority vector: Efficient ordered traversal during rendering

**Component Priority System:**
```
100 - Transform      (applied first)
200 - Shader         (shader selection)
300 - Appearance     (material, texture)
400 - Camera         (view/projection)
500 - Geometry       (drawing, applied last)
600+ - Custom        (user scripts)
```

**Design Rationale:**
Priority-based execution eliminates the need for explicit dependencies between components. Lower priorities execute first, ensuring transforms are set up before rendering, shaders are bound before materials, etc.


### Rendering System Architecture

**For API usage, see:** [Rendering System in README.md](README.md#rendering-system)

#### GPU State Management

EnGene uses a stack-based approach to manage OpenGL state hierarchically, mirroring the scene graph structure.

**Shader** - Manages GLSL programs with 4-tier uniform system

**4-Tier Uniform System Architecture:**

1. **Global Resources (UBOs/SSBOs)** - Bound at link time, shared across shaders
   - Managed by `GlobalResourceManager` singleton via `uniform::manager()`
   - Register with: `shader->addResourceBlockToBind("BlockName")`
   - Automatically bound during `Bake()`
   - Update modes: `PER_FRAME` (updated every frame) or `ON_DEMAND` (manual updates)

**Design Rationale:**
UBOs eliminate redundant uniform uploads for data shared across shaders (camera matrices, lights). Binding at link time ensures consistent binding points across all shaders.

2. **Static Uniforms** - Applied when shader activates
   - Configure with: `shader->configureStaticUniform<T>(name, provider)`
   - Applied once when `UseProgram()` is called

**Design Rationale:**
Static uniforms optimize for values that don't change during shader use (texture units, configuration flags). Provider functions allow lazy evaluation.

3. **Dynamic Uniforms** - Applied per-draw via provider functions
   - Configure with: `shader->configureDynamicUniform<T>(name, provider)`
   - Applied every time `ShaderStack::top()` is called
   - Provider functions are called each frame for fresh values

**Design Rationale:**
Dynamic uniforms handle frequently changing values (model matrix, materials). Provider functions decouple uniform configuration from value computation, enabling stack-based state management.

4. **Immediate Uniforms** - Set directly for one-off values
   - Set with: `shader->setUniform<T>(name, value)`
   - If shader inactive: queued and applied on next activation
   - If shader active: applied immediately

**Design Rationale:**
Immediate uniforms provide flexibility for one-off values (time, debug parameters) without requiring provider configuration.


#### Stack System Implementations

**ShaderStack** - Singleton managing active shader state
- Lazy activation: `top()` only calls `glUseProgram()` if shader changed
- Prevents redundant state changes
- Base shader cannot be popped (always valid shader available)

**Design Rationale:**
Lazy activation minimizes expensive `glUseProgram()` calls. The protected base ensures a valid shader is always available, preventing OpenGL errors.

**TransformStack** - Singleton managing hierarchical transforms
- Stores accumulated matrices (not individual transforms)
- `push()` multiplies with current top
- `current()` returns top matrix (used as model matrix)
- Cannot pop below identity matrix

**Design Rationale:**
Storing accumulated matrices eliminates redundant matrix multiplications during traversal. Each level represents the complete transform from root to that node.

**MaterialStack** - Singleton managing hierarchical material merging
- Each level stores complete merged property map
- `push()` copies current state and overwrites matching properties
- `getValue<T>()` for immediate access
- `getProvider<T>()` for creating uniform provider functions

**Design Rationale:**
Complete property maps at each level enable O(1) property lookup. Override semantics (not additive) match intuitive material inheritance behavior.

**TextureStack** - Singleton managing texture bindings across units
- Tracks GPU state to prevent redundant `glBindTexture()` calls
- Maintains sampler-to-unit mapping for dynamic resolution
- Intelligent state restoration on `pop()`

**Design Rationale:**
GPU state tracking eliminates redundant texture binds. Complete state storage at each level enables efficient restoration without querying OpenGL state.


#### Observer Pattern Implementation

**IObserver** interface: Defines `onNotify(const ISubject*)` method
**ISubject** interface: Manages observer list, provides `notify()` method

**Usage in Transform:**
- Transform implements `ISubject` interface
- Notifies observers on matrix changes
- `ObservedTransformComponent` implements `IObserver` to cache world transforms
- Prevents redundant matrix calculations by caching until notified

**Design Rationale:**
The observer pattern decouples transform changes from dependent systems. Components can react to transform updates without polling or tight coupling. Weak pointers prevent circular dependencies.

#### Namespace Organization & Singleton Access

EnGene uses namespaces to organize functionality and provide clean singleton access:

**Core Namespaces:**
- `engene::` - Main application class and configuration
- `scene::` - Scene graph and node management
- `shader::` - Shader and shader stack
- `transform::` - Transform and transform stack
- `material::` - Material and material stack
- `texture::` - Texture and texture stack
- `light::` - Lighting system
- `component::` - All component types
- `uniform::` - Uniform system and resource management
- `input::` - Input handling
- `exception::` - Exception types
- `node::` - Generic node template

**Singleton Access Pattern:**
All major singletons use Meyers singleton pattern with free function access:
```cpp
scene::graph()->addNode("name");
shader::stack()->push(my_shader);
transform::stack()->push(matrix);
```

**Design Rationale:**
Free functions provide cleaner syntax than `ClassName::getInstance()`. Meyers singletons are thread-safe for initialization and avoid static initialization order issues.


#### Threading Model

**Single-Threaded Rendering:**
- All OpenGL calls MUST occur on the main thread
- Scene graph traversal is single-threaded
- Component `apply()`/`unapply()` methods are NOT thread-safe
- Singleton access is NOT thread-safe (Meyers singletons are thread-safe for initialization only)

**Future Considerations:**
- Scene graph updates could be parallelized (not rendering)
- Physics/game logic can run on separate threads
- Use mutexes if modifying scene graph from non-main threads
- Consider double-buffering scene graph for async updates

---

## "Other Genes" Philosophy

**For API usage, see:** [Project Structure in README.md](README.md#project-structure)

### CoreGene vs Other Genes

The `other_genes/` directory contains **derived code that is NOT intended to remain in this repository permanently**. CoreGene is dedicated exclusively to the most **nuclear and central abstractions** (scene graph, components, shaders, transforms, etc.). The "other genes" are **specific derivations** of these core abstractions.

### What Belongs in CoreGene?

**Core Abstractions (Keep in CoreGene):**
- Scene graph infrastructure (`Node<T>`, `SceneGraph`, `SceneNodeBuilder`)
- Component system (`Component`, `ComponentCollection`)
- OpenGL wrappers (`Shader`, `Geometry`, `Texture`, `Transform`, `Material`)
- Stack systems (`ShaderStack`, `TransformStack`, `MaterialStack`, `TextureStack`)
- Camera base classes and core implementations
- Light base classes and manager
- Application framework (`EnGene`, `EnGeneConfig`)
- Input handling infrastructure

**Criteria for Core Abstractions:**
1. **Fundamental** - Required by most applications using the engine
2. **Abstract** - Defines interfaces and patterns, not specific implementations
3. **Stable** - API unlikely to change frequently
4. **Universal** - Applicable across different use cases


### What Belongs in Other Genes?

**Derived Implementations (Move to other_genes/):**
- Specific geometric shapes (circles, polygons, spheres, cubes)
- Textured shape variants
- Input handler presets (arcball controller, FPS controller)
- Utility classes (grid renderer, skybox)
- Application-specific components

**Criteria for Other Genes:**
1. **Specific** - Concrete implementations of core abstractions
2. **Optional** - Not required by all applications
3. **Derived** - Built using core abstractions, not extending them
4. **Extractable** - Can be moved to separate repository without breaking core

### When to Extract into Separate Gene Repositories

As the ecosystem matures, specific "genes" should be extracted into separate repositories:

**Extraction Criteria:**
1. **Maturity** - Code is stable and well-tested
2. **Cohesion** - Related functionality forms a logical unit
3. **Size** - Sufficient code to justify separate repository
4. **Independence** - Can be versioned and released independently

**Example Future Genes:**
- `ShapeGene` - Collection of 2D/3D geometric primitives
- `InputGene` - Advanced input handling and camera controllers
- `PhysicsGene` - Physics integration components
- `AudioGene` - Audio system integration
- `UIGene` - User interface components

### Guidelines for New Contributions

**When adding new code, ask:**

1. **Is this a core abstraction or a specific implementation?**
   - Core abstraction → Add to appropriate core directory
   - Specific implementation → Add to `other_genes/`

2. **Does this extend the engine's capabilities or use existing capabilities?**
   - Extends capabilities → Consider core directory
   - Uses capabilities → Add to `other_genes/`

3. **Will most applications need this?**
   - Yes → Consider core directory
   - No → Add to `other_genes/`

4. **Is this application-specific?**
   - Yes → Does NOT belong in CoreGene at all
   - No → Proceed with contribution

**Examples:**

✅ **Core Directory:**
- New component base class with lifecycle methods
- New stack system for managing OpenGL state
- Core camera implementation (orthographic, perspective)

✅ **Other Genes Directory:**
- Torus geometry implementation
- Procedural terrain generator
- Particle system component
- Debug visualization utilities

❌ **Does Not Belong:**
- Game-specific logic (player controller, enemy AI)
- Application UI (menus, HUD)
- Asset management for specific game
- Network code for multiplayer


---

## Coding Standards

### Naming Conventions

**Classes and Structs:** `PascalCase`
```cpp
class SceneNode;
struct EnGeneConfig;
```

**Functions and Methods:** `camelCase` for public API, `snake_case` for internal
```cpp
void addChild(NodePtr child);        // Public API
void apply_static_uniforms();        // Internal
```

**Member Variables:** `m_` prefix for private members
```cpp
class Shader {
private:
    unsigned int m_pid;
    bool m_is_dirty;
    std::vector<std::string> m_resource_blocks_to_bind;
};
```

**Constants and Enums:** `kPascalCase` or `ALL_CAPS`
```cpp
constexpr int kMaxLights = 16;
#define MAX_SCENE_LIGHTS 16
```

**Namespaces:** `lowercase`
```cpp
namespace shader { }
namespace component { }
namespace exception { }
```

**Smart Pointer Type Aliases:** `XXXXPtr` convention
- Use type aliases for smart pointers following the pattern `ClassNamePtr`
- The specific smart pointer type (`shared_ptr`, `unique_ptr`, `weak_ptr`) depends on ownership semantics
- This convention improves readability and makes ownership semantics explicit

```cpp
// Shared ownership (most common for scene graph objects)
class Shader;
using ShaderPtr = std::shared_ptr<Shader>;

class SceneNode;
using SceneNodePtr = std::shared_ptr<SceneNode>;

// Unique ownership (for exclusive resources)
class Component;
using ComponentPtr = std::unique_ptr<Component>;

// Weak references (for non-owning pointers)
class Transform;
using TransformWeakPtr = std::weak_ptr<Transform>;
```

**When to use each:**
- `std::shared_ptr` → `XXXXPtr` - For objects with shared ownership (nodes, shaders, textures)
- `std::unique_ptr` → `XXXXPtr` - For objects with exclusive ownership (components, internal state)
- `std::weak_ptr` → `XXXXWeakPtr` - For non-owning references (parent pointers, observers)


### Header Include Order

Since EnGene is a **header-only library**, follow this include order in your application `.cpp` files:

1. C system headers
2. C++ standard library headers
3. Third-party library headers (GLFW, GLM, etc.)
4. EnGene headers

```cpp
#include <cstdlib>            // 1. C system headers

#include <memory>             // 2. C++ standard library
#include <vector>
#include <string>

#include <glad/glad.h>        // 3. Third-party libraries
#include <glm/glm.hpp>

#include <EnGene.h>           // 4. EnGene main header
// Or include specific headers:
#include "gl_base/shader.h"
#include "core/scene.h"
```

### Header-Only Library Guidelines

**CRITICAL: EnGene is header-only. DO NOT create .cpp files for EnGene classes.**

- All implementation must be in header files (`.h`)
- Template classes naturally require full definitions in headers
- Non-template classes use `inline` functions or define methods in the class body
- This design choice eliminates linking issues and simplifies integration

**When contributing to EnGene:**
- ❌ DO NOT create `.cpp` implementation files
- ✅ DO implement all functions inline or within class definitions
- ✅ DO use `inline` keyword for non-member functions defined in headers
- ✅ DO use templates where appropriate to avoid ODR violations

### Pointers & References

**Smart Pointers:**
- `std::shared_ptr<T>` - Shared ownership (nodes, shaders, textures)
- `std::unique_ptr<T>` - Exclusive ownership (components, internal state)
- `std::weak_ptr<T>` - Non-owning references (parent pointers, observers)

**Raw Pointers:**
- ONLY for non-owning references
- NEVER for ownership transfer
- Prefer references when pointer cannot be null

**References:**
- Use `const T&` for read-only parameters
- Use `T&` for mutable parameters
- Return references for chaining (builder pattern)

### Code Formatting

- **Indentation:** 4 spaces (no tabs)
- **Braces:** Opening brace on same line (K&R style)
- **Line Length:** 120 characters maximum
- **Pointer/Reference:** `Type* ptr` or `Type& ref` (asterisk/ampersand with type)


### Documentation Standards

**Doxygen for Internal Documentation:**
EnGene uses Doxygen-style comments throughout the codebase for internal documentation.

**Documentation Requirements:**
- **All public classes** must have a `@class` or `@struct` block with `@brief` description
- **All public methods** should have `@brief` descriptions
- **Complex methods** should document `@param` and `@return` values
- **Important implementation details** should be documented with `@details` or `@note`

**Doxygen Comment Style:**
```cpp
/**
 * @class ClassName
 * @brief One-line description of the class.
 * 
 * Detailed description of the class behavior, design decisions,
 * and usage patterns. Can span multiple lines.
 */
class ClassName {
public:
    /**
     * @brief One-line description of the method.
     * @param paramName Description of the parameter.
     * @return Description of the return value.
     * 
     * @note Important implementation detail or gotcha.
     * @details Extended explanation of complex behavior.
     */
    ReturnType methodName(ParamType paramName);
};
```

**Common Doxygen Tags Used:**
- `@class`, `@struct` - Document classes and structs
- `@brief` - Short one-line description (required for all public APIs)
- `@param` - Document function parameters
- `@return` - Document return values
- `@note` - Highlight important information or warnings
- `@details` - Extended description for complex behavior
- `@code` / `@endcode` - Include code examples
- `@see` - Cross-reference related classes
- `@throws` - Document exceptions that may be thrown

**Documentation Examples from Codebase:**
```cpp
/**
 * @brief Enables the modern OpenGL debug message callback.
 * This is the recommended way to handle errors.
 * Call this ONCE after your OpenGL context is created and made current.
 */
static void EnableDebugCallback();

/**
 * @brief Constructs the EnGene application engine from a configuration struct.
 *
 * This constructor initializes GLFW, creates a window, sets up an OpenGL context,
 * and prepares the main loop.
 *
 * @param on_initialize A function that runs once for setup.
 * @param on_fixed_update A function for simulation logic, called at a fixed rate.
 * @param on_render A function for all drawing calls, called once per frame.
 * @param config A struct containing setup parameters for the engine.
 * @param handler A pointer to an InputHandler instance.
 */
EnGene(/* parameters */);
```

**When to Document:**
- ✅ DO document all public APIs (classes, methods, functions)
- ✅ DO document complex algorithms or non-obvious behavior
- ✅ DO document design decisions and rationale
- ✅ DO include usage examples for non-trivial APIs
- ❌ DON'T document obvious getters/setters unless they have side effects
- ❌ DON'T document private implementation details unless complex
- ❌ DON'T duplicate information that's obvious from the code


---

## Critical Rules & Gotchas

### DO NOT

1. **DO NOT create .cpp files for EnGene library code**
   - EnGene is a header-only library
   - All implementation must be in header files
   - Use `inline` functions or class-body definitions
   - Application code can have .cpp files, but not library code

2. **DO NOT call raw OpenGL functions directly in application code**
   - Use the provided abstractions (Shader, Geometry, Texture, etc.)
   - Exception: Low-level engine code in `gl_base/` with `GL_CHECK()` macro

3. **DO NOT use global variables**
   - Use singletons with Meyers pattern for global state
   - Access via functions: `scene::graph()`, `shader::stack()`, etc.

4. **DO NOT modify OpenGL state directly**
   - Use stacks: `ShaderStack`, `TransformStack`, `MaterialStack`, `TextureStack`
   - State changes are managed hierarchically

5. **DO NOT create circular shared_ptr references**
   - Parent-child: parent uses `shared_ptr`, child uses `weak_ptr` to parent
   - Observer pattern: subject uses `weak_ptr` to observers

6. **DO NOT forget to call `Bake()` after modifying shaders**
   - Attaching new shader stages requires re-linking
   - `Bake()` is called automatically by `UseProgram()` if dirty

7. **DO NOT mix uniform tiers incorrectly**
   - Tier 1 (UBOs): Bind at link time via `addResourceBlockToBind()`
   - Tier 2 (Static): Configure via `configureStaticUniform()`
   - Tier 3 (Dynamic): Configure via `configureDynamicUniform()`
   - Tier 4 (Immediate): Set via `setUniform()`

8. **DO NOT call OpenGL from non-main threads**
   - All rendering must occur on the main thread
   - Scene graph modifications from other threads require synchronization

9. **DO NOT include GLAD separately if using EnGene.h**
   - `EnGene.h` automatically defines `GLAD_GL_IMPLEMENTATION`
   - Including GLAD elsewhere will cause linker errors (multiple definitions)
   - If you need GLAD separately, include specific EnGene headers instead of `EnGene.h`

10. **DO NOT forget initialization order**
    - Include `<EnGene.h>` AFTER standard library and third-party headers
    - GLAD must be included before GLFW (handled automatically by EnGene.h)
    - Create EnGene instance before accessing singletons (they initialize on first use)

11. **DO NOT add derived/specific implementations to core directories**
    - CoreGene is for nuclear abstractions only (scene graph, components, shaders, transforms)
    - Derived implementations belong in `other_genes/` (shapes, input handlers, utilities)
    - Application-specific code does NOT belong in CoreGene at all
    - When `other_genes/` grows large, extract mature code into separate "gene" repositories

12. **DO NOT add new third-party dependencies without approval**
    - Discuss with team first
    - Consider impact on build system and portability


### ALWAYS

1. **ALWAYS check for shader compilation errors**
   - Shader class automatically throws `ShaderException` on failure
   - Catch and handle exceptions appropriately

2. **ALWAYS use `glm::` types for math**
   - `glm::vec2`, `glm::vec3`, `glm::vec4`
   - `glm::mat3`, `glm::mat4`
   - Never use raw float arrays for vectors/matrices

3. **ALWAYS use RAII for OpenGL resources**
   - Wrap resources in classes with proper destructors
   - Let smart pointers manage lifetime

4. **ALWAYS use `GL_CHECK()` after critical OpenGL calls**
   - Especially in low-level `gl_base/` code
   - Helps catch errors early in development

5. **ALWAYS validate uniform types match GLSL**
   - Shader class performs automatic validation
   - Pay attention to type mismatch warnings

6. **ALWAYS use the builder pattern for scene construction**
   - `scene::graph()->addNode(name).with<Component>().addNode(child)`
   - More readable and maintainable than manual construction

7. **ALWAYS set component priorities correctly**
   - Transform (100) before Geometry (500)
   - Shader (200) before Material (300)
   - Custom components use 600+

8. **ALWAYS use factory methods (`Make()`) for object creation**
   - Ensures proper initialization
   - Returns smart pointers with correct ownership

### Common Gotchas

**Uniform Location Invalidation:**
- Uniform locations become invalid after shader re-linking
- `Bake()` automatically refreshes all uniform locations
- Don't cache uniform locations manually

**Texture Unit Confusion:**
- Texture units (0-31) vs. texture IDs (OpenGL handles)
- `TextureComponent` binds to units, not IDs
- Use `texture::getSamplerProvider()` for dynamic unit resolution

**Transform Stack Accumulation:**
- `TransformStack` stores accumulated matrices, not individual transforms
- `push()` multiplies with current top
- Don't push the same transform twice expecting addition

**Material Property Merging:**
- `MaterialStack` merges properties hierarchically
- Child materials override parent properties with matching names
- Use unique names to avoid unintended overrides

**Component Execution Order:**
- Components execute in priority order during `apply()`
- Reverse order during `unapply()`
- Incorrect priorities cause rendering bugs (e.g., drawing before transform)

**Shader Activation Side Effects:**
- `ShaderStack::top()` has side effects (activates shader, applies uniforms)
- Call once per draw, not multiple times
- Use `peek()` for inspection without activation

**Light Count Mismatch:**
- C++ `MAX_SCENE_LIGHTS` must match GLSL definition exactly
- Mismatch causes rendering artifacts or crashes
- Default is 16 lights

### Common Integration Pitfalls (From Testing)

These are real issues encountered during environment configuration testing:

**1. Missing SceneNodeBuilder Header:**
```cpp
// ❌ WRONG - Causes "incomplete type" errors
#include <EnGene.h>
scene::graph()->addNode("Node").with<Component>();  // Compilation error!

// ✅ CORRECT - Include the builder header
#include <EnGene.h>
#include <core/scene_node_builder.h>  // Required for fluent API
#include <components/all.h>
scene::graph()->addNode("Node").with<Component>();  // Works!
```

**2. Custom Shader Uniform Configuration:**
```cpp
// ❌ WRONG - u_model not configured for custom shaders
config.base_vertex_shader_source = custom_vertex_shader;
engene::EnGene app(on_init, on_update, on_render, config);
app.run();  // Warning: u_model not configured!

// ✅ CORRECT - Configure u_model after construction
engene::EnGene app(on_init, on_update, on_render, config);

// Can also be configured in the on_init lambda
auto shader = app.getBaseShader();
shader->configureDynamicUniform<glm::mat4>("u_model", transform::current);
app.run();
```

**3. Missing Buffer Clear in Render Callback:**
```cpp
// ❌ WRONG - Black screen, buffers not cleared
auto on_render = [](double alpha) {
    scene::graph()->draw();  // Nothing visible!
};

// ✅ CORRECT - Always clear buffers
auto on_render = [](double alpha) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    scene::graph()->draw();
    GL_CHECK("render");  // Optional but recommended
};
```

**4. CMake Generator on Windows:**
```bash
# ❌ WRONG - Default generator may fail on MinGW systems
cmake -B build

# ✅ CORRECT - Specify MinGW Makefiles
cmake -B build -G "MinGW Makefiles"
```


---

## Development Workflow

### Commit Message Format

Follow **Conventional Commits** specification:

```
<type>(<scope>): <subject>

<body>

<footer>
```

**Types:**
- `feat:` - New feature
- `fix:` - Bug fix
- `docs:` - Documentation changes
- `style:` - Code style changes (formatting, no logic change)
- `refactor:` - Code refactoring
- `perf:` - Performance improvements
- `test:` - Adding or updating tests
- `chore:` - Build process or auxiliary tool changes

**Examples:**
```
feat(shader): add support for geometry shaders

fix(transform): correct matrix multiplication order in TransformStack

docs(readme): update build instructions for macOS

refactor(component): simplify priority-based execution logic
```

### Branching Strategy

- **main** - Stable, production-ready code
- **develop** - Integration branch for features
- **feature/<name>** - Feature development branches
- **bugfix/<name>** - Bug fix branches
- **hotfix/<name>** - Critical fixes for production

**Workflow:**
1. Create feature branch from `develop`
2. Implement feature with regular commits
3. Open pull request to `develop`
4. Code review and approval
5. Merge to `develop`
6. Periodically merge `develop` to `main` for releases

### Code Review Checklist

- [ ] No .cpp files created for library code (header-only requirement)
- [ ] All functions properly marked `inline` or defined in class body
- [ ] Follows naming conventions
- [ ] Uses RAII for resource management
- [ ] No raw OpenGL calls in application code
- [ ] Proper use of smart pointers
- [ ] Component priorities set correctly
- [ ] Shader uniforms validated
- [ ] Error handling implemented
- [ ] No memory leaks (checked with valgrind/sanitizers)
- [ ] Documentation updated (if API changed)
- [ ] Examples updated (if applicable)

### Pull Request Process

1. **Create Branch:** Branch from `develop` with descriptive name
2. **Implement Changes:** Make commits following commit message format
3. **Test Thoroughly:** Run example applications, check for errors
4. **Update Documentation:** Update README.md and AGENTS.md if needed
5. **Open PR:** Provide clear description and testing notes
6. **Address Feedback:** Respond to review comments, make requested changes
7. **Merge:** Squash and merge once approved

### Development Steps

1. **Setup:** Clone repository, initialize submodules, build dependencies
2. **Feature Development:** Create branch, implement feature, test with examples
3. **Testing:** Run example applications, check for OpenGL errors
4. **Documentation:** Update README.md and AGENTS.md if needed
5. **Commit:** Follow commit message format
6. **Pull Request:** Open PR with description and testing notes
7. **Review:** Address feedback, make changes
8. **Merge:** Squash and merge to target branch

---

**Last Updated:** 2025-11-04  
**Version:** 1.0  
**Maintainer:** EnGene Project Team

