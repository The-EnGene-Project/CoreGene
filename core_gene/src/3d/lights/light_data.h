#ifndef LIGHT_DATA_H
#define LIGHT_DATA_H
#pragma once

#include <glm/glm.hpp>

namespace light {

/**
 * @brief Enumeration of light types supported by the lighting system.
 * 
 * This enum is used to identify the type of light in both CPU and GPU code.
 * The underlying type is int for GPU compatibility (GLSL int type).
 * 
 * @note The numeric values are used in shaders for branching logic.
 */
enum class LightType : int {
    /**
     * @brief Inactive light slot.
     * 
     * Indicates that this light slot in the array is not currently in use.
     * Shaders should skip processing lights with this type.
     */
    INACTIVE = 0,
    
    /**
     * @brief Directional light (e.g., sun).
     * 
     * Directional lights have a direction but no position. They affect all
     * objects in the scene uniformly, simulating infinitely distant light sources.
     */
    DIRECTIONAL = 1,
    
    /**
     * @brief Point light (e.g., light bulb).
     * 
     * Point lights emit light uniformly in all directions from a single point.
     * They have position and attenuation properties.
     */
    POINT = 2,
    
    /**
     * @brief Spot light (e.g., flashlight).
     * 
     * Spot lights emit light in a cone from a position in a specific direction.
     * They have position, direction, attenuation, and cutoff angle properties.
     */
    SPOT = 3
};

/**
 * @brief GPU-compatible light data structure with std140 layout.
 * 
 * This structure represents a single light's data in a format compatible with
 * OpenGL's std140 uniform buffer layout. All members are aligned to vec4 boundaries
 * to ensure consistent memory layout between CPU and GPU.
 * 
 * @note Total size: 112 bytes (7 vec4s + 4 ints with padding)
 * 
 * Memory Layout:
 * - position:    16 bytes (vec4)
 * - direction:   16 bytes (vec4)
 * - ambient:     16 bytes (vec4)
 * - diffuse:     16 bytes (vec4)
 * - specular:    16 bytes (vec4)
 * - attenuation: 16 bytes (vec4)
 * - type:         4 bytes (int)
 * - padding:     12 bytes (3 ints)
 * Total:        112 bytes
 */
struct LightData {
    /**
     * @brief World-space position of the light.
     * 
     * For point and spot lights, this contains the light's position (w=1).
     * For directional lights, this field is unused and should be zero.
     */
    glm::vec4 position;
    
    /**
     * @brief World-space direction of the light.
     * 
     * For directional and spot lights, this contains the normalized direction vector (w=0).
     * For point lights, this field is unused and should be zero.
     */
    glm::vec4 direction;
    
    /**
     * @brief Ambient color contribution (RGB + padding).
     * 
     * The ambient component provides a base level of illumination even in shadowed areas.
     * The w component is unused padding for alignment.
     */
    glm::vec4 ambient;
    
    /**
     * @brief Diffuse color contribution (RGB + padding).
     * 
     * The diffuse component provides the main color of the light based on surface angle.
     * The w component is unused padding for alignment.
     */
    glm::vec4 diffuse;
    
    /**
     * @brief Specular color contribution (RGB + padding).
     * 
     * The specular component provides highlights based on view angle.
     * The w component is unused padding for alignment.
     */
    glm::vec4 specular;
    
    /**
     * @brief Attenuation and cutoff parameters.
     * 
     * - x (constant):  Constant attenuation factor (used by point and spot lights)
     * - y (linear):    Linear attenuation factor (used by point and spot lights)
     * - z (quadratic): Quadratic attenuation factor (used by point and spot lights)
     * - w (cutoff):    Cutoff angle cosine (used by spot lights only)
     * 
     * For directional lights, all components are unused and should be zero.
     */
    glm::vec4 attenuation;
    
    /**
     * @brief Type of light (LightType enum value).
     * 
     * This field is used for runtime type discrimination in shaders.
     * Cast from LightType enum to int for storage.
     */
    int type;
    
    /**
     * @brief Padding to align structure to vec4 boundary.
     * 
     * Required for std140 layout compatibility. Ensures the structure
     * size is a multiple of 16 bytes.
     */
    int padding[3];
};

/**
 * @brief Templated container for all scene lights with GPU-compatible layout.
 * 
 * This structure holds an array of lights and a count of active lights.
 * It is designed to be uploaded to the GPU as a Uniform Buffer Object (UBO)
 * with std140 layout.
 * 
 * @tparam MAX_LIGHTS Maximum number of lights supported in the scene.
 *                    This should match the MAX_SCENE_LIGHTS constant from light_config.h
 *                    and the corresponding constant in GLSL shaders.
 * 
 * @note Memory Layout:
 * - lights array:        MAX_LIGHTS * 112 bytes
 * - active_light_count:  4 bytes (int)
 * - padding:            12 bytes (3 ints)
 * Total:                (MAX_LIGHTS * 112) + 16 bytes
 * 
 * Example for MAX_LIGHTS=16:
 * - lights array:        1792 bytes
 * - active_light_count:     4 bytes
 * - padding:               12 bytes
 * Total:                 1808 bytes
 * 
 * @warning The MAX_LIGHTS template parameter must match the value used in shaders
 *          to ensure correct data interpretation on the GPU.
 */
template<size_t MAX_LIGHTS>
struct SceneLights {
    /**
     * @brief Array of light data for all lights in the scene.
     * 
     * Lights with indices [0, active_light_count) contain valid data.
     * Lights with indices [active_light_count, MAX_LIGHTS) should have
     * type set to LightType::INACTIVE.
     */
    LightData lights[MAX_LIGHTS];
    
    /**
     * @brief Number of active lights in the scene.
     * 
     * This value indicates how many lights in the array are currently active
     * and should be processed by shaders. Valid range: [0, MAX_LIGHTS].
     */
    int active_light_count;
    
    /**
     * @brief Padding to align structure to vec4 boundary.
     * 
     * Required for std140 layout compatibility. Ensures the structure
     * size is a multiple of 16 bytes.
     */
    int padding[3];
};

} // namespace light

#endif // LIGHT_DATA_H
