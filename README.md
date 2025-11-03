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

---

# Project Architecture

## Main Classes & Entities

### Core Architecture

**Node<T>**
Generic tree structure implementing the Composite pattern with visitor support. Templated on payload type for flexibility. Each node maintains parent/child relationships via shared/weak pointers, stores custom pre/post-visit actions, and provides traversal methods. Supports operations like `addChild()`, `removeChild()`, `visit()`, and applicability toggling for conditional rendering. The visitor pattern allows behavior injection without modifying the node class itself.

**SceneGraph**
Meyers singleton managing the entire scene hierarchy. Maintains the root node, a name-based registry for fast lookups, an ID-based registry for unique identification, and the active camera reference. Provides the `addNode()` entry point for the builder pattern and `buildAt()` for continuing builds from existing nodes. Automatically creates a default orthographic camera on construction. Handles node registration, renaming, removal, and duplication. The `draw()` method initiates scene traversal from the root.

**SceneNode**
Type alias for `Node<ComponentCollection>` - a node specialized to carry components as its payload. This is the fundamental building block of the scene graph. Each SceneNode can have multiple components that define its behavior (transform, geometry, shader, etc.). The node's `visit()` method triggers component `apply()` before visiting children and `unapply()` after, creating a stack-based state machine.

**SceneNodeBuilder**
Fluent interface for declarative scene construction. Wraps a SceneNode and provides chainable methods: `with<T>()` adds unnamed components, `withNamed<T>()` adds named components, and `addNode()` creates child nodes returning a new builder. Supports implicit conversion to `SceneNode&` and `SceneNodePtr` for seamless integration. The builder pattern eliminates manual node creation and registration boilerplate.

---

### Component System (ECS-inspired)

**Component**
Abstract base class for all components. Defines the component lifecycle with `apply()` (called on scene traversal entry) and `unapply()` (called on exit). Each component has a unique runtime ID, an optional name for retrieval, a priority value determining execution order, and a weak reference to its owner node. Uses the Strategy pattern - subclasses define specific behaviors. Priority ranges: Transform (100), Shader (200), Appearance (300), Camera (400), Geometry (500), Custom (600+).

**ComponentCollection**
Container managing all components on a node. Maintains three internal data structures: a type-indexed map for fast type-based queries, a name-indexed map for named component lookup, and a priority-sorted vector for ordered execution. Provides `get<T>()` for retrieving single components, `getAll<T>()` for retrieving all components of a type (including derived types via dynamic casting), and removal methods by instance, name, or ID. The `apply()` and `unapply()` methods iterate the sorted vector in forward/reverse order respectively.

**TransformComponent**
Wraps a `Transform` object and pushes its matrix onto the transform stack during `apply()`, pops during `unapply()`. Supports custom priority within a validated range (0 to CAMERA priority) to allow multiple transforms per node with controlled ordering. This enables complex hierarchies like orbit + rotation. Provides `getTransform()`, `setTransform()`, `getMatrix()`, and `setMatrix()` accessors. Multiple TransformComponents on a single node compose in priority order.

**GeometryComponent**
Wraps a `Geometry` object and calls its `Draw()` method during `apply()`. Has the highest default priority (500) to ensure drawing happens after all state setup. The geometry itself contains VAO/VBO/EBO handles and vertex attribute configuration. GeometryComponent is the final step in the rendering pipeline - by the time it executes, transforms, shaders, textures, and materials are already configured.

**ShaderComponent**
Overrides the default shader for a node and its subtree. Pushes the custom shader onto the shader stack during `apply()`, pops during `unapply()`. This allows per-object shader customization (e.g., textured objects, lit objects, special effects). The shader stack's lazy activation means the shader isn't actually bound until `shader::stack()->top()` is called by a GeometryComponent.

**TextureComponent**
Binds a texture to a specific texture unit and registers the sampler name with the texture stack. During `apply()`, pushes the texture onto the stack and registers the sampler-to-unit mapping. During `unapply()`, pops the texture and unregisters the sampler. This enables shader uniforms to dynamically query the correct texture unit via `texture::getUnitProvider()`. Supports multiple textures per node on different units.

**MaterialComponent**
Pushes a material onto the material stack during `apply()`, pops during `unapply()`. Materials define PBR properties (ambient, diffuse, specular, shininess) that merge hierarchically. Child nodes inherit parent material properties but can override specific values. The material stack uses provider functions to supply uniform values, enabling dynamic material changes without shader reconfiguration.

**LightComponent**
Integrates lights into the scene graph with automatic transform inheritance. Wraps a `Light` object and a local transform. Inherits from `ObservedTransformComponent` to track world-space position/direction. Automatically registers with the `LightManager` on construction and unregisters on destruction. The manager collects all active lights and uploads them to a UBO each frame. Supports directional, point, and spot lights.

