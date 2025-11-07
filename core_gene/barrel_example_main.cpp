#include <EnGene.h>
#include <core/scene.h>
#include <core/scene_node_builder.h>
#include <other_genes/3d_shapes/cube.h>
#include <other_genes/3d_shapes/sphere.h>
#include <other_genes/3d_shapes/cylinder.h>
#include <other_genes/input_handlers/arcball_input_handler.h>
#include <gl_base/error.h>
#include <gl_base/shader.h>
#include <gl_base/material.h>
#include <components/all.h>
#include <components/light_component.h>
#include <components/material_component.h>
#include <3d/lights/light_manager.h>
#include <3d/lights/point_light.h>
#include <3d/camera/perspective_camera.h>

#define BACKGROUND_COLOR 0.05f, 0.05f, 0.1f
#define SCREEN_WIDTH 1600
#define SCREEN_HEIGHT 900


int main() {

    auto* handler = new input::InputHandler();
    std::shared_ptr<arcball::ArcBallInputHandler> arcball_handler;
    shader::ShaderPtr textured_shader;

    auto on_init = [&](engene::EnGene& app) {
        // Configure the uniforms from the base shader (lit shader for non-textured objects)
        app.getBaseShader()->configureDynamicUniform<glm::mat4>("u_model", transform::current);
        light::manager().bindToShader(app.getBaseShader());
        
        // Set material uniform names to match our shader
        material::Material::SetDefaultAmbientName("u_material_ambient");
        material::Material::SetDefaultDiffuseName("u_material_diffuse");
        material::Material::SetDefaultSpecularName("u_material_specular");
        material::Material::SetDefaultShininessName("u_material_shininess");
        
        // Configure material system with the shader
        material::stack()->configureShaderDefaults(app.getBaseShader());
        
        // Create textured shader for textured objects
        textured_shader = shader::Shader::Make(
            "shaders/n_map_vertex.glsl",
            "shaders/n_map_fragment.glsl"
        )
        ->configureDynamicUniform<glm::mat4>("u_model", transform::current)
        ->configureDynamicUniform<uniform::detail::Sampler2D>("u_normalMap", texture::getSamplerProvider("normal"))
        ->configureDynamicUniform<uniform::detail::Sampler2D>("u_roughnessMap", texture::getSamplerProvider("roughness"))
        ->configureDynamicUniform<uniform::detail::Sampler2D>("u_diffuseMap", texture::getSamplerProvider("diffuse"));
        

        // Base plane (ground) - white/light gray with matte appearance
        scene::graph()->addNode("base plane")
            .with<component::GeometryComponent>(
                Cube::Make(),
                "base_plane"
            )
            .with<component::MaterialComponent>(
                material::Material::Make(glm::vec3(0.9f, 0.9f, 0.9f))
                    ->setShininess(16.0f)
            )
            .with<component::TransformComponent>(
                transform::Transform::Make()
                ->translate(0, -0.6, 0)
                ->scale(5.0, 0.1, 5.0)
            );

        // Central cube (yellow/gold)
        scene::graph()->addNode("cube translate")
            .with<component::ObservedTransformComponent>(
                transform::Transform::Make()
                ->translate(0, -0.5, -0.4)
                    ->rotate(25, 0, 1, 0),
                "cube transform"
            )
            .addNode("cube")
                .with<component::GeometryComponent>(
                    Cube::Make(),
                    "cube"
                )
                .with<component::MaterialComponent>(
                    material::Material::Make(glm::vec3(1.0f, 0.84f, 0.0f))
                        ->setShininess(32.0f)
                )
                .with<component::TransformComponent>(
                    transform::Transform::Make()
                    ->scale(1.5, 1.0, 1.5) // Stretched (was 1.0, 1.0, 1.0)
                );
        
        // Pink cylinder on top of cube
        scene::graph()->buildAt("cube translate")
            .addNode("pink cylinder")
                .with<component::GeometryComponent>(
                    Cylinder::Make(0.2f, 0.4f, 32, 1, true), // Radius 0.5, Height 0.4
                    "pink_cylinder"
                )
                .with<component::MaterialComponent>(
                    material::Material::Make(glm::vec3(1.0f, 0.5f, 0.7f)) // Pink
                        ->setSpecular(glm::vec3(1.0f, 1.0f, 1.0f))
                        ->setShininess(32.0f)
                )
                .with<component::TransformComponent>(
                    transform::Transform::Make()
                    // Sits on new cube top (y=1.0, from 1.0 scale) + half-height (0.2)
                    ->translate(0.3, 1.0, -0.3) 
                );

        // Top sphere (light green) - beside the cylinder with high shininess
        scene::graph()->buildAt("cube translate")
            .addNode("top sphere")
                .with<component::GeometryComponent>(
                    Sphere::Make(32, 32),
                    "top_sphere"
                )
                .with<component::MaterialComponent>(
                    material::Material::Make(glm::vec3(0.5f, 1.0f, 0.5f))
                        // ->setSpecular(glm::vec3(1.0f, 1.0f, 1.0f))
                        ->setShininess(64.0f)
                )
                .with<component::TransformComponent>(
                    transform::Transform::Make()
                    // Placed diagonally beside pink cylinder (center y=1.2)
                    ->translate(-0.35, 1.35, 0.35) 
                    ->scale(0.35, 0.35, 0.35)
                );
        
        // Textured objects container - uses textured shader
        scene::graph()->addNode("textured objects")
            .with<component::ShaderComponent>(textured_shader)
            .addNode("side sphere")
                .with<component::GeometryComponent>(
                    Sphere::Make(32, 32),
                    "side_sphere"
                )
                .with<component::MaterialComponent>(
                    material::Material::Make(glm::vec3(1.0f, 1.0f, 1.0f))
                        ->setAmbient(glm::vec3(0.8f, 0.8f, 0.8f))
                        ->setDiffuse(glm::vec3(1.0f, 1.0f, 1.0f))
                        ->setShininess(64.0f)
                )
                .with<component::TextureComponent>(
                    texture::Texture::Make("assets/images/earth.jpg"),
                    "diffuse",
                    1
                )
                .with<component::TextureComponent>(
                    texture::Texture::Make("assets/images/earth-normal.png"),
                    "normal",
                    2
                )
                .with<component::TextureComponent>(
                    texture::Texture::Make("assets/images/noise.png"),
                    "roughness",
                    3
                )
                .with<component::TransformComponent>(
                    transform::Transform::Make()
                    ->translate(1.2, 0.05, -2.0)
                    ->scale(0.55, 0.55, 0.55)
                );
        
        // Cylinder as sibling to sphere under textured objects
        scene::graph()->buildAt("textured objects")
            .addNode("barrel location")
                .with<component::ObservedTransformComponent>(
                    transform::Transform::Make()
                    ->translate(-1.2, 0.0, 1.0),
                    "barrel location"
                )
            .addNode("test cylinder")
                .with<component::GeometryComponent>(
                    Cylinder::Make(1.0f, 1.0f, 32, 1, true),
                    "test_cylinder"
                )
                .with<component::MaterialComponent>(
                    material::Material::Make(glm::vec3(0.6f, 0.4f, 0.2f))
                        ->setAmbient(glm::vec3(0.8f, 0.8f, 0.8f))
                        ->setDiffuse(glm::vec3(1.0f, 1.0f, 1.0f))
                        // ->setAmbient(glm::vec3(0.3f, 0.2f, 0.1f))
                        // ->setDiffuse(glm::vec3(0.9f, 0.6f, 0.3f))
                        // ->setSpecular(glm::vec3(0.6f, 0.4f, 0.2f))
                        ->setShininess(16.0f)
                )
                .with<component::TextureComponent>(
                    texture::Texture::Make("assets/images/barrel.jpg"),
                    "diffuse",
                    1
                )
                .with<component::TextureComponent>(
                    texture::Texture::Make("assets/images/barrel-normal.jpg"),
                    "normal",
                    2
                )
                .with<component::TextureComponent>(
                    texture::Texture::Make("assets/images/noise.png"),
                    "roughness",
                    3
                )
                .with<component::ObservedTransformComponent>(
                    transform::Transform::Make()
                    ->translate(0.0, -0.5, 0.0)
                    ->scale(0.4, 0.7, 0.4),
                    "barrel floor"
                );
        
        // Point light
        light::PointLightParams p;
        p.ambient = glm::vec4(0.4f, 0.4f, 0.4f, 1.0f);
        p.diffuse = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        p.specular = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

        scene::graph()->addNode("light")
            .with<component::LightComponent>(
                light::PointLight::Make(p),
                transform::Transform::Make()
                ->translate(2.0, 2.0, 1.5)
            )
            .addNode("light visualizer")
                .with<component::GeometryComponent>(
                    Sphere::Make(8, 8),
                    "light_visualizer"
                )
                .with<component::TransformComponent>(
                    transform::Transform::Make()
                    ->scale(0.1, 0.1, 0.1)
                );

        // Setup camera
        scene::graph()->addNode("camera node")
            .with<component::PerspectiveCamera>();

        scene::graph()->setActiveCamera("camera node");
        // scene::graph()->getActiveCamera()->setAspectRatio(SCREEN_WIDTH / SCREEN_HEIGHT);
        scene::graph()->getActiveCamera()->setTarget(
            scene::graph()->getNodeByName("barrel location")->payload().get<component::ObservedTransformComponent>("barrel location")
        );

        
        // Bind lights and camera to textured shader
        // textured_shader->addResourceBlockToBind("CameraMatrices");
        // textured_shader->addResourceBlockToBind("CameraPosition");
        // textured_shader->addResourceBlockToBind("SceneLights");
        // textured_shader->bindRegisteredShaderResources();

        
        light::manager().bindToShader(textured_shader);
        scene::graph()->getActiveCamera()->bindToShader(textured_shader);
        textured_shader->Bake();
        
        material::stack()->configureShaderDefaults(textured_shader);

        

        light::manager().apply();

        // Attach arcball controls
        arcball_handler = arcball::attachArcballTo(*handler);
    };

    // This function handles the fixed-timestep simulation logic.
    auto on_fixed_update = [&](double fixed_timestep) {

    };

    // This function handles all rendering.
    auto on_render = [&](double alpha) {
        // The 'alpha' parameter can be used for smooth interpolation between physics states
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        arcball_handler->syncWithCameraTarget();
        
        // Render the scene graph with the updated positions.
        scene::graph()->draw();
        
        GL_CHECK("render");
    };

    try {
        engene::EnGeneConfig config;
        config.width = SCREEN_WIDTH;
        config.height = SCREEN_HEIGHT;
        config.title = "Lighting Engine Demo";
        config.clearColor[0] = 0.05f;
        config.clearColor[1] = 0.05f;
        config.clearColor[2] = 0.1f;
        config.clearColor[3] = 1.0f;
        config.base_vertex_shader_source = "shaders/lit_vertex.glsl";
        config.base_fragment_shader_source = "shaders/lit_fragment.glsl";

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