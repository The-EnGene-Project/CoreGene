#include <iostream>
#include <cmath>
#include <EnGene.h>
#include <core/scene_node_builder.h>
#include <components/all.h>
#include <gl_base/error.h>
#include <gl_base/texture.h>
#include <gl_base/cubemap.h>
#include <3d/camera/perspective_camera.h>
#include <3d/lights/directional_light.h>
#include <3d/lights/point_light.h>
#include <3d/lights/spot_light.h>
#include <other_genes/3d_shapes/sphere.h>
#include <other_genes/3d_shapes/cube.h>
#include <other_genes/3d_shapes/cylinder.h>
#include <other_genes/input_handlers/arcball_input_handler.h>
#include <gl_base/uniforms/uniform.h>
#include <other_genes/environment_mapping.h>

/**
 * @brief Project 2: Mixed Scenes (Table + Solar System)
 *
 * This application demonstrates both scenes with scene switching:
 * - Scene 1: Table Scene with lamp, sphere, cylinder
 * - Scene 2: Mini-Solar System with animated planets
 *
 * Controls:
 * - '1' Key: Switch to Table Scene
 * - '2' Key: Switch to Solar System Scene
 * - 'C' Key: Switch cameras (Solar System only)
 * - Left Mouse Button + Drag: Rotate camera (orbit)
 * - Middle Mouse Button + Drag: Pan camera
 * - Mouse Wheel: Zoom in/out
 * - ESC: Exit
 */

// Helper to create a procedural texture (checkered pattern)
texture::TexturePtr createProceduralTexture(const glm::vec3& color1, const glm::vec3& color2) {
    const int texSize = 64;
    auto* data = new unsigned char[texSize * texSize * 3];
    for (int y = 0; y < texSize; ++y) {
        for (int x = 0; x < texSize; ++x) {
            int idx = (y * texSize + x) * 3;
            bool isColor1 = ((x / 8) % 2) == ((y / 8) % 2);
            data[idx + 0] = static_cast<unsigned char>(isColor1 ? color1.r * 255 : color2.r * 255);
            data[idx + 1] = static_cast<unsigned char>(isColor1 ? color1.g * 255 : color2.g * 255);
            data[idx + 2] = static_cast<unsigned char>(isColor1 ? color1.b * 255 : color2.b * 255);
        }
    }
    auto tex = texture::Texture::Make(texSize, texSize, data);
    delete[] data;
    return tex;
}

// Helper to create a procedural cubemap
texture::CubemapPtr createProceduralCubemap(bool starfield = false) {
    const int face_size = starfield ? 512 : 256;
    std::array<unsigned char*, 6> face_data;
    
    for (int i = 0; i < 6; i++) {
        face_data[i] = new unsigned char[face_size * face_size * 3];
        
        if (starfield) {
            // Fill with black
            std::memset(face_data[i], 0, face_size * face_size * 3);
            // Add random "stars"
            for (int k = 0; k < 200; k++) {
                int x = rand() % face_size;
                int y = rand() % face_size;
                int idx = (y * face_size + x) * 3;
                unsigned char brightness = 150 + (rand() % 106);
                face_data[i][idx + 0] = brightness;
                face_data[i][idx + 1] = brightness;
                face_data[i][idx + 2] = brightness;
            }
        } else {
            // Simple color gradients
            for (int y = 0; y < face_size; y++) {
                for (int x = 0; x < face_size; x++) {
                    int idx = (y * face_size + x) * 3;
                    float gradient_x = static_cast<float>(x) / face_size;
                    float gradient_y = static_cast<float>(y) / face_size;
                    face_data[i][idx + 0] = static_cast<unsigned char>(50 + (i % 2) * 150 * gradient_x);
                    face_data[i][idx + 1] = static_cast<unsigned char>(50 + (i % 3) * 150 * gradient_y);
                    face_data[i][idx + 2] = static_cast<unsigned char>(50 + (i % 4) * 150 * (1.0f - gradient_x));
                }
            }
        }
    }
    
    auto cubemap = texture::Cubemap::Make(face_size, face_size, face_data);
    for (int i = 0; i < 6; i++) delete[] face_data[i];
    return cubemap;
}

enum class ActiveScene {
    TABLE,
    SOLAR_SYSTEM
};

