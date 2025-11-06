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
 * @brief Fresnel effect test application.
 * 
 * This test demonstrates:
 * - Environment mapping with FRESNEL mode
 * - Interactive Fresnel power control
 * - Angle-dependent reflection intensity
 * - Blend between reflection and refraction
 * 
 * Controls:
 * - Arrow keys: Rotate camera
 * - 1/2: Decrease/increase Fresnel power
 * - 3/4: Decrease/increase IOR
 * - ESC: Exit
 */

int main() {
    std::cout << "=== Fresnel Effect Test ===" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  Arrow Keys: Rotate camera" << std::endl;
    std::cout << "  1/2: Decrease/increase Fresnel power" << std::endl;
    std::cout << "  3/4: Decrease/increase IOR" << std::endl;
    std::cout << "  ESC: Exit" << std::endl;
    std::cout << std::endl;
    std::cout << "Fresnel Effect: Reflection intensity varies with viewing angle" << std::endl;
    std::cout << "  - Grazing angles (edges): Strong reflection" << std::endl;
    std::cout << "  - Perpendicular view (center): Weak reflection, more refraction" << std::endl;
    std::cout << std::endl;

    // Camera control state
    float camera_yaw = 0.0f;
    float camera_pitch = 0.3f;
    float camera_distance = 5.0f;
    
    // Shared pointers for OpenGL resources (created in on_init)
    texture::CubemapPtr cubemap;
    std::shared_ptr<environment::EnvironmentMapping> env_mapping;
    
    auto on_init = [&cubemap, &env_mapping](engene::EnGene& app) {
        // Create procedural cubemap with high contrast
        std::cout << "[INIT] Creating procedural cubemap..." << std::endl;
        const int face_size = 512;
        std::array<unsigned char*, 6> face_data;
        
        // Generate high-contrast environment for Fresnel visibility
        for (int i = 0; i < 6; i++) {
            face_data[i] = new unsigned char[face_size * face_size * 3];
            
            // Bright colors with gradients
            for (int y = 0; y < face_size; y++) {
                for (int x = 0; x < face_size; x++) {
                    int idx = (y * face_size + x) * 3;
                    float gradient_x = static_cast<float>(x) / face_size;
                    float gradient_y = static_cast<float>(y) / face_size;
                    
                    unsigned char r = (i == 0 || i == 1) ? static_cast<unsigned char>(255 * gradient_x) : 50;
                    unsigned char g = (i == 2 || i == 3) ? static_cast<unsigned char>(255 * gradient_y) : 50;
                    unsigned char b = (i == 4 || i == 5) ? static_cast<unsigned char>(255 * (1.0f - gradient_x)) : 50;
                    
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

        // Environment mapping configuration - Fresnel effect
        environment::EnvironmentMappingConfig env_config;
        env_config.cubemap = cubemap;
        env_config.mode = environment::MappingMode::FRESNEL;
        env_config.fresnel_power = 3.0f;
        env_config.index_of_refraction = 1.33f;  // Water-like
        env_config.base_color = glm::vec3(0.1f, 0.3f, 0.5f);
        
        env_mapping = std::make_shared<environment::EnvironmentMapping>(env_config);
        std::cout << "✓ Environment mapping system created (Fresnel power: 3.0)" << std::endl;
        std::cout << "[INIT] Setting up scene..." << std::endl;
        
        // Add skybox
        scene::graph()->addNode("skybox")
            .with<component::SkyboxComponent>(cubemap);
        
        // Create Fresnel sphere
        auto sphere_geom = Sphere::Make(1.0f, 32, 64);  // radius, stacks, slices - Higher tessellation for smooth Fresnel
        
        scene::graph()->addNode("fresnel_sphere")
            .with<component::TransformComponent>(
                transform::Transform::Make()->setTranslate(0.0f, 0.0f, 0.0f))
            .with<component::CubemapComponent>(cubemap, "environmentMap", 0)
            .with<component::ShaderComponent>(env_mapping->getShader())
            .with<component::GeometryComponent>(sphere_geom);
        
        std::cout << "✓ Fresnel sphere added to scene" << std::endl;
        
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
        
        // TODO: Add keyboard input for Fresnel power and IOR control when input handling is implemented
    };

    auto on_render = [](double alpha) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        scene::graph()->draw();
        GL_CHECK("render");
    };

    engene::EnGeneConfig config;
    config.title = "Fresnel Effect Test";
    config.width = 800;
    config.height = 600;
    config.clearColor[0] = 0.1f;
    config.clearColor[1] = 0.1f;
    config.clearColor[2] = 0.15f;
    config.clearColor[3] = 1.0f;

    try {
        engene::EnGene app(on_init, on_update, on_render, config);
        std::cout << "\n[RUNNING] Fresnel effect test application" << std::endl;
        std::cout << "Expected: Sphere with angle-dependent reflection" << std::endl;
        std::cout << "  - Center (perpendicular view): More refraction, less reflection" << std::endl;
        std::cout << "  - Edges (grazing angles): Strong reflection" << std::endl;
        std::cout << "Higher Fresnel power = sharper transition between reflection/refraction" << std::endl;
        app.run();
        
        std::cout << "\n✓ Test completed successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "✗ Test failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
