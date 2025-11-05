# EnGene - Modular Declarative C++ OpenGL Library

EnGene is a modular, declarative C++ OpenGL abstraction library for developers who want full control over their graphics rendering pipeline. Build complex 2D/3D scenes using an intuitive scene graph architecture with an ECS-inspired component system.

## Key Features

- **Declarative Scene Building** - Fluent API for intuitive scene construction
- **Scene Graph Architecture** - Hierarchical node system with automatic state management
- **Component System** - ECS-inspired design with priority-based execution
- **4-Tier Uniform System** - Optimized GPU data management (UBOs, static, dynamic, immediate)
- **Hierarchical Transforms** - Automatic matrix accumulation through scene hierarchy
- **Material Stack** - Property inheritance and overrides for flexible appearance control
- **Advanced Lighting** - Directional, point, and spot lights with automatic GPU upload
- **Fixed-Timestep Loop** - Separate simulation from rendering for consistent physics
- **Automatic Resource Management** - RAII-based cleanup for all OpenGL resources

> **For Contributors:** If you want to contribute to EnGene, see [AGENTS.md](AGENTS.md) for development guidelines, architecture details, and contribution standards.

---

## Quick Start

### Installation

EnGene is a **header-only library** - no compilation required, just include the headers!

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

# Link required dependencies (GLFW, GLAD, GLM, STB)
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
3. Add the include path to your CMakeLists.txt as shown above

**In your C++ code:**
```cpp
#include <EnGene.h>  // Main header with all functionality

// Or include specific headers:
#include "gl_base/shader.h"
#include "core/scene.h"
#include "components/all.h"
```

### Minimal Working Example

Here's a complete example that creates a window and renders a colored triangle:

```cpp
#include <EnGene.h>

int main() {
    // Define initialization callback
    auto on_initialize = [](engene::EnGene& app) {
        // Create a simple triangle geometry
        std::vector<float> vertices = {
            // positions        // colors
            0.0f,  0.5f,        1.0f, 0.0f, 0.0f,  // top (red)
           -0.5f, -0.5f,        0.0f, 1.0f, 0.0f,  // bottom-left (green)
            0.5f, -0.5f,        0.0f, 0.0f, 1.0f   // bottom-right (blue)
        };
        std::vector<unsigned int> indices = {0, 1, 2};
        
        // Geometry::Make(vertices, indices, num_vertices, num_indices, position_size, {other_attribute_sizes})
        auto triangle = geometry::Geometry::Make(
            vertices.data(), indices.data(), 
            3, 3,  // 3 vertices, 3 indices
            2,     // 2 floats for position (x, y)
            {3}    // 3 floats for color (r, g, b)
        );
        
        // Build scene graph
        scene::graph()->addNode("Triangle")
            .with<component::GeometryComponent>(triangle);
    };
    
    // Define fixed update callback (for physics/simulation)
    auto on_fixed_update = [](double dt) {
        // Update logic here (runs at fixed 60 FPS)
    };
    
    // Define render callback
    auto on_render = [](double alpha) {
        // Draw the scene
        scene::graph()->draw();
    };
    
    // Create and run the application (config is optional)
    engene::EnGene app(on_initialize, on_fixed_update, on_render);
    app.run();
    
    return 0;
}
```

### Evolved Example with Other Genes

Here's a more practical example using pre-built shapes from `other_genes/`:

```cpp
#include <EnGene.h>
#include "other_genes/shapes/circle.h"
#include "other_genes/3d_shapes/sphere.h"

int main() {
    auto on_initialize = [](engene::EnGene& app) {
        // Create materials inline
        auto red_material = material::Material::Make(glm::vec3(1.0f, 0.2f, 0.2f))
            ->setSpecular(glm::vec3(1.0f))
            ->setShininess(32.0f);
        
        auto blue_material = material::Material::Make(glm::vec3(0.2f, 0.2f, 1.0f))
            ->setSpecular(glm::vec3(0.5f))
            ->setShininess(16.0f);
        
        // Build scene with inline component creation
        scene::graph()->addNode("Sun")
            .with<component::TransformComponent>(
                transform::Transform::Make()->scale(2.0f, 2.0f, 2.0f)
            )
            .with<component::MaterialComponent>(red_material)
            .with<component::GeometryComponent>(
                Sphere::Make(1.0f, 32, 32)  // radius, segments, rings
            )
            .addNode("Planet")
                .with<component::TransformComponent>(
                    transform::Transform::Make()
                        ->translate(5.0f, 0.0f, 0.0f)
                        ->scale(0.8f, 0.8f, 0.8f)
                )
                .with<component::MaterialComponent>(blue_material)
                .with<component::GeometryComponent>(
                    Sphere::Make(1.0f, 24, 24)
                );
    };
    
    auto on_fixed_update = [](double dt) {
        // Rotate planet around sun
        static float angle = 0.0f;
        angle += dt * 30.0f;  // 30 degrees per second
        
        auto planet = scene::graph()->findNode("Planet");
        if (planet) {
            auto transform_comp = planet->payload().get<component::TransformComponent>();
            if (transform_comp) {
                transform_comp->getTransform()
                    ->setTranslate(5.0f * cos(glm::radians(angle)), 
                                   0.0f, 
                                   5.0f * sin(glm::radians(angle)));
            }
        }
    };
    
    auto on_render = [](double alpha) {
        scene::graph()->draw();
    };
    
    engene::EnGene app(on_initialize, on_fixed_update, on_render);
    app.run();
    
    return 0;
}
```

---

## Core Concepts

### Scene Graph

The scene graph is a hierarchical tree structure where each node can have components that define its behavior. Nodes automatically inherit transformations from their parents.

```cpp
// Access the singleton scene graph
scene::graph()->addNode("Parent")
    .with<component::TransformComponent>(parent_transform)
    .addNode("Child")
        .with<component::TransformComponent>(child_transform)
        .with<component::GeometryComponent>(geometry);
```

**Key Points:**
- Nodes are organized in a parent-child hierarchy
- Transformations accumulate down the tree
- Components define node behavior (rendering, transforms, etc.)
- Scene traversal happens automatically during `scene::graph()->draw()`

### Component System

Components are modular behaviors attached to scene nodes. Each component has a priority that determines execution order.

**Component Priorities:**
- **Transform (100)** - Applied first to set up transformations
- **Shader (200)** - Shader selection
- **Material/Texture (300)** - Appearance properties
- **Camera (400)** - View/projection setup
- **Geometry (500)** - Drawing happens last
- **Custom (600+)** - User-defined behaviors