int main() {
    auto* handler = new input::InputHandler();
    
    // Separate arcball controllers for each interactive camera
    std::shared_ptr<arcball::ArcBallController> table_arcball;
    std::shared_ptr<arcball::ArcBallController> solar_arcball;

    shader::ShaderPtr phong_shader;
    shader::ShaderPtr emissive_shader;

    // Environment mapping for cylinder
    std::shared_ptr<environment::EnvironmentMapping> cylinder_env_mapping;

    // Shared resources
    texture::CubemapPtr tableCubemap;
    texture::CubemapPtr spaceCubemap;
    
    // Table scene textures
    texture::TexturePtr woodTexture;
    texture::TexturePtr marbleTexture;
    texture::TexturePtr roughnessTexture;
    
    // Solar system textures
    texture::TexturePtr sunTexture;
    texture::TexturePtr earthTexture;
    texture::TexturePtr earthRoughness;
    texture::TexturePtr moonTexture;
    texture::TexturePtr mercuryTexture;

    // Cameras
    std::shared_ptr<component::PerspectiveCamera> table_cam;
    std::shared_ptr<component::PerspectiveCamera> global_cam;
    std::shared_ptr<component::PerspectiveCamera> earth_cam;

    ActiveScene current_scene = ActiveScene::TABLE;

    auto on_init = [&](engene::EnGene& app) {
        std::cout << "=== Project 2: Mixed Scenes ===" << std::endl;
        std::cout << "Press '1' for Table Scene, '2' for Solar System" << std::endl;

        // 1. Create Lights First (so SceneLights UBO exists before shader compilation)
        std::cout << "[INIT] Creating lights..." << std::endl;
        
        // Dummy lights that will be replaced by scene-specific lights
        light::DirectionalLightParams dummy_dir;
        dummy_dir.base_direction = glm::vec3(0.0f, -1.0f, 0.0f);
        dummy_dir.ambient = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
        dummy_dir.diffuse = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
        dummy_dir.specular = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
        auto dummy_light = light::DirectionalLight::Make(dummy_dir);
        scene::graph()->addNode("dummy_light")
            .with<component::LightComponent>(dummy_light, transform::Transform::Make());
        
        light::manager().apply();
        std::cout << "✓ Initial lights created" << std::endl;

        // 2. Load Shaders
        try {
            phong_shader = shader::Shader::Make("test/proj2/shaders/phong.vert", "test/proj2/shaders/phong.frag");
            phong_shader->addResourceBlockToBind("CameraMatrices");
            phong_shader->addResourceBlockToBind("CameraPosition");
            phong_shader->addResourceBlockToBind("SceneLights");
            phong_shader->configureDynamicUniform<glm::mat4>("u_model", transform::current);
            phong_shader->configureDynamicUniform<glm::mat4>("u_projectorViewProj", 
                []() { return glm::mat4(1.0f); }); // Dummy projector matrix
            phong_shader->configureDynamicUniform<float>("u_reflectionFactor", 
                []() { return 0.4f; });
            material::stack()->configureShaderDefaults(phong_shader);
            phong_shader->Bake();

            emissive_shader = shader::Shader::Make("test/proj2/shaders/sun.vert", "test/proj2/shaders/sun.frag");
            emissive_shader->addResourceBlockToBind("CameraMatrices");
            emissive_shader->configureDynamicUniform<glm::mat4>("u_model", transform::current);
            material::stack()->configureShaderDefaults(emissive_shader);
            emissive_shader->Bake();

        } catch (const std::exception& e) {
            std::cerr << "Shader compilation failed: " << e.what() << std::endl;
            throw;
        }

        // 3. Create Textures and Skyboxes
        tableCubemap = createProceduralCubemap(true);
        spaceCubemap = createProceduralCubemap(true);
        
        // 3.5. Create Environment Mapping for Cylinder
        environment::EnvironmentMappingConfig env_config;
        env_config.cubemap = tableCubemap;
        env_config.mode = environment::MappingMode::FRESNEL;
        env_config.index_of_refraction = 1.5f;
        env_config.fresnel_power = 2.0f;
        env_config.base_color = glm::vec3(0.1f, 0.4f, 0.8f); // Blue-ish base color
        cylinder_env_mapping = std::make_shared<environment::EnvironmentMapping>(env_config);
        
        woodTexture = createProceduralTexture({0.4f, 0.2f, 0.1f}, {0.3f, 0.15f, 0.05f});
        marbleTexture = createProceduralTexture({0.9f, 0.9f, 0.9f}, {0.7f, 0.7f, 0.8f});
        roughnessTexture = createProceduralTexture({0.8f, 0.8f, 0.8f}, {0.2f, 0.2f, 0.2f});
        
        sunTexture = createProceduralTexture({0.9f, 0.7f, 0.2f}, {1.0f, 0.5f, 0.0f});
        earthTexture = createProceduralTexture({0.2f, 0.5f, 0.8f}, {0.1f, 0.6f, 0.2f});
        earthRoughness = createProceduralTexture({0.8f, 0.8f, 0.8f}, {0.1f, 0.1f, 0.1f});
        moonTexture = createProceduralTexture({0.7f, 0.7f, 0.7f}, {0.5f, 0.5f, 0.5f});
        mercuryTexture = createProceduralTexture({0.5f, 0.4f, 0.4f}, {0.3f, 0.3f, 0.3f});

        // 4. Create Geometries
        auto cube_geom = Cube::Make();
        auto sphere_geom = Sphere::Make(1.0f, 32, 64);
        auto cylinder_geom = Cylinder::Make(1.0f, 1.0f, 32);

        // 5. Create Materials
        auto table_material = material::Material::Make(glm::vec3(0.0f, 0.6f, 0.0f));
        table_material->setShininess(32.0f);

        auto leg_material = material::Material::Make(glm::vec3(0.6f, 0.3f, 0.1f));
        leg_material->setShininess(16.0f);

        auto sphere_material = material::Material::Make(glm::vec3(1.0f, 1.0f, 1.0f));
        sphere_material->setShininess(128.0f);

        auto lamp_material = material::Material::Make(glm::vec3(0.1f, 0.4f, 0.8f));
        lamp_material->setShininess(64.0f);

        auto planet_material = material::Material::Make(glm::vec3(1.0f, 1.0f, 1.0f));
        planet_material->setShininess(32.0f);

        auto sun_material = material::Material::Make(glm::vec3(1.0f, 1.0f, 1.0f));

        // 6. Common Components
        auto phong_shader_comp = component::ShaderComponent::Make(phong_shader);
        auto no_clip = component::ClipPlaneComponent::Make("clip_planes", "num_clip_planes");

        // ========== TABLE SCENE ==========
        std::cout << "Building Table Scene..." << std::endl;

        // Remove dummy light
        auto dummy_node = scene::graph()->getNodeByName("dummy_light");
        if (dummy_node) {
            scene::graph()->removeNode(dummy_node);
        }

        // Table Scene Root
        scene::graph()->addNode("table_scene")
            .with<component::TransformComponent>(transform::Transform::Make())
            .with<component::SkyboxComponent>(tableCubemap);

        // Table Scene Lights
        light::DirectionalLightParams dir_params;
        dir_params.base_direction = glm::vec3(-0.5f, -1.0f, -0.8f);
        dir_params.ambient = glm::vec4(0.05f, 0.05f, 0.05f, 1.0f);
        dir_params.diffuse = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
        dir_params.specular = glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);
        auto directional_light = light::DirectionalLight::Make(dir_params);
        scene::graph()->buildAt("table_scene")
            .addNode("table_dir_light")
                .with<component::LightComponent>(directional_light, transform::Transform::Make());

        light::SpotLightParams spot_params;
        spot_params.position = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        spot_params.base_direction = glm::vec3(0.0f, -1.0f, 0.0f);
        spot_params.ambient = glm::vec4(0.1f, 0.1f, 0.0f, 1.0f);
        spot_params.diffuse = glm::vec4(1.0f, 1.0f, 0.7f, 1.0f);
        spot_params.specular = glm::vec4(1.0f, 1.0f, 0.7f, 1.0f);
        spot_params.constant = 1.0f;
        spot_params.linear = 0.09f;
        spot_params.quadratic = 0.032f;
        spot_params.cutOff = glm::cos(glm::radians(45.0f));
        auto spot_light = light::SpotLight::Make(spot_params);
        auto spot_transform = transform::Transform::Make();
        scene::graph()->buildAt("table_scene")
            .addNode("table_spot_light")
                .with<component::LightComponent>(spot_light, spot_transform);

        // Apply lights to UBO
        light::manager().apply();
        std::cout << "✓ Table scene lights created and applied" << std::endl;

        // Table Scene Variables (shared uniforms for all table objects)
        auto table_scene_vars = component::VariableComponent::Make(
            uniform::Uniform<glm::vec3>::Make("fogcolor", []() { return glm::vec3(0.5f, 0.5f, 0.5f); })
        );
        table_scene_vars->addUniform(uniform::Uniform<float>::Make("fogdensity", []() { return 0.05f; }));
        table_scene_vars->addUniform(uniform::Uniform<bool>::Make("u_enableProjectiveTex", []() { return false; }));
        table_scene_vars->addUniform(uniform::Uniform<bool>::Make("u_hasNormalMap", []() { return false; }));
        table_scene_vars->addUniform(uniform::Uniform<bool>::Make("u_enableReflection", []() { return false; }));

        // Table Top - separate transform node from geometry node
        auto table_top_vars = component::VariableComponent::Make(
            uniform::Uniform<bool>::Make("u_hasDiffuseMap", [](){ return false; })
        );
        table_top_vars->addUniform(uniform::Uniform<bool>::Make("u_hasRoughnessMap", [](){ return false; }));
        table_top_vars->addUniform(uniform::Uniform<bool>::Make("u_enableReflection", [](){ return false; }));
        
        scene::graph()->buildAt("table_scene")
            .addNode("table_top")
                .with<component::TransformComponent>(
                    transform::Transform::Make()->translate(0.0f, 0.0f, 0.0f))
            .addNode("table_top_geom")
                .with<component::TransformComponent>(
                    transform::Transform::Make()->scale(6.0f, 0.2f, 4.0f))
                .addComponent(phong_shader_comp)
                .with<component::MaterialComponent>(table_material)
                // .with<component::TextureComponent>(woodTexture, "u_diffuseMap", 0)
                .addComponent(table_top_vars)
                .addComponent(table_scene_vars)
                .addComponent(no_clip)
                .addComponent(component::GeometryComponent::Make(cube_geom));

        // Table Legs
        auto leg_vars = component::VariableComponent::Make(
            uniform::Uniform<bool>::Make("u_hasDiffuseMap", [](){ return false; })
        );
        leg_vars->addUniform(uniform::Uniform<bool>::Make("u_hasRoughnessMap", [](){ return false; }));
        leg_vars->addUniform(uniform::Uniform<bool>::Make("u_enableReflection", [](){ return false; }));
        
        std::vector<glm::vec3> leg_positions = {
            glm::vec3( 2.8f, -2.0f,  1.8f), glm::vec3(-2.8f, -2.0f,  1.8f),
            glm::vec3( 2.8f, -2.0f, -1.8f), glm::vec3(-2.8f, -2.0f, -1.8f)
        };
        for (int i = 0; i < 4; ++i) {
            scene::graph()->buildAt("table_top")
                .addNode("table_leg_" + std::to_string(i))
                    .with<component::TransformComponent>(
                        transform::Transform::Make()->translate(leg_positions[i].x, leg_positions[i].y, leg_positions[i].z)->scale(0.2f, 2.0f, 0.2f))
                    .addComponent(phong_shader_comp)
                    .with<component::MaterialComponent>(leg_material)
                    .addComponent(leg_vars)
                    .addComponent(table_scene_vars)
                    .addComponent(no_clip)
                    .addComponent(component::GeometryComponent::Make(cylinder_geom));
        }

        // Sphere with clip plane
        auto sphere_clip = component::ClipPlaneComponent::Make("clip_planes", "num_clip_planes");
        sphere_clip->addPlane(0.5f, 0.5f, 0.0f, 0.0f);

        auto sphere_vars = component::VariableComponent::Make(
            uniform::Uniform<bool>::Make("u_hasDiffuseMap", [](){ return true; })
        );
        sphere_vars->addUniform(uniform::Uniform<bool>::Make("u_hasRoughnessMap", [](){ return true; }));
        sphere_vars->addUniform(uniform::Uniform<bool>::Make("u_enableReflection", [](){ return true; }));

        scene::graph()->buildAt("table_top")
            .addNode("table_sphere")
                .with<component::TransformComponent>(
                    transform::Transform::Make()->translate(-1.0f, 0.7f, 0.5f)->scale(0.5f, 0.5f, 0.5f))
                .addComponent(phong_shader_comp)
                .with<component::MaterialComponent>(sphere_material)
                .with<component::TextureComponent>(marbleTexture, "u_diffuseMap", 0)
                .with<component::TextureComponent>(roughnessTexture, "u_roughnessMap", 1)
                .with<component::CubemapComponent>(tableCubemap, "u_skybox", 2)
                .addComponent(sphere_vars)
                .addComponent(table_scene_vars)
                .addComponent(sphere_clip)
                .addComponent(component::GeometryComponent::Make(sphere_geom));

        // Cylinder with Fresnel environment mapping
        scene::graph()->buildAt("table_top")
            .addNode("table_cylinder")
                .with<component::TransformComponent>(
                    transform::Transform::Make()->translate(-2.0f, 0.75f, -1.0f)->scale(0.5f, 0.75f, 0.5f))
                .with<component::ShaderComponent>(cylinder_env_mapping->getShader())
                .with<component::CubemapComponent>(tableCubemap, "environmentMap", 0)
                .addComponent(component::GeometryComponent::Make(cylinder_geom));

        // Lamp - separate transform nodes from geometry nodes
        scene::graph()->buildAt("table_top")
            .addNode("lamp_base")
                .with<component::TransformComponent>(
                    transform::Transform::Make()->translate(2.0f, 0.2f, 0.0f))
            .addNode("lamp_base_geom")
                .with<component::TransformComponent>(
                    transform::Transform::Make()->scale(0.7f, 0.1f, 0.7f))
                .addComponent(phong_shader_comp)
                .with<component::MaterialComponent>(lamp_material)
                .addComponent(leg_vars)
                .addComponent(table_scene_vars)
                .addComponent(no_clip)
                .with<component::GeometryComponent>(cube_geom);
        
        scene::graph()->buildAt("lamp_base")
            .addNode("lamp_arm1")
                .with<component::TransformComponent>(
                    transform::Transform::Make()->translate(0.0f, 0.1f, 0.0f)
                    ->rotate(-30.0f, 0.0f, 0.0f, 1.0f))
            .addNode("lamp_arm1_geom")
                .with<component::TransformComponent>(
                    transform::Transform::Make()->scale(0.1f, 1.0f, 0.1f))
                .addComponent(phong_shader_comp)
                .with<component::MaterialComponent>(lamp_material)
                .addComponent(leg_vars)
                .addComponent(table_scene_vars)
                .addComponent(no_clip)
                .addComponent(component::GeometryComponent::Make(cube_geom));
        
        scene::graph()->buildAt("lamp_arm1")
            .addNode("lamp_arm2")
                .with<component::TransformComponent>(
                    transform::Transform::Make()->translate(0.0f, 1.0f, 0.0f)
                    ->rotate(60.0f, 0.0f, 0.0f, 1.0f))
            .addNode("lamp_arm2_geom")
                .with<component::TransformComponent>(
                    transform::Transform::Make()->scale(0.1f, 1.0f, 0.1f))
              .addComponent(phong_shader_comp)
                .with<component::MaterialComponent>(lamp_material)
                .addComponent(leg_vars)
                .addComponent(table_scene_vars)
                .addComponent(no_clip)
                .addComponent(component::GeometryComponent::Make(cube_geom));
        
        scene::graph()->buildAt("lamp_arm2")
            .addNode("lamp_head")
                .with<component::TransformComponent>(
                    transform::Transform::Make()->translate(0.2f, 0.5f, 0.0f)
                    ->rotate(75.0f, 0.0f, 0.0f, 1.0f))
            .addNode("lamp_head_geom")
                .with<component::TransformComponent>(
                    transform::Transform::Make()->scale(0.4f, 0.6f, 0.4f))
                .addComponent(phong_shader_comp)
                .with<component::MaterialComponent>(lamp_material)
                .addComponent(leg_vars)
                .addComponent(table_scene_vars)
                .addComponent(no_clip)
                .addComponent(component::GeometryComponent::Make(cylinder_geom));

        // Parent spotlight to lamp head
        auto spot_light_node = scene::graph()->getNodeByName("table_spot_light");
        auto lamp_head_node = scene::graph()->getNodeByName("lamp_head_geom");
        if (spot_light_node && lamp_head_node) {
            // Remove from current parent
            auto current_parent = spot_light_node->getParent();
            if (current_parent) {
                current_parent->removeChild(spot_light_node);
            }
            // Add to new parent
            lamp_head_node->addChild(spot_light_node);
        }

        // ========== SOLAR SYSTEM SCENE ==========
        std::cout << "Building Solar System Scene..." << std::endl;

        // Solar System Root
        auto solar_scene_node = scene::graph()->addNode("solar_scene")
            .with<component::TransformComponent>(transform::Transform::Make())
            .with<component::SkyboxComponent>(spaceCubemap);
        
        // Start with solar system hidden
        scene::graph()->getNodeByName("solar_scene")->setApplicability(false);

        // Solar System Light (Sun)
        light::PointLightParams sun_light_params;
        sun_light_params.position = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        sun_light_params.ambient = glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);
        sun_light_params.diffuse = glm::vec4(1.0f, 1.0f, 0.9f, 1.0f);
        sun_light_params.specular = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        sun_light_params.constant = 1.0f;
        sun_light_params.linear = 0.014f;
        sun_light_params.quadratic = 0.0007f;
        auto sun_light = light::PointLight::Make(sun_light_params);

        // Solar Scene Variables (shared uniforms for all solar objects)
        auto solar_scene_vars = component::VariableComponent::Make(
            uniform::Uniform<glm::vec3>::Make("fogcolor", []() { return glm::vec3(0.0f, 0.0f, 0.0f); })
        );
        solar_scene_vars->addUniform(uniform::Uniform<float>::Make("fogdensity", []() { return 0.0f; }));
        solar_scene_vars->addUniform(uniform::Uniform<bool>::Make("u_enableProjectiveTex", []() { return false; }));
        solar_scene_vars->addUniform(uniform::Uniform<bool>::Make("u_enableReflection", []() { return false; }));
        solar_scene_vars->addUniform(uniform::Uniform<bool>::Make("u_hasNormalMap", []() { return false; }));

        // Sun
        auto sun_vars = component::VariableComponent::Make(
            uniform::Uniform<bool>::Make("u_hasDiffuseMap", [](){ return true; })
        );
        
        scene::graph()->buildAt("solar_scene")
            .addNode("sun")
                .with<component::TransformComponent>(transform::Transform::Make()->scale(5.0f, 5.0f, 5.0f))
                .with<component::ShaderComponent>(emissive_shader)
                .with<component::MaterialComponent>(sun_material)
                .with<component::TextureComponent>(sunTexture, "u_diffuseMap", 0)
                .addComponent(sun_vars)
                .with<component::LightComponent>(sun_light, transform::Transform::Make())
                .with<component::GeometryComponent>(sphere_geom)
            .addNode("mercury_orbit")
                .with<component::TransformComponent>(transform::Transform::Make());

        // Mercury
        auto mercury_vars = component::VariableComponent::Make(
            uniform::Uniform<bool>::Make("u_hasDiffuseMap", [](){ return true; })
        );
        mercury_vars->addUniform(uniform::Uniform<bool>::Make("u_hasRoughnessMap", [](){ return false; }));
        
        scene::graph()->buildAt("mercury_orbit")
            .addNode("mercury")
                .with<component::TransformComponent>(
                    transform::Transform::Make()->translate(8.0f, 0.0f, 0.0f)->scale(0.38f, 0.38f, 0.38f))
                .addComponent(phong_shader_comp)
                .with<component::MaterialComponent>(planet_material)
                .with<component::TextureComponent>(mercuryTexture, "u_diffuseMap", 0)
                .addComponent(mercury_vars)
                .addComponent(solar_scene_vars)
                .addComponent(no_clip)
                .with<component::GeometryComponent>(sphere_geom);

        // Earth Orbit
        scene::graph()->buildAt("sun")
            .addNode("earth_orbit")
                .with<component::TransformComponent>(transform::Transform::Make());

        // Earth
        auto earth_vars = component::VariableComponent::Make(
            uniform::Uniform<bool>::Make("u_hasDiffuseMap", [](){ return true; })
        );
        earth_vars->addUniform(uniform::Uniform<bool>::Make("u_hasRoughnessMap", [](){ return true; }));
        
        scene::graph()->buildAt("earth_orbit")
            .addNode("earth")
                .with<component::TransformComponent>(
                    transform::Transform::Make()->translate(15.0f, 0.0f, 0.0f)->scale(1.0f, 1.0f, 1.0f))
                .addComponent(phong_shader_comp)
                .with<component::MaterialComponent>(planet_material)
                .with<component::TextureComponent>(earthTexture, "u_diffuseMap", 0)
                .with<component::TextureComponent>(earthRoughness, "u_roughnessMap", 1)
                .addComponent(earth_vars)
                .addComponent(solar_scene_vars)
                .addComponent(no_clip)
                .with<component::GeometryComponent>(sphere_geom)
            .addNode("moon_orbit")
                .with<component::TransformComponent>(transform::Transform::Make());

        // Moon (reuse mercury_vars since it has the same settings)
        // Use ObservedTransformComponent so the Earth camera can track it via observer pattern
        scene::graph()->buildAt("moon_orbit")
            .addNode("moon")
                .with<component::ObservedTransformComponent>(
                    transform::Transform::Make()->translate(2.5f, 0.0f, 0.0f)->scale(0.27f, 0.27f, 0.27f))
                .addComponent(phong_shader_comp)
                .with<component::MaterialComponent>(planet_material)
                .with<component::TextureComponent>(moonTexture, "u_diffuseMap", 0)
                .addComponent(mercury_vars)
                .addComponent(solar_scene_vars)
                .addComponent(no_clip)
                .with<component::GeometryComponent>(sphere_geom);

        // ========== CAMERAS ==========
        // ARCHITECTURE HIGHLIGHT: Declarative Camera Setup with Zero Manual Updates!
        // 
        // The Earth camera demonstrates the power of EnGene's observer pattern:
        // 1. Camera is a CHILD of Earth node → automatically inherits Earth's world transform
        // 2. Camera uses setTarget(moon) → automatically looks at Moon via observer pattern
        // 3. NO manual calculations needed in update loop!
        // 
        // This is possible because:
        // - ObservedTransformComponent observes ALL ancestor transforms (new feature!)
        // - PerspectiveCamera observes its target's transform
        // - Everything updates automatically through the observer pattern
        //
        // Table and Global cameras are root-level to work with arcball controller
        
        // Table Camera - create as root-level node
        table_cam = component::PerspectiveCamera::Make(60.0f, 1.0f, 100.0f);
        table_cam->getTransform()->setTranslate(0.0f, 4.0f, 10.0f)
            ->rotate(-20.0f, 1.0f, 0.0f, 0.0f);
        
        scene::graph()->addNode("table_camera_node")
            .addComponent(table_cam);

        // Solar System Global Camera - create as root-level node
        global_cam = component::PerspectiveCamera::Make(60.0f, 1.0f, 1000.0f);
        global_cam->getTransform()->setTranslate(0.0f, 15.0f, 35.0f)
            ->rotate(-25.0f, 1.0f, 0.0f, 0.0f);
        
        scene::graph()->addNode("global_camera_node")
            .addComponent(global_cam);

        // Earth Camera - NOW AS A CHILD OF EARTH!
        // Thanks to ObservedTransformComponent's ancestor observation, the camera
        // automatically inherits Earth's world position and rotation!
        // AND it automatically looks at the Moon via setTarget()!
        earth_cam = component::PerspectiveCamera::Make(45.0f, 0.1f, 100.0f);
        earth_cam->getTransform()->translate(0.0f, 2.0f, 0.0f);  // Offset from Earth center
        
        // Get the Moon's ObservedTransformComponent to use as target
        auto moon_node = scene::graph()->getNodeByName("moon");
        if (moon_node) {
            auto moon_transform = moon_node->payload().get<component::ObservedTransformComponent>();
            if (moon_transform) {
                earth_cam->setTarget(moon_transform);  // Camera automatically tracks Moon!
            }
        }
        
        scene::graph()->buildAt("earth")
            .addNode("earth_camera_node")
                .addComponent(earth_cam);

        // Set initial camera
        scene::graph()->setActiveCamera(table_cam);

        // ========== INPUT HANDLERS ==========
        light::manager().apply();
        
        // Create separate arcball controllers for each scene
        // Table Scene Arcball - closer zoom limits, orbits around table center
        table_arcball = arcball::ArcBallController::CreateFromCameraNode("table_camera_node");
        table_arcball->setTarget(glm::vec3(0.0f, 0.0f, 0.0f));
        table_arcball->setZoomLimits(2.0f, 20.0f);
        table_arcball->setSensitivity(0.005f, 1.0f, 0.001f);
        table_arcball->attachTo(*handler);  // Start with table arcball active
        
        // Solar System Arcball - wider zoom limits, orbits around solar system center
        solar_arcball = arcball::ArcBallController::CreateFromCameraNode("global_camera_node");
        solar_arcball->setTarget(glm::vec3(0.0f, 0.0f, 0.0f));
        solar_arcball->setZoomLimits(10.0f, 100.0f);
        solar_arcball->setSensitivity(0.005f, 1.0f, 0.002f);
        // solar_arcball is created but not attached yet (will attach when switching to solar scene)

        handler->registerCallback<input::InputType::KEY>([&](GLFWwindow* window, int key, int scancode, int action, int mods) {
            if (action != GLFW_PRESS) return; // Only respond to key press, not release
            
            if (key == GLFW_KEY_1) {
                std::cout << "Switching to Table Scene" << std::endl;
                current_scene = ActiveScene::TABLE;
                
                // Detach current arcball (if any)
                solar_arcball->detachFrom(*handler);
                
                // Switch scene visibility
                scene::graph()->getNodeByName("table_scene")->setApplicability(true);
                scene::graph()->getNodeByName("solar_scene")->setApplicability(false);
                
                // Switch camera and attach table arcball
                scene::graph()->setActiveCamera(table_cam);
                table_arcball->attachTo(*handler);
                
                light::manager().apply();
            }
            else if (key == GLFW_KEY_2) {
                std::cout << "Switching to Solar System Scene" << std::endl;
                current_scene = ActiveScene::SOLAR_SYSTEM;
                
                // Detach current arcball (if any)
                table_arcball->detachFrom(*handler);
                
                // Switch scene visibility
                scene::graph()->getNodeByName("table_scene")->setApplicability(false);
                scene::graph()->getNodeByName("solar_scene")->setApplicability(true);
                
                // Switch camera and attach solar arcball
                scene::graph()->setActiveCamera(global_cam);
                solar_arcball->attachTo(*handler);
                
                light::manager().apply();
            }
            else if ((key == GLFW_KEY_C) && current_scene == ActiveScene::SOLAR_SYSTEM) {
                if (scene::graph()->getActiveCamera() == global_cam) {
                    std::cout << "Switching to Earth Camera (view from Earth looking at Moon)" << std::endl;
                    
                    // Detach arcball - Earth camera is fixed relative to Earth
                    solar_arcball->detachFrom(*handler);
                    
                    // Switch to Earth camera
                    scene::graph()->setActiveCamera(earth_cam);
                    
                    std::cout << "  Camera locked to Earth, tracking Moon" << std::endl;
                } else {
                    std::cout << "Switching to Global Camera (free orbit)" << std::endl;
                    
                    // Switch back to global camera
                    scene::graph()->setActiveCamera(global_cam);
                    
                    // Re-attach solar arcball for free orbit
                    solar_arcball->attachTo(*handler);
                }
            }
        });

        std::cout << "✓ Scenes initialized." << std::endl;
        std::cout << "Controls:" << std::endl;
        std::cout << "  '1' - Table Scene" << std::endl;
        std::cout << "  '2' - Solar System Scene" << std::endl;
        std::cout << "  'C' - Switch cameras (Solar System only)" << std::endl;
    };

    auto on_update = [&](double dt) {
        // Only update solar system animations when that scene is active
        if (current_scene == ActiveScene::SOLAR_SYSTEM) {
            // Rotate Mercury orbit
            auto mercury_orbit = scene::graph()->getNodeByName("mercury_orbit");
            if (mercury_orbit) {
                auto t = mercury_orbit->payload().get<component::TransformComponent>();
                if (t) t->getTransform()->rotate(glm::degrees(dt * 0.8f), 0.0f, 1.0f, 0.0f);
            }
            
            // Rotate Mercury on its axis
            auto mercury = scene::graph()->getNodeByName("mercury");
            if (mercury) {
                auto t = mercury->payload().get<component::TransformComponent>();
                if (t) t->getTransform()->rotate(glm::degrees(dt * 0.2f), 0.0f, 1.0f, 0.0f);
            }
            
            // Rotate Earth orbit
            auto earth_orbit = scene::graph()->getNodeByName("earth_orbit");
            if (earth_orbit) {
                auto t = earth_orbit->payload().get<component::TransformComponent>();
                if (t) t->getTransform()->rotate(glm::degrees(dt * 0.5f), 0.0f, 1.0f, 0.0f);
            }
            
            // Rotate Earth on its axis
            auto earth_node = scene::graph()->getNodeByName("earth");
            if (earth_node) {
                auto t = earth_node->payload().get<component::TransformComponent>();
                if (t) t->getTransform()->rotate(glm::degrees(dt * 1.0f), 0.0f, 1.0f, 0.0f);
            }
            
            // Rotate Moon orbit (with slight tilt on Z axis)
            auto moon_orbit = scene::graph()->getNodeByName("moon_orbit");
            if (moon_orbit) {
                auto t = moon_orbit->payload().get<component::TransformComponent>();
                if (t) t->getTransform()->rotate(glm::degrees(dt * 2.0f), 0.0f, 1.0f, 0.1f);
            }
            
            // NO MANUAL CAMERA UPDATE NEEDED!
            // The Earth camera automatically:
            // 1. Follows Earth's position/rotation (via scene graph hierarchy + ObservedTransformComponent)
            // 2. Looks at the Moon (via setTarget() + observer pattern)
            // Everything is handled by the engine! 🎉
        }
    };

    auto on_render = [](double alpha) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        scene::graph()->draw();
        GL_CHECK("render");
    };

    engene::EnGeneConfig config;
    config.title = "Project 2: Mixed Scenes (Table + Solar System)";
    config.width = 1280;
    config.height = 720;
    config.clearColor[0] = 0.5f;
    config.clearColor[1] = 0.5f;
    config.clearColor[2] = 0.5f;
    config.clearColor[3] = 1.0f;

    try {
        engene::EnGene app(on_init, on_update, on_render, config, handler);
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Application failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
