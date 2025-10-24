#ifndef LIGHT_CONFIG_H
#define LIGHT_CONFIG_H
#pragma once

namespace light {
    /**
     * @brief Maximum number of lights supported by the scene.
     * 
     * This constant determines the size of the GPU light array and must be
     * synchronized with the corresponding GLSL shader constant. Modify this
     * value to balance performance and feature requirements for your specific
     * application.
     * 
     * @note Increasing this value will increase GPU memory usage proportionally.
     * @note This value must match MAX_SCENE_LIGHTS in your shader code.
     */
    constexpr size_t MAX_SCENE_LIGHTS = 16;
}

#endif // LIGHT_CONFIG_H