```cpp
// Add components to a node
node.with<component::TransformComponent>(transform)
    .with<component::ShaderComponent>(custom_shader)
    .with<component::MaterialComponent>(material)
    .with<component::GeometryComponent>(geometry);
```

**Component Lifecycle:**
- `apply()` - Called when entering the node during scene traversal
- `unapply()` - Called when leaving the node (in reverse priority order)

### Transform Hierarchy

Transforms automatically accumulate through the scene graph, creating a hierarchical transformation system.

```cpp
// Parent rotation affects all children
scene::graph()->addNode("SolarSystem")
    .with<component::TransformComponent>(system_rotation)
    .addNode("Planet")
        .with<component::TransformComponent>(orbit_transform)
        .addNode("Moon")
            .with<component::TransformComponent>(moon_orbit);
// Moon's final transform: system_rotation * orbit_transform * moon_orbit
```

**Important:** The transform stack stores accumulated matrices. Each level contains the product of all parent transforms.

### Rendering Pipeline

The rendering pipeline follows a well-defined flow:

1. **Scene Traversal** - `scene::graph()->draw()` traverses the scene tree
2. **Component Application** - Each node applies its components in priority order
3. **State Setup** - Transforms, shaders, materials, and textures are configured
4. **Drawing** - Geometry components issue draw calls
5. **State Cleanup** - Components unapply in reverse order

```cpp
// Typical rendering flow in your render callback
auto on_render = [](double alpha) {
    scene::graph()->draw();  // Traverses and renders entire scene
};
```

---

## API Reference


### Scene Management

#### SceneGraph

The `SceneGraph` is a Meyers singleton that manages the entire scene hierarchy.

**Access:**
```cpp
scene::graph()  // Returns pointer to singleton instance
```

**Key Methods:**
```cpp
// Add a new root-level node and return a builder
SceneNodeBuilder addNode(const std::string& name);

// Continue building from an existing node
SceneNodeBuilder buildAt(const std::string& name);
SceneNodeBuilder buildAt(SceneNodePtr node);

// Find nodes by name or ID
SceneNodePtr findNode(const std::string& name);
SceneNodePtr findNodeById(unsigned int id);

// Camera management
void setActiveCamera(std::shared_ptr<component::Camera> camera);
std::shared_ptr<component::Camera> getActiveCamera();

// Render the entire scene
void draw();
```

**Example:**
```cpp
// Create a scene hierarchy
scene::graph()->addNode("Root")
    .with<component::TransformComponent>(root_transform)
    .addNode("Child1")
        .with<component::GeometryComponent>(geometry1)
    .addNode("Child2")
        .with<component::GeometryComponent>(geometry2);

// Later, continue building from an existing node
scene::graph()->buildAt("Child1")
    .addNode("Grandchild")
        .with<component::GeometryComponent>(geometry3);

// Render the scene
scene::graph()->draw();
```


#### SceneNode and SceneNodeBuilder

`SceneNode` is a type alias for `Node<ComponentCollection>` - the fundamental building block of the scene.

**SceneNodeBuilder** provides a fluent interface for declarative scene construction.

**Builder Methods:**
```cpp
// Add unnamed component
template<typename T, typename... Args>
SceneNodeBuilder& with(Args&&... args);

// Add named component
template<typename T, typename... Args>
SceneNodeBuilder& withNamed(const std::string& name, Args&&... args);

// Add child node
SceneNodeBuilder addNode(const std::string& name);

// Get the underlying node
SceneNode& getNode();
SceneNodePtr getNodePtr();
```

**Example:**
```cpp
// Declarative scene building
scene::graph()->addNode("Player")
    .with<component::TransformComponent>(player_transform)
    .withNamed<component::ShaderComponent>("PlayerShader", custom_shader)
    .with<component::GeometryComponent>(player_model)
    .addNode("Weapon")
        .with<component::TransformComponent>(weapon_offset)
        .with<component::GeometryComponent>(weapon_model);
```


### Component System

#### Component Base Class

All components inherit from the `Component` base class and implement the component lifecycle.

**Lifecycle Methods:**
```cpp
virtual void apply();    // Called when entering node during traversal
virtual void unapply();  // Called when leaving node (reverse order)
```

**Component Properties:**
- **ID** - Unique runtime identifier
- **Name** - Optional name for retrieval
- **Priority** - Determines execution order (lower = earlier)
- **Owner** - Weak reference to parent node

#### ComponentCollection

Container managing all components on a node.

**Key Methods:**
```cpp
// Get first component of type
template<typename T>
std::shared_ptr<T> get();

// Get named component
template<typename T>
std::shared_ptr<T> get(const std::string& name);

// Get all components of type (including derived types)
template<typename T>
std::vector<std::shared_ptr<T>> getAll();

// Add component
void addComponent(std::shared_ptr<Component> component);

// Remove component
void removeComponent(std::shared_ptr<Component> component);
void removeComponent(const std::string& name);
void removeComponent(unsigned int id);
```

**Example:**
```cpp
// Access node's component collection
auto& components = node->payload();

// Add components
components.addComponent(component::TransformComponent::Make(transform));

// Retrieve components
auto transform = components.get<component::TransformComponent>();
auto light = components.get<component::LightComponent>("MainLight");
auto all_transforms = components.getAll<component::TransformComponent>();
```


#### Core Components

**TransformComponent**

Manages node transformations and pushes/pops matrices on the transform stack.

```cpp
// Create inline during scene building (recommended)
scene::graph()->addNode("MyNode")
    .with<component::TransformComponent>(
        transform::Transform::Make()
            ->translate(1.0f, 0.0f, 0.0f)
            ->rotate(45.0f, 0.0f, 0.0f, 1.0f)  // degrees
            ->scale(2.0f, 2.0f, 2.0f)
    );

// Or create separately if you need to modify it later
auto transform = transform::Transform::Make();
node.with<component::TransformComponent>(transform);

// Custom priority (optional)
node.with<component::TransformComponent>(transform, 150);
```

**Priority:** 100 (default)

**Lifecycle Methods:**
- `apply()` - Pushes transform onto transform stack (used during scene traversal)
- `unapply()` - Pops transform from stack (used during scene traversal)

**ObservedTransformComponent**

A specialized transform component that caches its world transform and notifies observers when it changes. Essential for cameras and objects that need to track their world position.

```cpp
// Create observed transform for a camera or tracked object
scene::graph()->addNode("Camera")
    .with<component::ObservedTransformComponent>(
        transform::Transform::Make()->translate(0.0f, 5.0f, 10.0f)
    );

// Access cached world transform (efficient - no recalculation)
auto observed = node->payload().get<component::ObservedTransformComponent>();
const glm::mat4& world_transform = observed->getCachedWorldTransform();

// Force immediate update if needed (calculates on-demand)
const glm::mat4& updated = observed->getWorldTransform();
```