**Camera**
Abstract base class for all camera types. Manages aspect ratio, provides pure virtual methods for `getViewMatrix()` and `getProjectionMatrix()`, and automatically creates a UBO for camera matrices. The UBO is configured with a provider function that queries the camera's matrices each frame. Provides `bindToShader()` to register the camera's UBO with shaders. Concrete implementations include `OrthographicCamera` and `PerspectiveCamera`.

**ObservedTransformComponent**
Extends `TransformComponent` and implements the `IObserver` interface. Caches the world-space transformation matrix by observing the transform stack. When the transform changes (via the observer pattern), updates the cached matrix. This is more efficient than recalculating world transforms every frame. Used by lights and cameras that need world-space positions/directions for rendering calculations.

---

### Rendering System

**Shader**
Manages GLSL shader programs with a sophisticated 4-tier uniform system. Tier 1 (Global Resources): UBOs bound at link time, shared across shaders. Tier 2 (Static): Applied once when shader activates. Tier 3 (Dynamic): Applied every draw call via provider functions. Tier 4 (Immediate): Set directly, queued if shader inactive. Supports loading from files or raw strings. Implements "Just-in-Time" linking with the `Bake()` method that links the program and refreshes uniform locations. Validates uniform types against GLSL declarations.

**ShaderStack**
Singleton managing active shader state with lazy activation. Maintains a stack of shaders and tracks the last used shader to prevent redundant `glUseProgram()` calls. The `top()` method activates the shader only if different from the last used one, then applies dynamic (Tier 3) uniforms. This means `top()` has side effects and should be called once per draw. The base shader (created on construction) cannot be popped, ensuring a valid shader is always available.

**Geometry**
Low-level wrapper for OpenGL vertex data. Encapsulates VAO, VBO, and EBO handles. Constructor takes vertex data, indices, position size, and additional attribute sizes (colors, UVs, normals). Automatically configures vertex attribute pointers based on the provided sizes. The `Draw()` method binds the VAO and calls `glDrawElements()`. Destructor automatically frees GPU resources. Supports any vertex layout via the flexible attribute size vector.

**Transform**
Represents a 4x4 transformation matrix with chainable operations. Implements the observer pattern via `ISubject` - notifies observers when the matrix changes. Provides methods like `translate()`, `rotate()`, `scale()`, `multiply()`, and their "set" variants (which reset first). All modification methods call `notify()` to alert observers. Factory method `Make()` creates shared pointers. Used by TransformComponent to define local transformations.

**TransformStack**
Singleton managing hierarchical transformation accumulation. Stores accumulated matrices (not individual transforms). Each `push()` multiplies the incoming matrix with the current top and pushes the result. The `current()` function returns the top matrix, used by shaders as the model matrix. Cannot pop below the identity matrix at index 0. This design eliminates the need for matrix inversion during pop operations.

**Material**
Data container for material properties stored as `std::variant<float, int, vec2, vec3, vec4, mat3, mat4>`. Provides type-safe `set<T>()` method and convenience methods for PBR properties (`setAmbient()`, `setDiffuse()`, `setSpecular()`, `setShininess()`). Supports uniform name remapping via `setUniformNameOverride()` for shader compatibility. Factory method `Make(vec3)` creates PBR materials from a base color. Properties are stored in a map for flexible querying.

**MaterialStack**
Singleton managing hierarchical material property merging. Each level stores a complete merged property map (not individual materials). `push()` copies the current state and overwrites properties with matching names. Provides `getValue<T>()` for immediate access and `getProvider<T>()` for creating uniform provider functions. The `configureShaderDefaults()` method automatically binds all base state properties to a shader using dynamic uniforms. Cannot pop below the base PBR state at index 0.

**Texture**
Manages OpenGL 2D textures with automatic resource management. Uses STB Image for loading. Constructor loads image data, creates texture, configures wrapping/filtering, uploads to GPU, and generates mipmaps. Implements a static cache (`s_cache`) - multiple `Make()` calls with the same filename return the same texture instance, preventing duplicate GPU uploads. Provides `Bind()` and `Unbind()` methods for texture unit management. Destructor calls `glDeleteTextures()`.

**TextureStack**
Singleton managing texture bindings across multiple units with intelligent GPU state tracking. Each level stores a complete map of `{unit -> texture}` bindings. Tracks actual GPU state in `m_active_gpu_state` to prevent redundant `glBindTexture()` calls. `push()` only binds if the GPU state differs. `pop()` intelligently restores the previous state, rebinding only changed units. Maintains `m_sampler_to_unit_map` for dynamic sampler-to-unit resolution used by shader uniforms.

---

### Lighting System

