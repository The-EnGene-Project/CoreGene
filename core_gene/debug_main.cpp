#include <EnGene.h>
#include <core/scene.h>
#include <core/scene_node_builder.h>
#include <other_genes/3d_shapes/cube.h>
#include <other_genes/3d_shapes/sphere.h>
#include <other_genes/basic_input_handler.h>
#include <gl_base/error.h>
#include <gl_base/shader.h>
#include <components/all.h>
#include <components/light_component.h>
#include <3d/lights/light_manager.h>
#include <3d/lights/point_light.h>

#define BACKGROUND_COLOR 0.05f, 0.05f, 0.1f


int main() {

    auto on_init = [&](engene::EnGene& app) {
        // configures the uniforms from the base shader.
        app.getBaseShader()->configureDynamicUniform<glm::mat4>("u_model", transform::current);
        light::manager().bindToShader(app.getBaseShader());

        scene::graph()->addNode("scene rotation")
        .with<component::TransformComponent>(
            transform::Transform::Make()
            ->rotate(60,1,1,-1)
        )
        .addNode("cube")
            .with<component::GeometryComponent>(
                Cube::Make(),
                "cube"
            )
            .with<component::TransformComponent>(
                transform::Transform::Make()
                ->rotate(25,0,1,0)
                ->translate(0,-0.4,-0.4)
                ->scale(0.5,0.2,0.5)
            )
            .addNode("sphere")
                .with<component::GeometryComponent>(
                    Sphere::Make(),
                    "sphere"
                )
                .with<component::TransformComponent>(
                    transform::Transform::Make()
                    ->translate(0,2.0,0)
                    ->scale(0.4,1.0,0.4)
                );
        
        light::PointLightParams p;
        p.position = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

        scene::graph()->buildAt("scene rotation")
        .addNode("light")
            .with<component::LightComponent>(
                light::PointLight::Make(p),
                transform::Transform::Make()
                ->translate(-2,2,-0.5)
            );

        light::manager().apply();
    };

    // This function handles the fixed-timestep simulation logic.
    auto on_fixed_update = [&](double fixed_timestep) {

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
        config.title = "Lighting Engine Demo";
        config.clearColor[0] = 0.05f;
        config.clearColor[1] = 0.05f;
        config.clearColor[2] = 0.1f;
        config.clearColor[3] = 1.0f;
        config.base_vertex_shader_source = "shaders/lit_vertex.glsl";
        config.base_fragment_shader_source = "shaders/lit_fragment.glsl";

        auto* handler = new input::InputHandler();

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