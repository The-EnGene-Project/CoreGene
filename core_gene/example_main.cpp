#include <EnGene.h>
#include <core/scene.h>
#include <core/scene_node_builder.h>
#include <other_genes/shapes/circle.h>
#include <other_genes/textured_shapes/textured_circle.h>
#include <other_genes/basic_input_handler.h>
#include <gl_base/error.h>
#include <gl_base/shader.h>
#include <components/all.h>
#include <lights/light_config.h>
#include <lights/light_data.h>
#include <lights/light_manager.h>
#include <components/light_component.h>

#include <string>

#define BACKGROUND_COLOR 0.05f, 0.05f, 0.1f

// --- NEW (Step 6): Raw Vertex Shader Source ---
// We can define shaders directly in the code as raw C++ strings.
// This is perfect for bundling essential shaders without relying on external files.
// Note the changes:
// 1. We now use the 'CameraMatrices' UBO (Tier 1) for view/projection.
// 2. We use 'u_model' (Tier 3) for the model matrix, by convention.
const std::string textured_vertex_source = R"(
    #version 410 core
    layout (location = 0) in vec3 a_pos;
    layout (location = 1) in vec2 a_texCoord; // From TexturedCircle

    out vec2 v_texCoord;

    // Tier 1: Global UBO, managed by the Camera component.
    layout (std140, binding = 0) uniform CameraMatrices {
        mat4 view;
        mat4 projection;
    };

    // Tier 3: Dynamic Uniform, managed by the TransformComponent.
    uniform mat4 u_model;

    void main() {
        gl_Position = projection * view * u_model * vec4(a_pos, 1.0);
        v_texCoord = a_texCoord;
    }
)";