**Light**
Abstract base class for all light types. Stores common properties (ambient, diffuse, specular colors) and provides the pure virtual method `getType()` for runtime type identification. Provides getter methods for accessing color properties: `getAmbient()`, `getDiffuse()`, `getSpecular()`. Lights are created using designated initializer syntax with parameter structs (`LightParams`, `DirectionalLightParams`, etc.) for clarity. Subclasses implement specific light behaviors (directional, point, spot). The Light class itself does not handle GPU data conversion - this is done by the LightManager.

**DirectionalLight**
Represents infinite-distance lights (like the sun). Stores a `base_direction` vector in local space that gets transformed to world space by the LightManager using the LightComponent's world transform. No attenuation since the light is infinitely far away. Provides `getBaseDirection()` to access the local-space direction. Created via `DirectionalLight::Make(DirectionalLightParams)`. Useful for outdoor scenes and global illumination.

**PointLight**
Omnidirectional light with position and attenuation. Stores `position` (as vec4 with w=1) in local space and attenuation coefficients (constant, linear, quadratic). Light intensity falls off with distance according to the formula: `1.0 / (constant + linear * d + quadratic * d²)`. Provides getters: `getPosition()`, `getConstant()`, `getLinear()`, `getQuadratic()`. The LightManager transforms the position to world space. Created via `PointLight::Make(PointLightParams)`. Useful for lamps, torches, and localized lighting.

**SpotLight**
Cone-shaped light inheriting from PointLight. Adds `base_direction` (local space) and `cutOffAngle` (stored as cosine for efficient GPU comparison). Combines point light attenuation with directional constraints. Light intensity is full within the cone and zero outside. Provides `getBaseDirection()` and `getCutoffAngle()`. The LightManager transforms both position and direction to world space. Created via `SpotLight::Make(SpotLightParams)`. Useful for flashlights, stage lights, and focused illumination.

**LightManager**
Templated singleton (`LightManagerImpl<MAX_LIGHTS>`) collecting all active lights and uploading them to a GPU UBO. Maintains a registry of `LightComponent` pointers. The `registerLight()` and `unregisterLight()` methods are called automatically by LightComponent's constructor/destructor. The `apply()` method iterates all registered components, extracts light data using getters, applies world transforms from the component's cached world matrix, packs data into a `SceneLights<MAX_LIGHTS>` structure, and triggers GPU upload via `uniform::manager().applyShaderResource()`. Uses `dynamic_cast` to determine light types at runtime. Supports up to `MAX_SCENE_LIGHTS` (configurable via `light_config.h`, default 16) simultaneous lights. Provides `bindToShader()` to register the "SceneLights" UBO with shaders.

**Customizing Maximum Lights:**
To change the maximum number of lights supported, define `MAX_SCENE_LIGHTS` before including EnGene headers:

```cpp
// In your main.cpp or project header
#define MAX_SCENE_LIGHTS 32  // Increase to 32 lights
#include <EnGene.h>

// Rest of your code...
```

You must also update your GLSL shader to match:
```glsl
// In your shader
#define MAX_SCENE_LIGHTS 32  // Must match C++ value

layout (std140) uniform SceneLights {
    LightData lights[MAX_SCENE_LIGHTS];
    int active_light_count;
};
```

**Important:** The C++ and GLSL values must match exactly, or you'll get rendering artifacts or crashes. Default is 16 lights. Increasing this value increases GPU memory usage proportionally (~112 bytes per light in the UBO).

---

### Application Framework

**EnGene**
Main application class orchestrating the entire engine. Manages GLFW window, OpenGL context (4.3 Core), base shader, input handler, and the game loop. Constructor initializes all systems, creates a default camera, and binds it to the base shader. The `run()` method implements a fixed-timestep loop: accumulates time, runs `on_fixed_update` in fixed steps, calculates interpolation alpha, pushes base shader, applies per-frame UBOs, calls `on_render`, pops shader, swaps buffers. Prevents "spiral of death" with `maxFrameTime` cap. Non-copyable.

**EnGeneConfig**
Plain struct for engine configuration with sensible defaults. Fields: `width` (800), `height` (600), `title` ("EnGene Application"), `updatesPerSecond` (60), `maxFrameTime` (0.25), `clearColor` (black), `base_vertex_shader_source`, `base_fragment_shader_source`. All fields are optional - omitted fields use defaults. Supports both file paths and raw GLSL strings for shaders. Designed for aggregate initialization in C++20 or manual field assignment in C++17.

**InputHandler**
Manages GLFW input callbacks with type-safe registration. Supports keyboard, mouse button, mouse movement, and scroll callbacks. Uses template-based `registerCallback<InputType>()` method with predefined macros for callback signatures (e.g., `KEY_HANDLER_ARGS`). Callbacks are stored in maps and applied to the GLFW window via `applyCallbacks()`. Allows multiple callbacks per input type. Provides a clean abstraction over GLFW's C-style callback system.

## Design Patterns

