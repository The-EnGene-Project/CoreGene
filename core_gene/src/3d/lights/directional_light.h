#ifndef DIRECTIONAL_LIGHT_H
#define DIRECTIONAL_LIGHT_H
#pragma once
#include "light.h"
#include <memory>

namespace light {

class DirectionalLight;
using DirectionalLightPtr = std::shared_ptr<DirectionalLight>;

/**
 * @brief Parameters for creating a DirectionalLight.
 * 
 * Extends the base LightParams with directional light-specific properties.
 * Directional lights simulate distant light sources (like the sun) where all
 * light rays are parallel.
 */
struct DirectionalLightParams : LightParams {
    /**
     * @brief Base direction vector in local space.
     * 
     * This direction will be transformed to world space by the LightManager
     * using the light's world transform from its parent scene node.
     * Default: vec3(0, -1, 0) - pointing downward.
     */
    glm::vec3 base_direction = glm::vec3(0, -1, 0);
};

/**
 * @brief Directional light implementation.
 * 
 * Directional lights simulate distant light sources where all light rays
 * are parallel (e.g., sunlight). The light has no position, only a direction.
 * 
 * The base_direction is stored in local space and transformed to world space
 * by the LightManager when the light is processed for rendering.
 * 
 * @see LightComponent for attaching to scene nodes
 * @see LightManager for GPU synchronization
 */
class DirectionalLight : public Light {
protected:
    /**
     * @brief Base direction in local space.
     * 
     * This direction will be transformed to world space using the component's
     * world transform matrix.
     */
    glm::vec3 base_direction;

    /**
     * @brief Protected constructor for factory pattern.
     * 
     * @param params DirectionalLight parameters including base_direction.
     */
    DirectionalLight(DirectionalLightParams params)
        : Light(params),
          base_direction(params.base_direction) {}

public:
    /**
     * @brief Factory method to create a DirectionalLight.
     * 
     * @param params DirectionalLight parameters.
     * @return Shared pointer to the created DirectionalLight.
     */
    static DirectionalLightPtr Make(const DirectionalLightParams& params)
    {
        return DirectionalLightPtr(new DirectionalLight(params));
    }

    /**
     * @brief Get the light type identifier.
     * 
     * @return LightType::DIRECTIONAL
     */
    LightType getType() const override { return LightType::DIRECTIONAL; }

    /**
     * @brief Get the base direction in local space.
     * 
     * @return Const reference to the base direction vector.
     */
    const glm::vec3& getBaseDirection() const { return base_direction; }
};

/**
 * @brief Shared pointer type alias for DirectionalLight objects.
 */
using DirectionalLightPtr = std::shared_ptr<DirectionalLight>;

} // namespace light

#endif // DIRECTIONAL_LIGHT_H