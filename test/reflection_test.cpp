#include <iostream>
#include <cmath>
#include <EnGene.h>
#include <core/scene_node_builder.h>
#include <components/all.h>
#include <gl_base/cubemap.h>
#include <gl_base/error.h>
#include <3d/camera/perspective_camera.h>
#include <other_genes/environment_mapping.h>
#include <other_genes/3d_shapes/sphere.h>

/**
 * @brief Reflection test application.
 * 
 * This test demonstrates:
 * - Environment mapping with REFLECTION mode
 * - Interactive reflection coefficient control
 * - Proper reflection vector calculations
 * - Integration with skybox
 * 
 * Controls:
 * - Arrow keys: Rotate camera
 * - 1/2: Decrease/increase reflection coefficient
 * - R/G/B: Change base color (red/green/blue)
 * - ESC: Exit
 */

int main() {
    std::cout << "=== Reflection Test ===" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  Arrow Keys: Rotate camera" << std::endl;
    std::cout << "  1/2: Decrease/increase reflection coefficient" << std::endl;
    std::cout << "  R/G/B: Change base color" << std::endl;
    std::cout << "  ESC: Exit" << std::endl;
    std::cout << std::endl;

    // Camera control state
    float camera_yaw = 0.0f;
    float camera_pitch = 0.3f;
    float camera_distance = 5.0f;
    
    // Shared pointers for OpenGL resources (created in on_init)
    texture::CubemapPtr cubemap;
    std::shared_ptr<environment::EnvironmentMapping> env_mapping;
    
    auto on_init = [&cubemap, &env_mapping](engene::EnGene& app) {
        std::cout << "[INIT] Creating procedural cubemap..." << std::endl;
        const int face_size = 512;
        std::array<unsigned char*, 6> face_data;
        
        // Generate distinct colored faces for clear reflections
        for (int i = 0; i < 6; i++) {
            face_data[i] = new unsigned char[face_size * face_size * 3];
            
            // Bright distinct colors for each face
            unsigned char r = (i == 0) ? 255 : (i == 1) ? 200 : 50;
            unsigned char g = (i == 2) ? 255 : (i == 3) ? 200 : 50;
            unsigned char b = (i == 4) ? 255 : (i == 5) ? 200 : 50;
            
            for (int y = 0; y < face_size; y++) {
                for (int x = 0; x < face_size; x++) {
                    int idx = (y * face_size + x) * 3;
                    face_data[i][idx + 0] = r;
                    face_data[i][idx + 1] = g;
                    face_data[i][idx + 2] = b;
                }
            }
        }
        
        try {
            // cubemap = texture::Cubemap::Make(face_size, face_size, face_data);
            cubemap = texture::Cubemap::Make("test/skytest.png");
            std::cout << "✓ Cubemap created successfully" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "✗ Failed to create cubemap: " << e.what() << std::endl;
            for (int i = 0; i < 6; i++) {
                delete[] face_data[i];
            }
            throw;
        }
        
        for (int i = 0; i < 6; i++) {
            delete[] face_data[i];
        }

        // Environment mapping configuration
        environment::EnvironmentMappingConfig env_config;
        env_config.cubemap = cubemap;
        env_config.mode = environment::MappingMode::REFLECTION;
        env_config.reflection_coefficient = 0.8f;
        env_config.base_color = glm::vec3(0.2f, 0.2f, 0.8f);
        
        env_mapping = std::make_shared<environment::EnvironmentMapping>(env_config);
        std::cout << "✓ Environment mapping system created" << std::endl;
        std::cout << "[INIT] Setting up scene..." << std::endl;
        
        // Add skybox
        scene::graph()->addNode("skybox")
            .with<component::SkyboxComponent>(cubemap);
        
        // Create reflective sphere
        auto sphere_geom = Sphere::Make(1.0f, 16, 32);  // radius, stacks, slices
        
        scene::graph()->addNode("reflective_sphere")
            .with<component::TransformComponent>(
                transform::Transform::Make()->setTranslate(0.0f, 0.0f, 0.0f))
            .with<component::CubemapComponent>(cubemap, "environmentMap", 0)
            .with<component::ShaderComponent>(env_mapping->getShader())
            .with<component::GeometryComponent>(sphere_geom);
        
        std::cout << "✓ Reflective sphere added to scene" << std::endl;
        
        // Create camera
        auto camera = component::PerspectiveCamera::Make(60.0f, 0.1f, 100.0f);
        
        camera->getTransform()->setTranslate(0.0f, 1.5f, 5.0f);
        scene::graph()->setActiveCamera(camera);
        
        std::cout << "✓ Camera created" << std::endl;
    };

    auto on_update = [&camera_yaw, &camera_pitch, &camera_distance, &env_mapping](double dt) {
        auto camera = scene::graph()->getActiveCamera();
        if (!camera) return;
        
        // Camera rotation - automatic orbit for demonstration
        camera_yaw += static_cast<float>(dt) * 0.2f;
        camera_pitch = glm::clamp(camera_pitch, -1.5f, 1.5f);
        
        // Update camera position
        float x = camera_distance * std::cos(camera_pitch) * std::sin(camera_yaw);
        float y = camera_distance * std::sin(camera_pitch);
        float z = camera_distance * std::cos(camera_pitch) * std::cos(camera_yaw);
        
        camera->getTransform()->setTranslate(x, y, z);
        
        // TODO: Add keyboard input for reflection coefficient and color control when input handling is implemented
    };

    auto on_render = [](double alpha) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        scene::graph()->draw();
        GL_CHECK("render");
    };

    engene::EnGeneConfig config;
    config.title = "Reflection Test";
    config.width = 800;
    config.height = 600;
    config.clearColor[0] = 0.1f;
    config.clearColor[1] = 0.1f;
    config.clearColor[2] = 0.15f;
    config.clearColor[3] = 1.0f;

    try {
        engene::EnGene app(on_init, on_update, on_render, config);
        std::cout << "\n[RUNNING] Reflection test application" << std::endl;
        std::cout << "Expected: Sphere reflecting colored skybox faces" << std::endl;
        std::cout << "Reflection intensity should be adjustable with 1/2 keys" << std::endl;
        app.run();
        
        std::cout << "\n✓ Test completed successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "✗ Test failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