**Key Features:**
- **Caching:** Stores world transform to avoid redundant calculations
- **Observer Pattern:** Notifies dependents (like cameras) when transform changes
- **Automatic Updates:** Marks cache as dirty when local transform or parent transforms change
- **Efficient:** Only recalculates when needed

**Use Cases:**
- Camera positioning (cameras inherit from ObservedTransformComponent)
- Light positioning with transform inheritance
- Physics objects that need world position
- Any object that needs efficient world transform access

**Priority:** 100 (default, same as TransformComponent)

**GeometryComponent**

Wraps a `Geometry` object and issues draw calls.

```cpp
// Create inline during scene building (recommended)
scene::graph()->addNode("Triangle")
    .with<component::GeometryComponent>(
        geometry::Geometry::Make(
            vertices.data(), indices.data(),
            3, 3,  // 3 vertices, 3 indices
            3,     // position size
            {3}    // color size
        )
    );

// Or use pre-built shapes from other_genes
scene::graph()->addNode("Circle")
    .with<component::GeometryComponent>(
        Circle::Make(0.0f, 0.0f, 1.0f, color_rgb, 32, true)
    );
```

**Priority:** 500 (ensures drawing happens after all state setup)

**Lifecycle Methods:**
- `apply()` - Calls `geometry->Draw()` to render (used during scene traversal)

**ShaderComponent**

Overrides the default shader for a node and its subtree.

```cpp
// Create inline during scene building
scene::graph()->addNode("CustomShaded")
    .with<component::ShaderComponent>(
        shader::Shader::Make("vertex.glsl", "fragment.glsl")
    );
```

**Priority:** 200

**Lifecycle Methods:**
- `apply()` - Pushes shader onto shader stack (used during scene traversal)
- `unapply()` - Pops shader from stack (used during scene traversal)


**TextureComponent**

Binds a texture to a specific texture unit and registers the sampler name.

```cpp
// Create inline during scene building
scene::graph()->addNode("TexturedObject")
    .with<component::TextureComponent>(
        texture::Texture::Make("path/to/texture.png"),
        "u_texture",  // sampler name in shader
        0             // texture unit
    );
```

**Priority:** 300

**Lifecycle Methods:**
- `apply()` - Binds texture and registers sampler (used during scene traversal)
- `unapply()` - Unbinds texture and unregisters sampler (used during scene traversal)

**MaterialComponent**

Pushes material properties onto the material stack for hierarchical property inheritance.

```cpp
// Create inline during scene building
scene::graph()->addNode("RedObject")
    .with<component::MaterialComponent>(
        material::Material::Make(glm::vec3(0.8f, 0.2f, 0.2f))
            ->setSpecular(glm::vec3(1.0f))
            ->setShininess(32.0f)
    );
```

**Priority:** 300

**Lifecycle Methods:**
- `apply()` - Pushes material onto material stack (used during scene traversal)
- `unapply()` - Pops material from stack (used during scene traversal)

**LightComponent**

Integrates lights into the scene graph with automatic transform inheritance. Inherits from `ObservedTransformComponent` to track world position.

```cpp
// Create inline during scene building
scene::graph()->addNode("LightSource")
    .with<component::LightComponent>(
        light::PointLight::Make({
            .position = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
            .diffuse = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .constant = 1.0f,
            .linear = 0.09f,
            .quadratic = 0.032f
        }),
        transform::Transform::Make()  // local transform for the light
    );
```

**Priority:** 600 (CUSTOM_SCRIPT)

**Important Notes:**
- Automatically registers with `LightManager` on creation
- Automatically unregisters on destruction
- Inherits from `ObservedTransformComponent` for efficient world transform tracking
- No custom `apply()`/`unapply()` - uses inherited transform behavior


### Rendering System

#### Shader

Manages GLSL shader programs with a sophisticated 4-tier uniform system.

**Creation:**
```cpp
// From files (automatic attach and link)
auto shader = shader::Shader::Make("vertex.glsl", "fragment.glsl");

// From raw strings (automatic attach and link)
auto shader = shader::Shader::Make(vertex_source, fragment_source);

// Manual attach (for more control)
auto shader = shader::Shader::Make();
shader->AttachVertexShader("vertex.glsl");    // or vertex_source string
shader->AttachFragmentShader("fragment.glsl"); // or fragment_source string
shader->Bake();  // Link program and bind resources
```

**4-Tier Uniform System:**

**Tier 1: Global Resources (UBOs/SSBOs)**
- Bound at shader link time
- Shared across multiple shaders
- Updated once per frame or on-demand

```cpp
// Define your data structure (must match GLSL layout)
struct MyCustomData {
    glm::vec4 custom_color;
    float custom_value;
    float padding[3];  // Ensure proper alignment
};

// Create and register a custom UBO using the UBO class
auto my_ubo = uniform::UBO<MyCustomData>::Make(
    "MyCustomUBO",                   // unique name
    uniform::UpdateMode::PER_FRAME,  // update frequency
    3                                // binding point
);

// Set a provider function that returns the data
my_ubo->setProvider([]() {
    MyCustomData data;
    data.custom_color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    data.custom_value = 42.0f;
    return data;
});

// Or use partial provider for updating only specific regions
my_ubo->setPartialProvider([](MyCustomData& data) {
    data.custom_color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    data.custom_value = 42.0f;
    // Return the dirty region that was modified
    return uniform::DirtyRegion{0, sizeof(MyCustomData)};
});

// Bind UBO to shader at link time
shader->addResourceBlockToBind("MyCustomUBO");
shader->Bake();  // Links and binds the UBO

// The UBO will automatically update based on UpdateMode
// PER_FRAME: Updated every frame by the manager
// ON_DEMAND: Call uniform::manager().applyShaderResource("MyCustomUBO") manually
```

**Tier 2: Static Uniforms**
- Applied when shader becomes active
- For values that don't change during shader use

```cpp
// For constant values
shader->configureStaticUniform<int>("u_enableLighting", []() { return 1; });
shader->configureStaticUniform<float>("u_gamma", []() { return 2.2f; });
```

**Tier 3: Dynamic Uniforms**
- Applied every draw call via provider functions
- For frequently changing values

```cpp
// Model matrix from transform stack
shader->configureDynamicUniform<glm::mat4>("u_model", transform::current);

// Material properties
shader->configureDynamicUniform<glm::vec3>("u_material.ambient",
    material::stack()->getProvider<glm::vec3>("ambient"));

// Texture samplers
shader->configureDynamicUniform<uniform::detail::Sampler2D>("u_texture",
    texture::getSamplerProvider("u_texture"));
```

