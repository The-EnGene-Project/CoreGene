#pragma once
#include "point_light.h"

namespace light {

// Forward declaration
class SpotLight;

/**
 * @brief Shared pointer type alias for SpotLight objects.
 * 
 * This alias follows the codebase convention of using shared pointers
 * for managing spot light object lifetimes.
 */
using SpotLightPtr = std::shared_ptr<SpotLight>;

/**
 * @brief Parameters for creating a SpotLight.
 * 
 * This structure extends PointLightParams with spot light-specific properties:
 * base direction and cutoff angle.
 */
struct SpotLightParams : PointLightParams
{
    /**
     * @brief Direction of the spotlight cone in local space.
     * 
     * This direction will be transformed to world space by the LightComponent
     * using the component's world transform matrix.
     * Default: vec3(0, -1, 0) - pointing downward.
     */
    glm::vec3 base_direction = glm::vec3(0, -1, 0);
    
    /**
     * @brief Cutoff angle (as cosine) for the spotlight cone.
     * 
     * This value represents the cosine of the angle from the spotlight's
     * direction vector to the edge of the cone. Fragments outside this
     * angle receive no light.
     * Default: cos(12.5°) - narrow spotlight beam.
     */
    float cutOff = glm::cos(glm::radians(12.5f));
};

/**
 * @brief Spot light that emits light in a cone from a position in a specific direction.
 * 
 * Spot lights simulate light sources like flashlights or stage spotlights. They combine
 * the position and attenuation properties of point lights with a direction and cutoff angle
 * to create a cone of light.
 * 
 * The light's position and direction are stored in local space and will be transformed to
 * world space by the LightManager using the LightComponent's world transform.
 * 
 * @note This class should be created using the Make() factory method.
 * @note Spot lights must be wrapped in a LightComponent to be added to the scene.
 * 
 * @see LightComponent for scene graph integration
 * @see LightManager for GPU synchronization
 */
class SpotLight : public PointLight
{
protected:
    /**
     * @brief Direction of the spotlight cone in local space.
     * 
     * This will be transformed to world space by the LightManager.
     */
    glm::vec3 base_direction;
    
    /**
     * @brief Cutoff angle (as cosine) for the spotlight cone.
     */
    float cutOffAngle;
    
    /**
     * @brief Protected constructor for factory pattern.
     * 
     * @param params Spot light parameters including position, direction, attenuation, and cutoff angle.
     */
    SpotLight(SpotLightParams params)
        : PointLight(params),
          base_direction(params.base_direction),
          cutOffAngle(params.cutOff) {}

public:
    /**
     * @brief Get the type of this light.
     * 
     * @return LightType::SPOT
     */
    LightType getType() const override { return LightType::SPOT; }
    
    /**
     * @brief Get the base direction of the spotlight in local space.
     * 
     * @return Const reference to the base direction vector.
     */
    const glm::vec3& getBaseDirection() const { return base_direction; }
    
    /**
     * @brief Get the cutoff angle (as cosine) for the spotlight cone.
     * 
     * @return Cutoff angle cosine value.
     */
    float getCutoffAngle() const { return cutOffAngle; }
    
    /**
     * @brief Factory method to create a SpotLight.
     * 
     * @param params Spot light parameters including position, direction, colors, attenuation, and cutoff angle.
     * @return Shared pointer to the created SpotLight.
     */
    static SpotLightPtr Make(const SpotLightParams &params)
    {
        return SpotLightPtr(new SpotLight(params));
    }
};

} // namespace light
