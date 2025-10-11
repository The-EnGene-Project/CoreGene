#include <EnGene.h>
#include <core/scene.h>
#include <core/scene_node_builder.h>
#include <shapes/circle.h>
#include <other_genes/basic_input_handler.h>
#include <gl_base/error.h>
#include <components/all.h>

#define BACKGROUND_COLOR 0.05f, 0.05f, 0.1f

int main() {
    // The on_init lambda now only focuses on building the scene graph.
    // Shader initialization is moved outside.
    transform::TransformPtr sun_rotation;
    transform::TransformPtr earth_orbit;
    auto on_init = [&](engene::EnGene& app) {
        // configures the uniforms from the base shader.
        app.getBaseShader()->configureUniform<glm::mat4>("M", transform::current);

        // 1. Build the Sun
        // We start a chain from the graph's root. The .with<T>() methods
        // add components to the node being built ("Sun").
        sun_rotation = transform::Transform::Make()->rotate(30, 0, 0, 1);
        scene::graph()->addNode("Sun")
            .with<component::TransformComponent>(
                sun_rotation
            )
            .with<component::GeometryComponent>(
                Circle::Make(
                    0.0f, 0.0f,         // center pos
                    0.3f,               // radius
                    (float[]) {         // colors
                        0.8f, 0.5f, 0.0f, // center
                        BACKGROUND_COLOR, // edge
                    },
                    32,                 // segments
                    true                // has gradient
                )
            );

        // 2. Build the Earth System
        // We start a *new* chain from the root for the orbital pivot.
        earth_orbit = transform::Transform::Make();
        // Chaining .addNode() creates a child-parent relationship.
        scene::graph()->addNode("Earth") 
            .with<component::GeometryComponent>(
                Circle::Make(
                    0.0f, 0.0f,         // center pos
                    0.1f,               // radius
                    (float[]) {         // colors
                        0.0f, 0.1f, 0.5f // center
                    },
                    32,                 // segments
                    false               // no gradient
                )
            )
            .with<component::TransformComponent>(
                transform::Transform::Make()->translate(0.7f, 0.0f, 0.0f)
            )
            .with<component::TransformComponent>(
                earth_orbit, 99
            );
    };

    // Define the user's per-frame update and drawing logic
    auto on_update = [&]() {
        // This code runs every frame.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // ... update object positions, handle animations ...

        if (sun_rotation) {
            sun_rotation->rotate(-0.5f, 0, 0, 1); // Rotate the sun slowly
        }
        if (earth_orbit) {
            earth_orbit->rotate(1.2f, 0, 0, 1); // Orbit the earth faster
        }
        
        scene::graph()->draw();
        
        Error::Check("update");
    };

    try {
        // 1. Set up the EnGeneConfig struct.
        engene::EnGeneConfig config;
        /* Exclusive to C++20 and above:
        config = {
            .base_vertex_shader_path   = "../shaders/vertex.glsl",
            .base_fragment_shader_path = "../shaders/fragment.glsl",
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
        config.clearColor[0] = 0.05f;
        config.clearColor[1] = 0.05f;
        config.clearColor[2] = 0.1f;
        config.clearColor[3] = 1.0f;
        config.base_vertex_shader_path = "../shaders/vertex.glsl";      // This is a required field.
        config.base_fragment_shader_path = "../shaders/fragment.glsl";  // This is a required field.

        // 2. Create your input handler instance.
        auto* handler = new input::BasicInputHandler();

        // 3. Create the EnGene instance, passing in the configurations.
        engene::EnGene app(
            config,      // Pass the config struct
            handler,     // The input handler
            on_init,     // Your init function
            on_update    // Your update function
        );
        
        // 4. Run the application.
        app.run();

    } catch (const std::runtime_error& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}