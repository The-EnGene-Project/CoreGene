#pragma once

#include <string>

namespace engene {

/**
 * @struct EnGeneConfig
 * @brief Configuration object for the EnGene engine.
 * @brief A simple data structure holding all startup parameters for the engine.
 */
struct EnGeneConfig {
    // --- Shader Settings ---
    std::string base_vertex_shader_path;    // required
    std::string base_fragment_shader_path;  // required

    // --- Window Settings ---
    std::string title = "EnGene Window";
    int width = 800;
    int height = 800;

    // --- Engine Settings ---
    int maxFramerate = 60;
    float clearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
};

} // namespace engene