### 1. Entity-Component System (ECS)
**Implementation:** Nodes act as entities, ComponentCollection as the component container, and components define behavior through `apply()`/`unapply()` methods.

**Used in:**
- SceneNode contains ComponentCollection as payload
- All component types (TransformComponent, GeometryComponent, etc.)
- ComponentCollection manages component lifecycle and retrieval

### 2. Visitor Pattern
**Implementation:** Nodes store pre/post-visit lambdas and execute them during traversal. The `visit()` method implements depth-first traversal with configurable actions.

**Used in:**
- Node<T>::visit() for scene graph traversal
- SceneGraph::draw() initiates visitor traversal from root
- Components use pre-visit for state setup (apply) and post-visit for cleanup (unapply)

### 3. Builder Pattern
**Implementation:** SceneNodeBuilder wraps a node and provides chainable methods that return `*this` or a new builder for children.

**Used in:**
- SceneNodeBuilder for declarative scene construction
- Fluent API: `addNode().with<T>().with<T>().addNode()`
- SceneGraph::addNode() and buildAt() return builders

### 4. Singleton Pattern (Meyers)
**Implementation:** Private constructor, deleted copy/assignment, static instance returned by friend function. Thread-safe in C++11+.

**Used in:**
- SceneGraph via `scene::graph()`
- ShaderStack via `shader::stack()`
- TransformStack via `transform::stack()`
- MaterialStack via `material::stack()`
- TextureStack via `texture::stack()`
- LightManager via `light::manager()`
- GlobalResourceManager via `uniform::manager()`

### 5. Strategy Pattern
**Implementation:** Components encapsulate algorithms (behaviors) that can be swapped at runtime. The node delegates to components via polymorphic `apply()`/`unapply()` calls.

**Used in:**
- Component base class with virtual apply/unapply methods
- Different component types implement different strategies
- Nodes execute component strategies during traversal

### 6. Observer Pattern
**Implementation:** Transform implements ISubject with `notify()` method. ObservedTransformComponent implements IObserver with `onNotify()` callback. Subjects maintain observer lists.

**Used in:**
- Transform notifies observers on matrix changes
- ObservedTransformComponent observes transforms to cache world matrices
- LightComponent uses observation to track light positions/directions

### 7. Factory Pattern (Static Factory Method)
**Implementation:** Private/protected constructors with public static `Make()` methods that return shared pointers. Enables controlled object creation and initialization.

**Used in:**
- All major classes: `Shader::Make()`, `Transform::Make()`, `Material::Make()`
- Component creation: `TransformComponent::Make()`
- Geometry and shape creation
- Ensures proper shared_ptr usage and initialization

### 8. Stack Pattern (Hierarchical State Machine)
**Implementation:** Vector-based stacks with push/pop operations. Each level stores accumulated or merged state. Protected base level prevents underflow.

**Used in:**
- TransformStack for matrix accumulation
- ShaderStack for shader state management
- MaterialStack for property merging
- TextureStack for texture unit bindings

### 9. Resource Sharing (UBO Pattern)
**Implementation:** Uniform Buffer Objects store data shared across multiple shaders. Bound to binding points at link time, updated once per frame.

**Used in:**
- Camera matrices (view/projection) shared by all shaders
- Scene lights array shared by lit shaders
- GlobalResourceManager coordinates UBO bindings
- Tier 1 uniforms in the 4-tier system

### 10. Priority-Based Execution
**Implementation:** Components have numeric priorities. ComponentCollection sorts by priority and executes in order during apply, reverse order during unapply.

**Used in:**
- Component execution order: Transform (100) → Shader (200) → Material (300) → Camera (400) → Geometry (500)
- Ensures transforms are set before drawing
- Ensures shaders are bound before materials
- Custom priorities allow fine-grained control

## Project Structure