int main() {
    // These transform pointers are kept so we can animate them in the update loop.
    transform::TransformPtr sun_rotation;
    transform::TransformPtr earth_orbit;
    transform::TransformPtr earth_rotation;
    
    // Light transforms for animation
    transform::TransformPtr directional_light_transform;
    transform::TransformPtr point_light_transform;
    transform::TransformPtr spot_light_transform;

    // --- on_initialize ---
    // This lambda now *only* focuses on building the scene graph.
    auto on_init = [&](engene::EnGene& app) {
        
        // --- NEW: Create lights using the new API ---
        
        // Create DirectionalLight with base_direction parameter
        auto directional_light = light::DirectionalLight::Make({
            .base_direction = glm::vec3(0.0f, -1.0f, 0.0f),
            .ambient = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f),
            .diffuse = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f),
            .specular = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)
        });
        directional_light_transform = transform::Transform::Make();
        
        // Create PointLight with individual attenuation parameters
        auto point_light = light::PointLight::Make({
            .position = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
            .constant = 1.0f,
            .linear = 0.09f,
            .quadratic = 0.032f,
            .ambient = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f),
            .diffuse = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f),  // Orange light
            .specular = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)
        });
        point_light_transform = transform::Transform::Make()->translate(0.7f, 0.0f, 0.0f);
        
        // Create SpotLight with base_direction parameter
        auto spot_light = light::SpotLight::Make({
            .position = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
            .constant = 1.0f,
            .linear = 0.09f,
            .quadratic = 0.032f,
            .base_direction = glm::vec3(0.0f, -1.0f, 0.0f),
            .cutoff_angle = glm::cos(glm::radians(12.5f)),
            .ambient = glm::vec4(0.05f, 0.05f, 0.05f, 1.0f),
            .diffuse = glm::vec4(0.0f, 0.8f, 1.0f, 1.0f),  // Cyan light
            .specular = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)
        });
        spot_light_transform = transform::Transform::Make()->translate(-0.7f, 0.5f, 0.0f);
        
        // --- OLD SHADER SETUP (REMOVED) ---
        // app.getBaseShader()->configureUniform<glm::mat4>("M", transform::current);
        
        // --- NEW (Step 5 & 6): Automatic Base Shader Configuration ---
        // This is no longer needed! The EnGene constructor now automatically:
        // 1. Finds the SceneGraph's default OrthographicCamera.
        // 2. Binds the camera's UBOs to the engine's base shader.
        // 3. Configures the base shader's "u_model" uniform (Tier 3)
        //    to use 'transform::current' automatically.
        // Your base shader is ready to use with the UBO system out-of-the-box.


        // --- NEW (Step 6): Creating a Custom Shader ---
        // We now create the textured shader, demonstrating:
        // 1. Loading the vertex shader from our raw string variable.
        // 2. Loading the fragment shader from a file (showing flexibility).
        // 3. The new 4-tier uniform configuration methods.
        shader::ShaderPtr textured_shader = shader::Shader::Make(
            textured_vertex_source,                 // Source from raw string
            "../shaders/textured_fragment.glsl"   // Source from file
        )
        // --- Tier 3 (Dynamic): Per-Draw Uniforms ---
        // This uniform changes for every object. 'transform::current' provides
        // the final model matrix from the transform stack.
        ->configureDynamicUniform<glm::mat4>("u_model", transform::current)

        // --- Tier 2 (Static): Per-Use Uniforms ---
        // This uniform is for the texture, which is constant for this shader
        // program. We use 'configureStaticUniform' so it's only applied once
        // when the shader is first activated.
        ->configureStaticUniform<int>("tex", texture::getUnitProvider("tex"));

        // --- NEW (Step 6): Linking the Shader ---
        // Because our shader needs to bind to Tier 1 UBOs (like "CameraMatrices"),
        // we must tell the camera *before* we link.
        
        // 1. Get the scene's active camera.
        auto camera = scene::graph()->getActiveCamera();

        // 2. Tell the camera to bind its resources to our new shader.
        //    (This just queues the names "CameraMatrices" and "CameraPosition").
        camera->bindToShader(textured_shader);
        
        // 3. Bind the SceneLights UBO to the shader for lighting
        uniform::manager().bindResourceToShader(textured_shader, "SceneLights");


        // 1. Build the Sun
        // This node uses the *base shader* automatically.
        // The builder pattern is unchanged and works perfectly.
        sun_rotation = transform::Transform::Make()->rotate(30, 0, 0, 1);
        scene::graph()->addNode("Sun")
            .with<component::TransformComponent>(
                sun_rotation
            )
            .with<component::GeometryComponent>(
                Circle::Make(
                    0.0f, 0.0f,          // center pos
                    0.3f,                // radius
                    (float[]) {          // colors
                        0.8f, 0.5f, 0.0f, // center
                        BACKGROUND_COLOR, // edge
                    },
                    32,                  // segments
                    true                 // has gradient
                )
            )
            .with<component::LightComponent>(
                component::LightComponent::Make(point_light, point_light_transform),
                "SunPointLight"
            );

        // 2. Build the Earth System
        // We start a *new* chain from the root for the orbital pivot.
        // This node uses our new *textured shader*.
        earth_orbit = transform::Transform::Make();
        earth_rotation = transform::Transform::Make();
        scene::graph()->addNode("Earth") 
            .with<component::GeometryComponent>(
                TexturedCircle::Make(
                    0.0f, 0.0f,         // center pos
                    0.1f,               // radius
                    32,                 // segments
                    0.5f, 0.5f,       // tex center
                    0.45f             // tex radius
                )
            )
            .with<component::ShaderComponent>(textured_shader)
            .with<component::TextureComponent>(
                texture::Texture::Make("../assets/images/earth_from_space.jpg"),
                "tex",
                0
            )
            .with<component::TransformComponent>(
                transform::Transform::Make()->translate(0.7f, 0.0f, 0.0f)
            )
            .with<component::TransformComponent>(
                earth_orbit, 99 // Lower priority for orbit
            )
            .with<component::TransformComponent>(
                earth_rotation, 101 // Higher priority for local rotation
            );
        
        // 3. Add DirectionalLight node
        // This light will provide general scene illumination
        scene::graph()->addNode("DirectionalLight")
            .with<component::LightComponent>(
                component::LightComponent::Make(directional_light, directional_light_transform),
                "MainDirectionalLight"
            );
        
        // 4. Add SpotLight node with nested hierarchy to test transform inheritance
        // This light will be positioned above and to the left
        // We'll create a parent node that rotates, and the spotlight as a child
        auto parent_transform = transform::Transform::Make();
        auto parent_node = scene::graph()->addNode("SpotLightParent")
            .with<component::TransformComponent>(parent_transform);
        
        // Add the spotlight as a child of the parent node
        parent_node.addChild("SpotLight")
            .with<component::LightComponent>(
                component::LightComponent::Make(spot_light, spot_light_transform),
                "MainSpotLight"
            );
        
        // Store the parent transform so we can animate it
        // This will demonstrate that the child light inherits the parent's transform
        spot_light_transform = parent_transform;
    };

    // --- Simulation vs. Render Loop ---
    // We split logic: 'on_fixed_update' for simulation/physics
    // and 'on_render' for drawing.

    // --- on_fixed_update ---
    // This logic runs at a fixed rate (e.g., 60fps) for stable animation.
    auto on_fixed_update = [&](double fixed_time_step) {
        if (sun_rotation) {
            sun_rotation->rotate(0.25f, 0, 0, 1); // Rotate the sun slowly
        }
        if (earth_orbit) {
            earth_orbit->rotate(0.6f, 0, 0, 1); // Orbit the earth faster
        }
        if (earth_rotation) {
            earth_rotation->rotate(3.0f, 0, 0, -1);
        }
        
        // --- NEW: Animate the spotlight parent to test transform inheritance ---
        // The spotlight is a child of this parent, so it should inherit the rotation
        if (spot_light_transform) {
            spot_light_transform->rotate(0.5f, 0, 0, 1); // Rotate the parent slowly
        }
        
        // Animate the directional light direction
        if (directional_light_transform) {
            directional_light_transform->rotate(0.3f, 1, 0, 0); // Rotate around X axis
        }
    };

    // --- on_render ---
    // This logic runs once per displayed frame.
    auto on_render = [&](double interpolation_alpha) {
        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // --- NEW: Apply light manager before rendering ---
        // This synchronizes all light data to the GPU
        light::manager().apply();
        
        // Draw the entire scene graph.
        scene::graph()->draw();
        
        GL_CHECK("render");
    };

    try {
        // If desired, set up the EnGeneConfig struct. 
        // If not provided, default values are taken 
        // for the missing fields.
        engene::EnGeneConfig config;
        /* Exclusive to C++20 and above:
        config = {
            .base_vertex_shader_source   = "../shaders/vertex.glsl",
            .base_fragment_shader_source = "../shaders/fragment.glsl",
            .title                     = "My Modern EnGene App",
            .width                     = 800,
            .height                    = 800,
            .clearColor                = { BACKGROUND_COLOR, 1.0f }
            // .maxFramerate is not listed, so it will keep its default value (60)
        };
        */
        // config.width = 800;
        // config.height = 800;
        // config.title = "My Awesome EnGene App";
        // config.maxFramerate = 60;
        // config.clearColor[0] = 0.05f;
        // config.clearColor[1] = 0.05f;
        // config.clearColor[2] = 0.1f;
        // config.clearColor[3] = 1.0f;
        // config.base_vertex_shader_source = "../shaders/vertex.glsl";
        // config.base_fragment_shader_source = "../shaders/fragment.glsl";
        
        // 2. Create your input handler instance.
        auto* handler = new input::BasicInputHandler();

        // 3. Create the EnGene instance, passing in the configurations.
        engene::EnGene app(
            on_init,           // Your init function
            on_fixed_update,   // Your simulation function
            on_render,         // Your render function
            config,            // Pass the config struct (Optional)
            handler            // The input handler (Optional)
        );
        
        // 4. Run the application.
        app.run();

    } catch (const std::runtime_error& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}