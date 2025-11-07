#ifndef ENGENE_CONFIG_H
#define ENGENE_CONFIG_H
#pragma once

#include <string>

namespace engene {

/**
 * @struct EnGeneConfig
 * @brief Configuration object for the EnGene engine.
 * @brief A simple data structure holding all startup parameters for the engine.
 */
struct EnGeneConfig {
    // --- Window Settings ---
    std::string title = "EnGene Window";
    int width = 800;
    int height = 800;

    // --- Engine Settings ---
    int updatesPerSecond = 60;  // Amount of times the simulation function will be called per second. 
    double maxFrameTime = 0.25; // Max time slice to prevent spiral of death on major lag.
    float clearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };

    // --- Shader Settings ---
    // Can be a file path OR the raw GLSL source code.
    // If left empty, a default shader will be used.
    std::string base_vertex_shader_source = DEFAULT_VERTEX_SHADER;
    std::string base_fragment_shader_source = DEFAULT_FRAGMENT_SHADER;

    private:
    // --- Default Shaders ---
    // Using C++ raw string literals R"(...)" for multi-line strings.
    inline static const char* DEFAULT_VERTEX_SHADER = R"(
        #version 410 core
        layout (location = 0) in vec4 vertex;
        layout (location = 1) in vec4 icolor;

        out vec4 vertexColor;

        // Tier 1: Global Camera UBO
        layout (std140) uniform CameraMatrices {
            mat4 view;
            mat4 projection;
        };

        // Tier 3: Dynamic Model Matrix
        uniform mat4 u_model;

        void main() {
            vertexColor = icolor;
            // Note the new multiplication order for matrices
            gl_Position = projection * view * u_model * vertex;
        }
    )";

    inline static const char* DEFAULT_FRAGMENT_SHADER = R"(
        #version 410

        in vec4 vertexColor;
        out vec4 fragColor;

        void main() {
            fragColor = vertexColor;
        }
    )";

    friend class EnGene;

};

} // namespace engene

#endif // ENGENE_CONFIG_H