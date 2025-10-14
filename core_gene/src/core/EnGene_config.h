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
    int maxFramerate = 60;
    float clearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };

    // --- Shader Settings ---
    // Can be a file path OR the raw GLSL source code.
    // If left empty, a default shader will be used.
    std::string base_vertex_shader_source = DEFAULT_VERTEX_SHADER;
    std::string base_fragment_shader_source = DEFAULT_FRAGMENT_SHADER;

    private:
    // --- NEW: Default Shaders ---
    // Using C++ raw string literals R"(...)" for multi-line strings.
    inline static const char* DEFAULT_VERTEX_SHADER = R"(
        #version 410

        layout (location=0) in vec4 vertex;
        layout (location=1) in vec4 icolor;

        out vec4 vertexColor;

        uniform mat4 M;

        void main (void)
        {
        vertexColor = icolor;
        gl_Position = M * vertex;
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