**Tier 4: Immediate Uniforms**
- Set directly for one-off values
- Queued if shader inactive, applied immediately if active

```cpp
shader->setUniform<float>("u_time", elapsed_time);
shader->setUniform<glm::vec3>("u_lightPos", light_position);
```


**Key Methods:**
```cpp
// Shader lifecycle
void AttachVertexShader(const std::string& source);
void AttachFragmentShader(const std::string& source);
void Bake();  // Link program and refresh uniform locations
void UseProgram();  // Activate shader

// Uniform configuration
template<typename T>
void configureStaticUniform(const std::string& name, std::function<T()> provider);

template<typename T>
void configureDynamicUniform(const std::string& name, std::function<T()> provider);

template<typename T>
void setUniform(const std::string& name, const T& value);

// Resource binding
void addResourceBlockToBind(const std::string& block_name);
```

**Important Considerations:**

- **Baking Required:** Call `Bake()` after attaching shaders or modifying resource bindings
- **Uniform Locations:** Automatically refreshed on `Bake()` - don't cache locations manually
- **Type Safety:** Shader validates uniform types against GLSL declarations
- **Provider Functions:** Dynamic uniforms use provider functions called each frame for fresh values


#### ShaderStack

Singleton managing active shader state with lazy activation.

**Access:**
```cpp
shader::stack()  // Returns pointer to singleton instance
```

**Key Methods:**
```cpp
void push(ShaderPtr shader);  // Add shader to stack (doesn't activate)
ShaderPtr top();              // Activate and return current shader
ShaderPtr peek();             // Return current shader without activating
void pop();                   // Remove top shader
```

**Important Considerations:**

- **Lazy Activation:** `push()` does NOT activate the shader - only `top()` does
- **Side Effects:** `top()` has side effects - it activates the shader and applies dynamic uniforms
- **Call Once Per Draw:** Call `top()` once per draw, not multiple times
- **Use peek() for Inspection:** Use `peek()` when you need to inspect the shader without triggering activation
- **Protected Base:** Cannot pop the base shader - ensures a valid shader is always available

**Example:**
```cpp
// Push custom shader for a subtree
shader::stack()->push(custom_shader);

// Activate and get shader (applies uniforms)
auto active = shader::stack()->top();

// Draw geometry...

// Restore previous shader
shader::stack()->pop();
```


#### Geometry

Low-level wrapper for OpenGL vertex data (VAO/VBO/EBO).

**Creation:**
```cpp
// Geometry::Make(vertex_data, index_data, num_vertices, num_indices, 
//                position_size, {additional_attribute_sizes})
auto geometry = geometry::Geometry::Make(
    vertex_data,           // float* - interleaved vertex data
    index_data,            // unsigned int* - element indices
    num_vertices,          // int - number of vertices
    num_indices,           // int - number of indices
    position_size,         // int - floats per position (typically 3 for x,y,z)
    {attr1_size, attr2_size}  // vector<int> - floats per additional attribute
);
```

**Example:**
```cpp
// Triangle with positions (3 floats) and colors (3 floats)
std::vector<float> vertices = {
    // positions        // colors
    0.0f,  0.5f, 0.0f,  1.0f, 0.0f, 0.0f,  // top (red)
   -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,  // bottom-left (green)
    0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f   // bottom-right (blue)
};
std::vector<unsigned int> indices = {0, 1, 2};

auto triangle = geometry::Geometry::Make(
    vertices.data(), indices.data(),
    3, 3,  // 3 vertices, 3 indices
    3,     // position: 3 floats (x, y, z)
    {3}    // color: 3 floats (r, g, b)
);

// Quad with positions (3 floats), UVs (2 floats), and normals (3 floats)
auto quad = geometry::Geometry::Make(
    quad_vertices.data(), quad_indices.data(),
    4, 6,  // 4 vertices, 6 indices
    3,     // position: 3 floats
    {2, 3} // UV: 2 floats, normal: 3 floats
);
```

**Key Methods:**
```cpp
void Draw();  // Bind VAO and call glDrawElements (used by GeometryComponent)
```

**Automatic Resource Management:**
- VAO, VBO, and EBO are automatically freed in destructor
- Vertex attribute pointers configured automatically based on sizes
- Attribute 0 is always position, subsequent attributes follow in order


#### Transform

Represents a 4x4 transformation matrix with chainable operations and observer pattern support.

**Creation:**
```cpp
auto transform = transform::Transform::Make();
```

**Chainable Operations:**
```cpp
// Modify existing transform (all methods return TransformPtr for chaining)
transform->translate(1.0f, 0.0f, 0.0f)
         ->rotate(45.0f, 0.0f, 0.0f, 1.0f)  // angle in DEGREES
         ->scale(2.0f, 2.0f, 2.0f);

// Reset and set operations (resets matrix first)
transform->setTranslate(1.0f, 0.0f, 0.0f);
transform->setRotate(45.0f, 0.0f, 0.0f, 1.0f);  // angle in DEGREES
transform->setScale(2.0f, 2.0f, 2.0f);
```

**Key Methods:**
```cpp
TransformPtr translate(float x, float y, float z);
TransformPtr rotate(float angle_degrees, float axis_x, float axis_y, float axis_z);
TransformPtr scale(float x, float y, float z);
TransformPtr multiply(const glm::mat4& matrix);

TransformPtr setTranslate(float x, float y, float z);
TransformPtr setRotate(float angle_degrees, float axis_x, float axis_y, float axis_z);
TransformPtr setScale(float x, float y, float z);

const glm::mat4& getMatrix() const;
TransformPtr setMatrix(const glm::mat4& matrix);
```

**Important Notes:**
- Rotation angles are in **degrees** (converted to radians internally)
- All modification methods return `TransformPtr` for method chaining
- Rotation axis is automatically normalized if needed

**Observer Pattern:**
- Implements `ISubject` interface
- Notifies observers when matrix changes
- Used by `ObservedTransformComponent` to cache world transforms


#### TransformStack

Singleton managing hierarchical transformation accumulation.

**Access:**
```cpp
transform::stack()  // Returns pointer to singleton instance
```

**Key Methods:**
```cpp
void push(const glm::mat4& matrix);  // Multiply with current top and push
glm::mat4 current();                 // Get current accumulated matrix
void pop();                          // Remove top matrix
```

**Important Considerations:**

- **Accumulation Behavior:** Stores accumulated matrices, not individual transforms
- **Multiplication:** `push()` multiplies the incoming matrix with the current top: `new_state = current_top * incoming_matrix`
- **No Inversion:** `pop()` simply removes the top level - no matrix inversion needed
- **Protected Base:** Cannot pop below the identity matrix at index 0

