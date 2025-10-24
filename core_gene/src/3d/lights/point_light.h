#pragma once
#include "light.h"

namespace light {

// Forward declaration
class PointLight;

/**
 * @brief Shared pointer type alias for PointLight objects.
 * 
 * This alias follows the codebase convention of using shared pointers
 * for managing point light object lifetimes.
 */
using PointLightPtr = std::shared_ptr<PointLight>;

/**
 * @brief Parameters for creating a PointLight.
 * 
 * This structure extends LightParams with point light-specific properties:
 * position and attenuation coefficients.
 */
struct PointLightParams : LightParams
{
    /**
     * @brief Position of the point light in local space.
     * 
     * This position will be transformed to world space by the LightComponent
     * using the component's world transform matrix.
     * Default: vec4(0, 0, 0, 1) - origin with w=1 for position.
     */
    glm::vec4 position = glm::vec4(0, 0, 0, 1);
    
    /**
     * @brief Constant attenuation factor.
     * 
     * The constant term in the attenuation formula: 1.0 / (constant + linear*d + quadratic*d²)
     * Default: 1.0f - no constant attenuation.
     */
    float constant = 1.0f;
    
    /**
     * @brief Linear attenuation factor.
     * 
     * The linear term in the attenuation formula: 1.0 / (constant + linear*d + quadratic*d²)
     * Default: 0.09f - moderate linear falloff.
     */
    float linear = 0.09f;
    
    /**
     * @brief Quadratic attenuation factor.
     * 
     * The quadratic term in the attenuation formula: 1.0 / (constant + linear*d + quadratic*d²)
     * Default: 0.032f - moderate quadratic falloff.
     */
    float quadratic = 0.032f;
};

/**
 * @brief Point light that emits light uniformly in all directions from a position.
 * 
 * Point lights simulate light sources like light bulbs or candles. They have a position
 * and attenuation properties that control how the light intensity falls off with distance.
 * 
 * The light's position is stored in local space and will be transformed to world space
 * by the LightManager using the LightComponent's world transform.
 * 
 * Attenuation is calculated using the formula:
 *   attenuation = 1.0 / (constant + linear * distance + quadratic * distance²)
 * 
 * @note This class should be created using the Make() factory method.
 * @note Point lights must be wrapped in a LightComponent to be added to the scene.
 * 
 * @see LightComponent for scene graph integration
 * @see LightManager for GPU synchronization
 */
class PointLight : public Light
{
protected:
    /**
     * @brief Position of the light in local space.
     * 
     * This will be transformed to world space by the LightManager.
     */
    glm::vec4 position;
    
    /**
     * @brief Constant attenuation factor.
     */
    float constant;
    
    /**
     * @brief Linear attenuation factor.
     */
    float linear;
    
    /**
     * @brief Quadratic attenuation factor.
     */
    float quadratic;
    
    /**
     * @brief Protected constructor for factory pattern.
     * 
     * @param params Point light parameters including position and attenuation.
     */
    PointLight(PointLightParams params)
        : Light(params),
          position(params.position),
          constant(params.constant),
          linear(params.linear),
          quadratic(params.quadratic) {}

public:
    /**
     * @brief Get the type of this light.
     * 
     * @return LightType::POINT
     */
    LightType getType() const override { return LightType::POINT; }
    
    /**
     * @brief Get the position of the light in local space.
     * 
     * @return Const reference to the position vector.
     */
    const glm::vec4& getPosition() const { return position; }
    
    /**
     * @brief Get the constant attenuation factor.
     * 
     * @return Constant attenuation coefficient.
     */
    float getConstant() const { return constant; }
    
    /**
     * @brief Get the linear attenuation factor.
     * 
     * @return Linear attenuation coefficient.
     */
    float getLinear() const { return linear; }
    
    /**
     * @brief Get the quadratic attenuation factor.
     * 
     * @return Quadratic attenuation coefficient.
     */
    float getQuadratic() const { return quadratic; }
    
    /**
     * @brief Factory method to create a PointLight.
     * 
     * @param params Point light parameters including position, colors, and attenuation.
     * @return Shared pointer to the created PointLight.
     */
    static PointLightPtr Make(const PointLightParams &params)
    {
        return PointLightPtr(new PointLight(params));
    }
};

} // namespace light