```
EnGene/
├── core_gene/
│   ├── src/
│   │   ├── EnGene.h                           # Main engine class
│   │   ├── core/
│   │   │   ├── node.h                         # Generic tree node
│   │   │   ├── scene.h                        # Scene graph singleton
│   │   │   ├── scene_node_builder.h           # Fluent builder API
│   │   │   └── EnGene_config.h                # Configuration struct
│   │   ├── components/                        # ECS components
│   │   │   ├── all.h                          # Include all components
│   │   │   ├── component.h                    # Base component
│   │   │   ├── component_collection.h         # Component container
│   │   │   ├── transform_component.h          # Transform management
│   │   │   ├── observed_transform_component.h # Transform with observer
│   │   │   ├── geometry_component.h           # Geometry wrapper
│   │   │   ├── shader_component.h             # Shader override
│   │   │   ├── texture_component.h            # Texture binding
│   │   │   ├── material_component.h           # Material properties
│   │   │   └── light_component.h              # Light integration
│   │   ├── gl_base/                           # OpenGL abstractions
│   │   │   ├── gl_includes.h                  # OpenGL headers
│   │   │   ├── error.h                        # Error handling
│   │   │   ├── shader.h                       # Shader with 4-tier uniforms
│   │   │   ├── i_shader.h                     # Shader interface
│   │   │   ├── geometry.h                     # VAO/VBO/EBO wrapper
│   │   │   ├── transform.h                    # Matrix operations
│   │   │   ├── material.h                     # Material properties
│   │   │   ├── texture.h                      # Texture management
│   │   │   ├── input_handler.h                # Input callback system
│   │   │   └── uniforms/                      # Uniform system
│   │   │       ├── uniform.h                  # Base uniform
│   │   │       ├── ubo.h                      # Uniform Buffer Objects
│   │   │       ├── array_ssbo.h               # Array Shader Storage
│   │   │       ├── struct_ssbo.h              # Struct Shader Storage
│   │   │       ├── struct_resource.h          # Struct resource base
│   │   │       ├── shader_resource.h          # Resource interface
│   │   │       ├── global_resource_manager.h  # Resource manager
│   │   │       └── pending_uniform_command.h  # Deferred uniform setting
│   │   ├── 3d/
│   │   │   ├── camera/                        # Camera implementations
│   │   │   │   ├── camera.h                   # Abstract camera base
│   │   │   │   ├── camera3d.h                 # 3D camera base
│   │   │   │   ├── orthographic_camera.h      # Orthographic projection
│   │   │   │   └── perspective_camera.h       # Perspective projection
│   │   │   └── lights/                        # Light types & manager
│   │   │       ├── light.h                    # Abstract light base
│   │   │       ├── directional_light.h        # Directional light
│   │   │       ├── point_light.h              # Point light
│   │   │       ├── spot_light.h               # Spot light
│   │   │       ├── light_config.h             # Light configuration
│   │   │       ├── light_data.h               # Light data structures
│   │   │       └── light_manager.h            # Light collection & UBO
│   │   ├── other_genes/                       # Prebuilt shapes & utilities
│   │   │   ├── grid.h                         # Grid helper
│   │   │   ├── shapes/                        # 2D shapes
│   │   │   │   ├── circle.h                   # Circle geometry
│   │   │   │   └── polygon.h                  # Polygon geometry
│   │   │   ├── textured_shapes/               # Textured 2D shapes
│   │   │   │   ├── quad.h                     # Textured quad
│   │   │   │   └── textured_circle.h          # Textured circle
│   │   │   ├── 3d_shapes/                     # 3D shapes
│   │   │   │   ├── cube.h                     # Cube geometry
│   │   │   │   ├── sphere.h                   # Sphere geometry
│   │   │   │   └── cylinder.h                 # Cylinder geometry
│   │   │   └── input_handlers/                # Input handler presets
│   │   │       ├── basic_input_handler.h      # Basic input
│   │   │       ├── arcball_controller.h       # Arcball camera control
│   │   │       └── arcball_input_handler.h    # Arcball input handler
│   │   ├── utils/
│   │   │   └── observer_interface.h           # Observer pattern interface
│   │   └── exceptions/                        # Custom exceptions
│   │       ├── base_exception.h               # Base exception
│   │       ├── shader_exception.h             # Shader errors
│   │       └── node_not_found_exception.h     # Scene graph errors
│   ├── shaders/                               # GLSL shader files
│   │   ├── vertex.glsl                        # Basic vertex shader
│   │   ├── fragment.glsl                      # Basic fragment shader
│   │   ├── textured_vertex.glsl               # Textured vertex shader
│   │   ├── textured_fragment.glsl             # Textured fragment shader
│   │   ├── lit_vertex.glsl                    # Lit vertex shader
│   │   ├── lit_fragment.glsl                  # Lit fragment shader
│   │   ├── lit_vertex2.glsl                   # Alt lit vertex shader
│   │   └── lit_fragment2.glsl                 # Alt lit fragment shader
│   ├── example_main.cpp                       # Solar system demo with lighting
│   ├── working_example_main.cpp               # Physics simulation demo
│   └── debug_main.cpp                         # Debug/testing main
├── libs/                                      # External libraries (GLFW, GLAD, GLM, STB)
│   └── .gitkeep
├── build/                                     # Build output directory
│   └── .gitkeep
├── CMakeLists.txt                             # CMake build configuration
├── .gitignore                                 # Git ignore rules
└── README.md                                  # This file
```

## Key Features

### Declarative Scene Building
Build complex scene hierarchies using an intuitive fluent API that reads like natural language:
```cpp
scene::graph()->addNode("Sun")
    .with<component::TransformComponent>(sun_transform)
    .with<component::GeometryComponent>(circle_geometry)
    .addNode("Earth")
        .with<component::TransformComponent>(earth_orbit)
        .with<component::TextureComponent>(earth_texture);
```
The builder pattern eliminates boilerplate and makes scene construction self-documenting.