**Example:**
```cpp
// Push parent transform
transform::stack()->push(parent_matrix);

// Push child transform (accumulates with parent)
transform::stack()->push(child_matrix);

// Get final accumulated transform
glm::mat4 final = transform::current();  // Returns: parent * child

// Pop back to parent
transform::stack()->pop();
```

**Usage in Shaders:**
```cpp
// Configure shader to use current transform as model matrix
shader->configureDynamicUniform<glm::mat4>("u_model", transform::current);
```


#### Material

Data container for material properties with type-safe storage.

**Creation:**
```cpp
// Create from base color
auto material = gl_base::Material::Make(glm::vec3(0.8f, 0.2f, 0.2f));

// Create empty and set properties
auto material = gl_base::Material::Make()
    ->setAmbient(glm::vec3(0.2f))
    ->setDiffuse(glm::vec3(0.8f, 0.2f, 0.2f))
    ->setSpecular(glm::vec3(1.0f))
    ->setShininess(32.0f);
```

**Key Methods:**
```cpp
// PBR convenience methods
Material* setAmbient(const glm::vec3& ambient);
Material* setDiffuse(const glm::vec3& diffuse);
Material* setSpecular(const glm::vec3& specular);
Material* setShininess(float shininess);

// Generic property setting
template<typename T>
Material* set(const std::string& name, const T& value);

// Uniform name remapping
Material* setUniformNameOverride(const std::string& property, const std::string& uniform_name);
```

**Supported Types:**
- `float`, `int`
- `glm::vec2`, `glm::vec3`, `glm::vec4`
- `glm::mat3`, `glm::mat4`

**Example:**
```cpp
auto material = gl_base::Material::Make(glm::vec3(0.8f, 0.2f, 0.2f))
    ->setSpecular(glm::vec3(1.0f))
    ->setShininess(32.0f)
    ->set<float>("roughness", 0.5f)
    ->set<float>("metallic", 0.0f);
```


#### MaterialStack

Singleton managing hierarchical material property merging.

**Access:**
```cpp
material::stack()  // Returns pointer to singleton instance
```

**Key Methods:**
```cpp
void push(std::shared_ptr<Material> material);  // Merge and push
void pop();                                     // Remove top level

// Get property value
template<typename T>
T getValue(const std::string& name);

// Get provider function for shader uniforms
template<typename T>
std::function<T()> getProvider(const std::string& name);

// Configure shader with all base properties
void configureShaderDefaults(ShaderPtr shader);
```

**Important Considerations:**

- **Property Merging:** Each level stores a complete merged property map
- **Override Behavior:** Child properties completely override parent properties with matching names (not additive)
- **Provider Pattern:** Use `getProvider<T>()` for dynamic uniform binding
- **Protected Base:** Cannot pop below the base PBR state at index 0

**Example:**
```cpp
// Base material for all objects
auto base = gl_base::Material::Make(glm::vec3(0.8f));
material::stack()->push(base);

// Override specific properties for a subtree
auto shiny = gl_base::Material::Make()->setSpecular(glm::vec3(1.0f));
material::stack()->push(shiny);
// Now: ambient and diffuse inherited, specular overridden

// Render shiny objects...

material::stack()->pop();  // Restore base material
```


#### Texture

Manages OpenGL 2D textures with automatic resource management and caching.

**Creation:**
```cpp
// Load from file (uses STB Image)
auto texture = texture::Texture::Make("path/to/texture.png");
```

**Key Methods:**
```cpp
void Bind(unsigned int unit);    // Bind to texture unit
void Unbind(unsigned int unit);  // Unbind from texture unit
unsigned int GetID() const;      // Get OpenGL texture ID
```

**Automatic Caching:**
- Static cache prevents duplicate GPU uploads
- Multiple `Make()` calls with the same filename return the same texture instance
- Textures are freed when the last reference is destroyed

**Example:**
```cpp
// Load texture (cached automatically)
auto texture = texture::Texture::Make("assets/wood.png");

// Use with TextureComponent
node.with<component::TextureComponent>(texture, "u_texture", 0);
```

**Supported Formats:**
- PNG, JPG, BMP, TGA, and other formats supported by STB Image
- Automatically generates mipmaps
- Configurable wrapping and filtering (defaults: repeat wrapping, linear filtering)


#### TextureStack

Singleton managing texture bindings across multiple texture units with intelligent GPU state tracking.

**Access:**
```cpp
texture::stack()  // Returns pointer to singleton instance
```

**Key Methods:**
```cpp
void push(TexturePtr texture, unsigned int unit);  // Bind and push
void pop();                                        // Restore previous state

// Sampler registration (used by TextureComponent)
void registerSampler(const std::string& sampler_name, unsigned int unit);
void unregisterSampler(const std::string& sampler_name);

// Get sampler provider for shader uniforms
std::function<uniform::detail::Sampler2D()> getSamplerProvider(const std::string& sampler_name);
```

**Important Considerations:**

- **Complete State Storage:** Each level stores a map of `{unit -> texture}` for all active units
- **GPU State Tracking:** Tracks actual GPU state to prevent redundant `glBindTexture()` calls
- **Intelligent Restoration:** `pop()` only rebinds changed texture units
- **Unit vs ID Confusion:** Texture units (0-31) are different from texture IDs (OpenGL handles)
- **Protected Base:** Cannot pop below the empty base state at index 0

**Example:**
```cpp
// Bind multiple textures
texture::stack()->push(diffuse_tex, 0);
texture::stack()->push(normal_tex, 1);
texture::stack()->push(specular_tex, 2);

// Render objects with all three textures...

// Restore previous state
texture::stack()->pop();  // Removes specular
texture::stack()->pop();  // Removes normal
texture::stack()->pop();  // Removes diffuse
```


### Lighting System

#### Light Types

**Light (Base Class)**

Abstract base class for all light types with common properties.

```cpp
// Common properties
glm::vec4 ambient;   // Ambient color contribution
glm::vec4 diffuse;   // Diffuse color contribution
glm::vec4 specular;  // Specular color contribution
```

**DirectionalLight**

Represents infinite-distance lights (like the sun).

```cpp
auto sun = light::DirectionalLight::Make({
    .base_direction = glm::vec3(0.0f, -1.0f, 0.0f),
    .ambient = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f),
    .diffuse = glm::vec4(1.0f, 0.9f, 0.7f, 1.0f),
    .specular = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)
});
```

**PointLight**

Omnidirectional light with position and attenuation.

```cpp
auto lamp = light::PointLight::Make({
    .position = glm::vec4(0.0f, 2.0f, 0.0f, 1.0f),
    .ambient = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f),
    .diffuse = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
    .specular = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
    .constant = 1.0f,
    .linear = 0.09f,
    .quadratic = 0.032f
});
```

