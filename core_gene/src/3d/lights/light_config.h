#ifndef LIGHT_CONFIG_H
#define LIGHT_CONFIG_H
#pragma once

// Allow users to override the maximum number of lights by defining this before including EnGene
#ifndef MAX_SCENE_LIGHTS
    #define MAX_SCENE_LIGHTS 16
#endif

namespace light {
    /**
     * @brief Maximum number of lights supported by the scene.
     * 
     * This constant determines the size of the GPU light array and must be
     * synchronized with the corresponding GLSL shader constant.
     * 
     * Users can override this value by defining MAX_SCENE_LIGHTS before including
     * EnGene headers:
     * 
     * @code
     * #define MAX_SCENE_LIGHTS 32
     * #include <EnGene.h>
     * @endcode
     * 
     * @note Increasing this value will increase GPU memory usage proportionally.
     * @note This value must match MAX_SCENE_LIGHTS in your shader code.
     * @note Default value is 16 if not defined by the user.
     */
    constexpr size_t max_scene_lights = MAX_SCENE_LIGHTS;
}

#endif // LIGHT_CONFIG_H