### Component Priority System
Components execute in a well-defined order based on priority values, ensuring correct rendering:
- **Transform (100)** - Applied first to set up the model matrix
- **Shader (200)** - Shader selection happens before appearance
- **Material/Texture (300)** - Appearance properties configured
- **Camera (400)** - View/projection setup
- **Geometry (500)** - Drawing happens last
- **Custom Scripts (600)** - User logic runs after rendering

This eliminates common bugs where transforms are applied after geometry or shaders are bound in the wrong order.

### 4-Tier Uniform System
Optimized GPU data management with four distinct uniform tiers:

**Tier 1: Global Resources (UBOs)**
- Bound once at shader link time
- Shared across all shaders (camera matrices, scene lights)
- Zero per-draw overhead
```cpp
// Automatically managed by Camera and LightManager
uniform::UBO<CameraMatrices>::Make("CameraMatrices", ...);
```

**Tier 2: Static Uniforms (Per-Use)**
- Applied when shader becomes active
- For values that don't change during shader use (texture units)
```cpp
shader->configureStaticUniform<int>("tex", texture::getUnitProvider("tex"));
```

**Tier 3: Dynamic Uniforms (Per-Draw)**
- Applied every draw call via provider functions
- For frequently changing values (model matrix, material properties)
```cpp
shader->configureDynamicUniform<glm::mat4>("u_model", transform::current);
```

**Tier 4: Immediate Uniforms (Manual)**
- Set directly for one-off values
- Queued if shader is inactive, applied immediately if active
```cpp
shader->setUniform<float>("u_time", elapsed_time);
```

### Hierarchical Transforms
Transforms automatically accumulate through the scene graph hierarchy:
```cpp
// Parent rotation affects all children
scene::graph()->addNode("SolarSystem")
    .with<component::TransformComponent>(system_rotation)
    .addNode("Planet")
        .with<component::TransformComponent>(orbit_transform)
        .addNode("Moon")
            .with<component::TransformComponent>(moon_orbit);
// Moon inherits: system_rotation * orbit_transform * moon_orbit
```
The transform stack manages this automatically during scene traversal.

### Material Stack
Materials merge hierarchically, allowing property inheritance and overrides:
```cpp
// Base material for all objects
material::stack()->push(Material::Make(glm::vec3(0.8f)));

// Override specific properties for a subtree
material::stack()->push(Material::Make()->setSpecular(glm::vec3(1.0f)));
// ... render shiny objects ...
material::stack()->pop(); // Restore base material
```
Child nodes inherit parent materials but can override individual properties.

### Fixed-Timestep Loop
Separates simulation from rendering for consistent physics and smooth visuals:
```cpp
auto on_fixed_update = [](double dt) {
    // Physics runs at fixed 60 FPS regardless of frame rate
    physics_engine->update(dt);
};

auto on_render = [](double alpha) {
    // Rendering runs as fast as possible
    // alpha allows interpolation between physics states
    scene::graph()->draw();
};
```
Prevents the "spiral of death" where slow frames cause even slower updates.

### Type-Safe Components
Retrieve components with full type safety and inheritance support:
```cpp
// Get first unnamed component of type
auto transform = node->payload().get<TransformComponent>();

// Get component by name
auto light = node->payload().get<LightComponent>("MainLight");

// Get all components of type (including derived types)
auto all_transforms = node->payload().getAll<TransformComponent>();
```
Dynamic casting handles inheritance automatically, so requesting a base type returns derived types too.

### Automatic Resource Management
All OpenGL resources use RAII for automatic cleanup:
- **Shaders** - Programs deleted on destruction
- **Geometry** - VAO/VBO/EBO automatically freed
- **Textures** - GPU textures released when no longer referenced
- **UBOs** - Uniform buffers cleaned up automatically

No manual `glDelete*` calls needed - resources are freed when smart pointers go out of scope.

### Advanced Lighting System
Unified lighting with automatic GPU upload via UBOs:
```cpp
// Create lights with intuitive configuration
auto sun = light::PointLight::Make({
    .position = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
    .diffuse = glm::vec4(1.0f, 0.9f, 0.7f, 1.0f),
    .constant = 1.0f,
    .linear = 0.09f,
    .quadratic = 0.032f
});

// Attach to scene graph for transform inheritance
node->payload().addComponent(
    component::LightComponent::Make(sun, light_transform)
);
```
Supports directional, point, and spot lights with automatic world-space transformation.

### Observer Pattern for Transforms
Components can observe transform changes for reactive updates:
```cpp
class ObservedTransformComponent : public TransformComponent, public IObserver {
    // Automatically notified when transform changes
    void onNotify() override {
        m_cached_world_matrix = transform::current();
    }
};
```
Used by lights and cameras to cache world-space positions efficiently.