Attenuation formula: `1.0 / (constant + linear * d + quadratic * d²)`

**SpotLight**

Cone-shaped light (inherits from PointLight).

```cpp
auto flashlight = light::SpotLight::Make({
    .position = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
    .base_direction = glm::vec3(0.0f, 0.0f, -1.0f),
    .cutOffAngle = glm::cos(glm::radians(12.5f)),
    .diffuse = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
    .constant = 1.0f,
    .linear = 0.09f,
    .quadratic = 0.032f
});
```


#### LightManager

Templated singleton that collects all active lights and uploads them to a GPU UBO.

**Access:**
```cpp
light::manager()  // Returns reference to singleton instance
```

**Key Methods:**
```cpp
void apply();  // Collect lights and upload to GPU
void bindToShader(ShaderPtr shader);  // Register UBO with shader
```

**Automatic Registration:**
- `LightComponent` automatically registers/unregisters lights
- No manual registration needed in typical usage

**Customizing Maximum Lights:**

Default maximum is 16 lights. To change:

```cpp
// In your main.cpp BEFORE including EnGene
#define MAX_SCENE_LIGHTS 32
#include <EnGene.h>
```

**CRITICAL:** Must match GLSL shader definition exactly!

```glsl
// In your shader
#define MAX_SCENE_LIGHTS 32  // Must match C++ value

layout (std140) uniform SceneLights {
    LightData lights[MAX_SCENE_LIGHTS];
    int active_light_count;
};
```

**Example:**
```cpp
// Bind light manager to shader during initialization
auto shader = shader::Shader::Make("lit_vertex.glsl", "lit_fragment.glsl");
light::manager().bindToShader(shader);

// Lights are automatically collected and uploaded each frame
```


### Camera System

#### Camera (Base Class)

Abstract base class for all camera types.

**Key Methods:**
```cpp
virtual glm::mat4 getViewMatrix() = 0;
virtual glm::mat4 getProjectionMatrix() = 0;

void setAspectRatio(float aspect);
float getAspectRatio() const;

void bindToShader(ShaderPtr shader);  // Register camera UBO with shader
```

**Automatic UBO Creation:**
- Each camera creates a UBO for view and projection matrices
- UBO is automatically updated when matrices change

#### OrthographicCamera

Orthographic projection camera (no perspective distortion).

**Creation:**
```cpp
auto camera = component::OrthographicCamera::Make(
    left, right, bottom, top, near, far
);
```

**Example:**
```cpp
// 2D orthographic camera
auto camera = component::OrthographicCamera::Make(
    -10.0f, 10.0f,   // left, right
    -10.0f, 10.0f,   // bottom, top
    0.1f, 100.0f     // near, far
);

scene::graph()->setActiveCamera(camera);
```

#### PerspectiveCamera

Perspective projection camera (realistic 3D perspective).

**Creation:**
```cpp
auto camera = component::PerspectiveCamera::Make(
    fov_degrees, near, far
);
```

**Example:**
```cpp
// 3D perspective camera
auto camera = component::PerspectiveCamera::Make(
    45.0f,   // FOV in degrees
    0.1f,    // Near plane
    100.0f   // Far plane
);

// Set aspect ratio (width / height)
camera->setAspectRatio(800.0f / 600.0f);

scene::graph()->setActiveCamera(camera);
```

**Important Notes:**
- FOV is specified in **degrees** (not radians)
- Aspect ratio is **width / height** (e.g., 16/9 = 1.777...)
- Inherits from `ObservedTransformComponent` for efficient world position tracking
- Use `setTarget()` to make camera look at another ObservedTransformComponent


### Application Framework

#### EnGene

Main application class that orchestrates the entire engine.

**Creation:**
```cpp
engene::EnGene(
    std::function<void(EnGene&)> on_initialize,
    std::function<void(double)> on_fixed_update,
    std::function<void(double)> on_render,
    EnGeneConfig config = {},
    input::InputHandler* handler = nullptr
);
```

**Lifecycle Callbacks:**

- **on_initialize(EnGene&)** - Called once before the main loop
  - Receives engine reference for configuration
  - Use for scene construction and setup
  
- **on_fixed_update(double dt)** - Called at fixed rate (default 60 Hz)
  - Receives fixed timestep (e.g., 1/60 = 0.0166... seconds)
  - Use for physics and simulation logic
  
- **on_render(double alpha)** - Called every frame
  - Receives interpolation alpha (0.0-1.0) for smooth rendering
  - Use for drawing and visual updates

**Key Methods:**
```cpp
void run();  // Start the main loop (blocks until window closes)
ShaderPtr getBaseShader();  // Get base shader for configuration
```

**Example:**
```cpp
auto on_initialize = [](engene::EnGene& app) {
    // Build scene graph
    scene::graph()->addNode("Root")
        .with<component::GeometryComponent>(geometry);
    
    // Configure base shader if needed
    auto shader = app.getBaseShader();
    shader->setUniform<float>("u_time", 0.0f);
};

auto on_fixed_update = [](double dt) {
    // Update physics at fixed 60 FPS
    physics_engine->update(dt);
};

auto on_render = [](double alpha) {
    // Render scene
    scene::graph()->draw();
};

engene::EnGeneConfig config;
config.title = "My Application";
config.width = 1280;
config.height = 720;

engene::EnGene app(on_initialize, on_fixed_update, on_render, config);
app.run();
```


#### EnGeneConfig

Configuration struct with sensible defaults.

**Fields:**
```cpp
struct EnGeneConfig {
    int width = 800;                    // Window width
    int height = 600;                   // Window height
    std::string title = "EnGene Window"; // Window title
    int updatesPerSecond = 60;          // Fixed update frequency
    double maxFrameTime = 0.25;         // Spiral of death prevention
    float clearColor[4] = {0.1f, 0.1f, 0.1f, 1.0f};  // Background color
    std::string base_vertex_shader_source;    // Vertex shader (file or raw GLSL)
    std::string base_fragment_shader_source;  // Fragment shader (file or raw GLSL)
};
```

**Example:**
```cpp
engene::EnGeneConfig config;
config.width = 1920;
config.height = 1080;
config.title = "My Game";
config.updatesPerSecond = 120;  // 120 FPS fixed updates
config.clearColor[0] = 0.2f;    // Darker background
config.clearColor[1] = 0.2f;
config.clearColor[2] = 0.2f;

// Use custom shaders
config.base_vertex_shader_source = "shaders/my_vertex.glsl";
config.base_fragment_shader_source = "shaders/my_fragment.glsl";
```

