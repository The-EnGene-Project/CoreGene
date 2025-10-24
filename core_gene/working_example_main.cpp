#include <EnGene.h>
#include <core/scene.h>
#include <core/scene_node_builder.h>
#include <other_genes/textured_shapes/textured_circle.h>
#include <other_genes/textured_shapes/quad.h>
#include <other_genes/basic_input_handler.h>
#include <gl_base/error.h>
#include <gl_base/shader.h>
#include <components/all.h>

// <<< Include physics engine files
#include "physics/engine.h"
#include "physics/physicsBody.h"

#include <random> // For generating random positions

#define BACKGROUND_COLOR 0.05f, 0.05f, 0.1f

// <<< Declare the physics engine pointer so it can be accessed by on_init and on_update
EnginePtr physicsEngine;
int circle_count = 0;
TexturedCirclePtr earth;

// Function to create a circle with a physics body
void createPhysicsCircle(const glm::vec2& initialPosition, float radius, shader::ShaderPtr shader, std::string container)
{
    // 1. Create the transform that will be shared between the scene node and the physics body.
    auto circle_transform = transform::Transform::Make();
    
    // 2. Create the visual representation (the scene node).
    scene::graph()->buildAt(container)
    .addNode("Circle" + std::to_string(circle_count))
        .with<component::GeometryComponent>(earth)
        .with<component::ShaderComponent>(shader)
        .with<component::TextureComponent>(
            texture::Texture::Make("assets/images/earth_from_space.jpg"), // Using earth texture for all
            "tex",
            0
        )
        // This component holds the transform that the physics engine will update.
        .with<component::TransformComponent>(circle_transform)
        .with<component::TransformComponent>(
            transform::Transform::Make()
            ->translate(0,0,0.5)
            ->scale(radius,radius,radius),
            101
        );

    circle_count++;

    // 3. Create the physics body.
    // Pass the initial position, the shared transform, and the radius.
    auto physics_body = PhysicsBody::Make(initialPosition, circle_transform, radius);

    // 4. Add the new body to the engine.
    physicsEngine->addBody(physics_body);
}


int main() {

    // Setup for random positions
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> distr(-0.7f, 0.7f);
    std::uniform_real_distribution<> radii(0.03f, 0.3f);
    int initialNumberOfCircles = 30;
        

    shader::ShaderPtr textured_shader;

    auto on_init = [&](engene::EnGene& app) {
        // configures the uniforms from the base shader.
        // app.getBaseShader()->configureUniform<glm::mat4>("M", transform::current);

        // creates the texture shader and configures its uniforms
        textured_shader = shader::Shader::Make(
            "shaders/textured_vertex.glsl",
            "shaders/textured_fragment.glsl"
        )
        ->configureDynamicUniform<glm::mat4>("u_model", transform::current)
        ->configureDynamicUniform<int>("tex", texture::getUnitProvider("tex"));

        scene::graph()->getActiveCamera()->bindToShader(textured_shader);

        // <<< 1. Initialize the Physics Engine
        // We define the simulation area to match the typical OpenGL normalized device coordinates.
        physicsEngine = Engine::make(-1.0f, 1.0f, -1.0f, 1000.0f, glm::vec2(0.0f, -2.0f));

        // <<< 2. Create the container for the circles
        scene::graph()->addNode("container")
            .with<component::GeometryComponent>(
                Quad::Make(-1,-1,1,1)
            )
            .with<component::ShaderComponent>(textured_shader)
            .with<component::TextureComponent>(
                texture::Texture::Make("assets/images/starred-paint.jpg"),
                "tex",
                1
            );

        // <<< 3. Initialize the reused Circle Geometry
        earth = TexturedCircle::Make(
                0.0f, 0.0f,  // Center pos (local to the node)
                1.0f,        // Radius
                32,          // Segments
                0.5f, 0.5f,  // Texture scale/offset if needed
                0.45f
            );
    };

    double time_passed = 0;

    // This function handles the fixed-timestep simulation logic.
    auto on_fixed_update = [&](double fixed_timestep) {
        // Update the physics engine.
        // This will calculate new positions based on gravity and collisions.
        // Because we linked the transforms, the visual objects will move automatically.
        
        if (circle_count < initialNumberOfCircles)
        {
            time_passed += fixed_timestep;
            if (time_passed > 1) {
                glm::vec2 pos(distr(gen), 0.8f); // Random initial position
                createPhysicsCircle(pos, radii(gen), textured_shader, "container");
                time_passed = 0;
            }
        }

        if(physicsEngine)
        {
            physicsEngine->update(static_cast<float>(fixed_timestep));
        }

    };

    // This function handles all rendering.
    auto on_render = [&](double alpha) {
        // The 'alpha' parameter can be used for smooth interpolation between physics states
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Render the scene graph with the updated positions.
        scene::graph()->draw();
        
        GL_CHECK("render");
    };

    try {
        engene::EnGeneConfig config;
        config.width = 800;
        config.height = 800;
        config.title = "Physics Engine Demo";
        config.clearColor[0] = 0.05f;
        config.clearColor[1] = 0.05f;
        config.clearColor[2] = 0.1f;
        config.clearColor[3] = 1.0f;
        // config.base_vertex_shader_source = "shaders/vertex.glsl";
        // config.base_fragment_shader_source = "shaders/fragment.glsl";

        auto* handler = new input::InputHandler();
        handler->registerCallback<input::InputType::MOUSE_BUTTON>([&](MOUSE_BUTTON_HANDLER_ARGS) {
            if (action == GLFW_PRESS) {
                double xpos, ypos;
                glfwGetCursorPos(window, &xpos, &ypos);

                int fb_w, fb_h;
                glfwGetFramebufferSize(window, &fb_w, &fb_h);
                
                // Convert screen coordinates (pixels) to Normalized Device Coordinates (NDC) [-1, 1]
                float x_ndc = ((float)xpos / (float)fb_w) * 2.0f - 1.0f;
                float y_ndc = (1.0f - ((float)ypos / (float)fb_h)) * 2.0f - 1.0f;

                glm::vec2 pos(x_ndc,y_ndc);
                createPhysicsCircle(pos, radii(gen), textured_shader, "container");
            }
        });

        bool m_wireframe_mode = false;

        handler->registerCallback<input::InputType::KEY>([&](KEY_HANDLER_ARGS){
                
            if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
            // Toggle wireframe mode
            else if (key == GLFW_KEY_T && action == GLFW_PRESS) {
                m_wireframe_mode = !m_wireframe_mode;
                if (m_wireframe_mode) {
                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                } else {
                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                }
                std::cout << "Wireframe mode " << (m_wireframe_mode ? "ON" : "OFF") << std::endl;
            }
            else if (key == GLFW_KEY_C && action == GLFW_PRESS) {
                scene::graph()->clearGraph();
                physicsEngine->clearBodies();
            }
        });

        engene::EnGene app(
            on_init,
            on_fixed_update,
            on_render,
            config,
            handler
        );
        
        app.run();

    } catch (const std::runtime_error& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}