---

## Understanding the Stack Systems

EnGene uses several stack-based systems for managing hierarchical state. Each stack has unique behaviors that may differ from typical expectations.

### Transform Stack
**Location:** `transform::stack()`

**Purpose:** Accumulates transformation matrices through scene graph hierarchy.

**Key Behavior:**
- Stores **accumulated** matrices, not individual transforms
- Each level contains the product of all parent transforms
- `push()` multiplies the new matrix with the current top: `new_state = current_top * incoming_matrix`
- `pop()` simply removes the top level (no matrix inversion needed)

**Important Note:** Unlike other stacks, you cannot "peek" at individual transform components - you only get the final accumulated result via `transform::current()`.

```cpp
transform::stack()->push(parent_matrix);     // Stack: [identity * parent]
transform::stack()->push(child_matrix);      // Stack: [identity * parent, parent * child]
glm::mat4 final = transform::current();      // Returns: parent * child
transform::stack()->pop();                   // Back to: [identity * parent]
```

**Protected Base:** Cannot pop below the identity matrix at index 0.

---

### Shader Stack
**Location:** `shader::stack()`

**Purpose:** Manages active shader state with lazy activation and automatic uniform application.

**Key Behavior:**
- `push()` adds shader to stack but **does NOT activate it**
- `top()` activates the shader **only if different** from last used shader
- `peek()` returns the top shader **without activating it or applying uniforms** (no side effects)
- Tracks `last_used_shader` to prevent redundant `glUseProgram()` calls
- Automatically applies Tier 2 (static) and Tier 4 (queued) uniforms on activation
- Applies Tier 3 (dynamic) uniforms **every time** `top()` is called

**Unexpected Behavior:** Calling `top()` multiple times has side effects - it re-applies dynamic uniforms each time. This is intentional for per-draw updates. Use `peek()` when you need to inspect the shader without triggering activation.

```cpp
shader::stack()->push(custom_shader);        // Shader added but NOT active yet
shader::stack()->top();                      // NOW shader activates + uniforms applied
shader::stack()->top();                      // Dynamic uniforms re-applied (no glUseProgram)
shader::stack()->pop();                      // Previous shader restored on next top()
```

**State Transition:** When switching shaders, the previous shader's `m_is_currently_active_in_GL` flag is set to false, and the new shader's flag is set to true. This enables the Tier 4 immediate uniform system to know whether to apply immediately or queue.

**Protected Base:** Cannot pop the base shader created during stack initialization.

---

### Texture Stack
**Location:** `texture::stack()`

**Purpose:** Manages texture bindings across multiple texture units with intelligent GPU state tracking.

**Key Behavior:**
- Stores **complete texture state** at each level (all active units)
- Each `push()` copies the previous state and modifies it
- Tracks actual GPU state in `m_active_gpu_state` to prevent redundant `glBindTexture()` calls
- `pop()` intelligently restores previous state, only rebinding changed units
- Maintains a `sampler_to_unit_map` for dynamic uniform binding

**Unexpected Behavior:** Unlike other stacks, texture stack stores maps of `{unit -> texture}` at each level, not individual textures. This means you can have multiple textures active simultaneously on different units.

```cpp
// Level 0: {}
texture::stack()->push(tex1, 0);             // Level 1: {0 -> tex1}
texture::stack()->push(tex2, 1);             // Level 2: {0 -> tex1, 1 -> tex2}
texture::stack()->push(tex3, 0);             // Level 3: {0 -> tex3, 1 -> tex2} (overrides unit 0)
texture::stack()->pop();                     // Back to: {0 -> tex1, 1 -> tex2}
                                             // Only unit 0 is rebound (tex3 -> tex1)
```

**Sampler Registration:** TextureComponent registers sampler names during `apply()` and unregisters during `unapply()`. The `getUnitProvider()` function returns a provider lambda that queries the texture stack for the current unit associated with a sampler name. This provider is used by shader uniforms to dynamically resolve texture units at render time.

**Protected Base:** Cannot pop below the empty base state at index 0.

---

### Material Stack
**Location:** `material::stack()`

**Purpose:** Manages material properties with hierarchical merging and provider-based uniform binding.

**Key Behavior:**
- Stores **merged property maps** at each level, not individual materials
- Each `push()` copies the previous state and **overwrites** matching property names
- Properties are stored as `std::variant` for type safety
- Provides `getValue<T>()` and `getProvider<T>()` for shader uniform binding
- Base state (index 0) contains default PBR properties

**Unexpected Behavior:** Materials don't "stack" additively - child properties completely override parent properties with the same name. This is different from transform multiplication.