**Default Shaders:**
- If shader sources are not specified, EnGene uses built-in default shaders
- Default shaders include camera UBO binding and basic vertex color pass-through


#### InputHandler

Type-safe GLFW input callback management.

**Creation:**
```cpp
auto handler = std::make_unique<input::InputHandler>();
```

**Register Callbacks:**
```cpp
// Keyboard callback
handler->registerCallback<input::InputType::KEY>(
    [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }
    }
);

// Mouse button callback
handler->registerCallback<input::InputType::MOUSE_BUTTON>(
    [](GLFWwindow* window, int button, int action, int mods) {
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            // Handle left click
        }
    }
);

// Mouse movement callback
handler->registerCallback<input::InputType::MOUSE_MOVE>(
    [](GLFWwindow* window, double xpos, double ypos) {
        // Handle mouse movement
    }
);

// Scroll callback
handler->registerCallback<input::InputType::SCROLL>(
    [](GLFWwindow* window, double xoffset, double yoffset) {
        // Handle scroll
    }
);
```

**Example:**
```cpp
auto handler = std::make_unique<input::InputHandler>();

handler->registerCallback<input::InputType::KEY>(
    [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        if (action == GLFW_PRESS) {
            switch (key) {
                case GLFW_KEY_W: /* Move forward */ break;
                case GLFW_KEY_S: /* Move backward */ break;
                case GLFW_KEY_A: /* Move left */ break;
                case GLFW_KEY_D: /* Move right */ break;
            }
        }
    }
);

engene::EnGene app(on_init, on_update, on_render, config, handler.release());
```


---

## Advanced Topics

### Understanding Stack Systems

EnGene uses several stack-based systems for managing hierarchical state. Each stack has unique behaviors.

#### ShaderStack Cautions

**Important:** `top()` has side effects!

- `push()` adds shader to stack but does NOT activate it
- `top()` activates the shader AND applies dynamic uniforms
- Call `top()` once per draw, not multiple times
- Use `peek()` for inspection without activation

**Lazy Activation:**
The shader isn't actually bound until `top()` is called. This prevents redundant `glUseProgram()` calls.

```cpp
shader::stack()->push(custom_shader);  // NOT active yet
auto active = shader::stack()->top();  // NOW active + uniforms applied
```

#### TransformStack Cautions

**Important:** Stores accumulated matrices, not individual transforms!

- Each level contains the product of all parent transforms
- `push()` multiplies: `new_state = current_top * incoming_matrix`
- You cannot "peek" at individual transform components
- `pop()` simply removes the top (no matrix inversion)

```cpp
transform::stack()->push(parent);     // [identity * parent]
transform::stack()->push(child);      // [identity * parent, parent * child]
glm::mat4 final = transform::current(); // Returns: parent * child
```


#### TextureStack Cautions

**Important:** Texture units vs texture IDs!

- Texture units (0-31) are binding points, not texture IDs
- Each level stores a complete map of `{unit -> texture}`
- GPU state is tracked to prevent redundant binds
- `pop()` intelligently restores only changed units

**Sampler Registration:**
TextureComponent registers sampler names during `apply()` and unregisters during `unapply()`. Use `texture::getSamplerProvider()` for dynamic uniform binding.

```cpp
// Correct: Use texture unit
texture::stack()->push(texture, 0);  // Unit 0

// Incorrect: Don't use texture ID
// texture::stack()->push(texture, texture->GetID());  // WRONG!
```

#### MaterialStack Cautions

**Important:** Properties override, not add!

- Child materials completely override parent properties with matching names
- Not additive like transform multiplication
- Each level stores a complete merged property map
- Use unique names to avoid unintended overrides

```cpp
auto base = Material::Make()->setDiffuse(glm::vec3(1.0, 0.0, 0.0));
material::stack()->push(base);

auto override = Material::Make()->setDiffuse(glm::vec3(0.0, 1.0, 0.0));
material::stack()->push(override);
// Diffuse is now green, not red+green
```


### Component Priority System

**Important:** Components are automatically sorted by priority!

Components execute in priority order during `apply()` and reverse order during `unapply()`. The order you add components to a node doesn't matter - they're automatically sorted by their priority values.

**Standard Priorities:**
- **100** - Transform (applied first)
- **200** - Shader (shader selection)
- **300** - Material/Texture (appearance setup)
- **400** - Camera (view/projection)
- **500** - Geometry (drawing, applied last)
- **600+** - Custom scripts

**Declaration Order Doesn't Matter:**
```cpp
// These are equivalent - components are sorted by priority automatically
node.with<component::GeometryComponent>(geometry)      // Priority 500
    .with<component::TransformComponent>(transform);   // Priority 100
// Result: Transform (100) applies first, then Geometry (500)

// Same result:
node.with<component::TransformComponent>(transform)    // Priority 100
    .with<component::GeometryComponent>(geometry);     // Priority 500
// Result: Transform (100) applies first, then Geometry (500)
```

**Custom Priorities:**
If you need specific ordering, you can override the default priority:
```cpp
node.with<component::TransformComponent>(transform, 150);  // Custom priority
```

### Uniform System Best Practices

**Tier 1 (UBOs)** - Use for data shared across shaders
- Camera matrices (view, projection)
- Scene lights array
- Global parameters

**Tier 2 (Static)** - Use for per-shader constants
- Texture unit assignments
- Shader configuration flags

**Tier 3 (Dynamic)** - Use for per-draw values
- Model matrix (from transform stack)
- Material properties (from material stack)
- Texture samplers (from texture stack)

**Tier 4 (Immediate)** - Use for one-off values
- Time
- Custom parameters
- Debug values


### Uniform Location Invalidation

**Important:** Uniform locations become invalid after shader re-linking!

- `Bake()` links the shader program and refreshes all uniform locations
- Don't cache uniform locations manually
- The shader class handles this automatically

```cpp
shader->AttachVertexShader(new_vertex_source);
shader->Bake();  // Re-links and refreshes uniform locations
```

### Light Count Configuration

**Important:** C++ and GLSL values must match exactly!

```cpp
// C++ side (before including EnGene.h)
#define MAX_SCENE_LIGHTS 32
#include <EnGene.h>
```

```glsl
// GLSL side (must match!)
#define MAX_SCENE_LIGHTS 32

layout (std140) uniform SceneLights {
    LightData lights[MAX_SCENE_LIGHTS];
    int active_light_count;
};
```

Mismatch causes rendering artifacts or crashes.

---

## Project Structure