```cpp
// Level 0: {ambient: (0.2, 0.2, 0.2), diffuse: (0.8, 0.8, 0.8), ...}
auto mat1 = Material::Make()->setDiffuse(glm::vec3(1.0, 0.0, 0.0));
material::stack()->push(mat1);
// Level 1: {ambient: (0.2, 0.2, 0.2), diffuse: (1.0, 0.0, 0.0), ...}
//          ^^^ ambient inherited, diffuse overridden

auto mat2 = Material::Make()->setSpecular(glm::vec3(1.0));
material::stack()->push(mat2);
// Level 2: {ambient: (0.2, 0.2, 0.2), diffuse: (1.0, 0.0, 0.0), specular: (1.0, 1.0, 1.0), ...}
//          ^^^ all previous properties inherited, specular overridden
```

**Provider Pattern:** Instead of directly setting uniforms, materials use provider functions that query the stack at render time. This enables dynamic property changes without reconfiguring shaders.

**Protected Base:** Cannot pop below the base material state at index 0.

---

## The EnGene Application Class

### Overview
`EnGene` is the main application class that orchestrates the entire engine. It manages the window, OpenGL context, game loop, and coordinates all major systems.

### Responsibilities

**1. Window & Context Management**
- Initializes GLFW and creates the OpenGL context (4.3 Core Profile)
- Enables OpenGL debug output for error tracking
- Manages window lifecycle and event polling

**2. Base Shader Setup**
- Creates and configures the engine's default shader
- Automatically binds the scene's default camera UBOs to the base shader
- Configures the `u_model` uniform to use `transform::current()`
- Pushes base shader onto shader stack during render loop

**3. Fixed-Timestep Game Loop**
- Implements the "Fix Your Timestep" pattern
- Separates simulation (`on_fixed_update`) from rendering (`on_render`)
- Prevents "spiral of death" with `maxFrameTime` cap
- Provides interpolation alpha to render callback

**4. Input Handling**
- Takes ownership of an `InputHandler` instance
- Applies input callbacks to the GLFW window

**5. Scene Graph Integration**
- Accesses the singleton `SceneGraph` to get the default camera
- Coordinates with the scene during initialization

### Relationships

```
EnGene
├── owns → GLFWwindow*
├── owns → shader::ShaderPtr (base_shader)
├── owns → input::InputHandler (unique_ptr)
├── uses → scene::SceneGraph (singleton)
│   └── provides → component::Camera (default camera)
├── uses → shader::ShaderStack (singleton)
├── uses → transform::TransformStack (singleton)
└── uses → uniform::GlobalResourceManager (singleton)
```

### Initialization Flow

1. **Constructor Phase:**
   - Parse `EnGeneConfig` struct
   - Initialize GLFW window and OpenGL context
   - Create base shader from config paths
   - Attach and bake base shader
   - Get default camera from SceneGraph (triggers SceneGraph singleton creation)
   - Bind camera UBOs to base shader
   - Re-bake shader to activate UBO bindings
   - Set clear color and enable depth testing

2. **Run Phase:**
   - Call user's `on_initialize` callback (scene construction happens here)
   - Enter game loop:
     - Accumulate elapsed time
     - Run `on_fixed_update` in fixed timesteps until caught up
     - Push base shader onto shader stack
     - Apply per-frame uniforms (UBOs)
     - Call user's `on_render` callback
     - Pop base shader from stack
     - Swap buffers and poll events

### Key Design Decisions

**Why pass `EnGene&` to `on_initialize`?**
Allows user code to access `getBaseShader()` for additional uniform configuration during setup.

**Why push/pop base shader every frame?**
Ensures the base shader is always at the bottom of the shader stack, providing a consistent default. User code can push custom shaders on top.

**Why separate `on_fixed_update` and `on_render`?**
Decouples physics/simulation (which needs consistent timesteps) from rendering (which should run as fast as possible). The `alpha` parameter in `on_render` allows interpolation between physics states for smooth visuals.

**Why create a default camera automatically?**
Prevents crashes from missing camera setup. The default orthographic camera provides a working view/projection immediately. Users can replace it with `scene::graph()->setActiveCamera()`.

### Configuration

The `EnGeneConfig` struct provides sensible defaults:
```cpp
struct EnGeneConfig {
    int width = 800;
    int height = 600;
    std::string title = "EnGene Application";
    int updatesPerSecond = 60;           // Fixed timestep rate
    double maxFrameTime = 0.25;          // Spiral of death prevention
    float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    std::string base_vertex_shader_source = DEFAULT_VERTEX_SHADER;
    std::string base_fragment_shader_source = DEFAULT_FRAGMENT_SHADER;
};
```

All fields are optional - omitted fields use defaults. This allows minimal configuration:
```cpp
engene::EnGene app(on_init, on_update, on_render); // Uses all defaults
```

Or selective overrides:
```cpp
engene::EnGeneConfig config;
config.width = 1920;
config.height = 1080;
config.title = "My Game";
engene::EnGene app(on_init, on_update, on_render, config);
```