```
EnGene/
├── core_gene/
│   ├── src/
│   │   ├── EnGene.h                    # Main engine class
│   │   ├── core/                       # Scene graph core
│   │   │   ├── node.h
│   │   │   ├── scene.h
│   │   │   ├── scene_node_builder.h
│   │   │   └── EnGene_config.h
│   │   ├── components/                 # ECS components
│   │   │   ├── component.h
│   │   │   ├── component_collection.h
│   │   │   ├── transform_component.h
│   │   │   ├── geometry_component.h
│   │   │   ├── shader_component.h
│   │   │   ├── texture_component.h
│   │   │   ├── material_component.h
│   │   │   └── light_component.h
│   │   ├── gl_base/                    # OpenGL abstractions
│   │   │   ├── shader.h
│   │   │   ├── geometry.h
│   │   │   ├── transform.h
│   │   │   ├── material.h
│   │   │   ├── texture.h
│   │   │   └── uniforms/               # Uniform system
│   │   ├── 3d/
│   │   │   ├── camera/                 # Camera implementations
│   │   │   └── lights/                 # Light types & manager
│   │   └── other_genes/                # Prebuilt shapes & utilities
│   │       ├── shapes/                 # 2D shapes
│   │       ├── textured_shapes/        # Textured 2D shapes
│   │       ├── 3d_shapes/              # 3D shapes
│   │       └── input_handlers/         # Input handler presets
│   └── shaders/                        # GLSL shader files
├── libs/                               # External libraries
└── CMakeLists.txt                      # CMake configuration
```


---

## Dependencies

- **C++ Standard:** C++17 minimum (C++20 recommended)
- **OpenGL:** 4.3 Core Profile
- **GLFW** - Window and input management
- **GLAD** - OpenGL function loader
- **GLM** - Mathematics library for graphics
- **STB Image** - Image loading (stb_image.h)

---

## Examples

### Solar System with Lighting

```cpp
#include <EnGene.h>
#include "other_genes/3d_shapes/sphere.h"

int main() {
    auto on_initialize = [](engene::EnGene& app) {
        // Build solar system with inline component creation
        scene::graph()->addNode("Sun")
            .with<component::TransformComponent>(
                transform::Transform::Make()->scale(2.0f, 2.0f, 2.0f)
            )
            .with<component::LightComponent>(
                light::PointLight::Make({
                    .position = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
                    .diffuse = glm::vec4(1.0f, 0.9f, 0.7f, 1.0f),
                    .constant = 1.0f,
                    .linear = 0.014f,
                    .quadratic = 0.0007f
                }),
                transform::Transform::Make()
            )
            .with<component::MaterialComponent>(
                material::Material::Make(glm::vec3(1.0f, 0.9f, 0.2f))
            )
            .with<component::GeometryComponent>(
                Sphere::Make(1.0f, 32, 32)
            )
            .addNode("Earth")
                .with<component::TransformComponent>(
                    transform::Transform::Make()->translate(8.0f, 0.0f, 0.0f)
                )
                .with<component::MaterialComponent>(
                    material::Material::Make(glm::vec3(0.2f, 0.4f, 0.8f))
                )
                .with<component::GeometryComponent>(
                    Sphere::Make(0.8f, 24, 24)
                )
                .addNode("Moon")
                    .with<component::TransformComponent>(
                        transform::Transform::Make()->translate(2.0f, 0.0f, 0.0f)
                    )
                    .with<component::MaterialComponent>(
                        material::Material::Make(glm::vec3(0.7f, 0.7f, 0.7f))
                    )
                    .with<component::GeometryComponent>(
                        Sphere::Make(0.3f, 16, 16)
                    );
    };
    
    auto on_fixed_update = [](double dt) {
        // Update orbital rotations
        static float earth_angle = 0.0f;
        earth_angle += dt * 20.0f;  // degrees per second
        
        auto earth = scene::graph()->findNode("Earth");
        if (earth) {
            auto transform = earth->payload().get<component::TransformComponent>();
            if (transform) {
                transform->getTransform()->setTranslate(
                    8.0f * cos(glm::radians(earth_angle)),
                    0.0f,
                    8.0f * sin(glm::radians(earth_angle))
                );
            }
        }
    };
    
    auto on_render = [](double alpha) {
        scene::graph()->draw();
    };
    
    engene::EnGeneConfig config;
    config.title = "Solar System";
    
    engene::EnGene app(on_initialize, on_fixed_update, on_render, config);
    app.run();
    
    return 0;
}
```


### Textured Quad

```cpp
#include <EnGene.h>

int main() {
    auto on_initialize = [](engene::EnGene& app) {
        // Create quad geometry with UVs
        std::vector<float> vertices = {
            // positions        // UVs
           -0.5f, -0.5f, 0.0f,  0.0f, 0.0f,
            0.5f, -0.5f, 0.0f,  1.0f, 0.0f,
            0.5f,  0.5f, 0.0f,  1.0f, 1.0f,
           -0.5f,  0.5f, 0.0f,  0.0f, 1.0f
        };
        std::vector<unsigned int> indices = {0, 1, 2, 2, 3, 0};
        
        // Build scene with inline component creation
        scene::graph()->addNode("TexturedQuad")
            .with<component::ShaderComponent>(
                shader::Shader::Make("shaders/textured_vertex.glsl",
                                    "shaders/textured_fragment.glsl")
                    ->configureDynamicUniform<glm::mat4>("u_model", transform::current)
                    ->configureDynamicUniform<uniform::detail::Sampler2D>("u_texture",
                        texture::getSamplerProvider("u_texture"))
            )
            .with<component::TextureComponent>(
                texture::Texture::Make("assets/wood.png"),
                "u_texture",
                0  // texture unit
            )
            .with<component::GeometryComponent>(
                geometry::Geometry::Make(
                    vertices.data(), indices.data(),
                    4, 6,  // 4 vertices, 6 indices
                    3,     // position: 3 floats
                    {2}    // UV: 2 floats
                )
            );
    };
    
    auto on_fixed_update = [](double dt) {};
    
    auto on_render = [](double alpha) {
        scene::graph()->draw();
    };
    
    engene::EnGeneConfig config;
    config.title = "Textured Quad";
    
    engene::EnGene app(on_initialize, on_fixed_update, on_render, config);
    app.run();
    
    return 0;
}
```

---

## Contributing

Contributions are welcome! Please see [AGENTS.md](AGENTS.md) for:
- Development guidelines and architecture details
- Build system and testing protocols
- Coding standards and naming conventions
- Design patterns and best practices
- Contribution workflow

---

## License

[Add your license information here]

---

## Links

- **Repository:** https://github.com/The-EnGene-Project/CoreGene
- **Documentation:** [AGENTS.md](AGENTS.md)
- **Issues:** https://github.com/The-EnGene-Project/CoreGene/issues

