#pragma once
#include <glm/glm.hpp>
#include <memory>
#include "../lights/light_data.h"

namespace light {

// Forward declaration
class Light;

/**
 * @brief Shared pointer type alias for Light objects.
 * 
 * This alias follows the codebase convention of using shared pointers
 * for managing light object lifetimes.
 */
using LightPtr = std::shared_ptr<Light>;

/**
 * @brief Base parameters for all light types.
 * 
 * This structure contains the common color properties shared by all light types.
 * Specific light types (DirectionalLight, PointLight, SpotLight) extend this
 * structure with their own parameters.
 */
struct LightParams
{
    /**
     * @brief Ambient color contribution (RGB + alpha).
     * 
     * The ambient component provides a base level of illumination even in shadowed areas.
     * Default: vec4(0.3f) - 30% ambient light in all channels.
     */
    glm::vec4 ambient = glm::vec4(0.3f);
    
    /**
     * @brief Diffuse color contribution (RGB + alpha).
     * 
     * The diffuse component provides the main color of the light based on surface angle.
     * Default: vec4(0.7f) - 70% diffuse light in all channels.
     */
    glm::vec4 diffuse = glm::vec4(0.7f);
    
    /**
     * @brief Specular color contribution (RGB + alpha).
     * 
     * The specular component provides highlights based on view angle.
     * Default: vec4(1.0f) - 100% specular light in all channels (white highlights).
     */
    glm::vec4 specular = glm::vec4(1.0f);
};

/**
 * @brief Abstract base class for all light types in the lighting system.
 * 
 * This class provides the common interface and properties for all light types.
 * It has been refactored to work with the new component-based architecture where
 * lights are attached to scene nodes via LightComponent, and the LightManager
 * handles synchronization with the GPU.
 * 
 * The old shader attachment methods have been removed. Lights are now managed
 * centrally by the LightManager, which collects all active lights and uploads
 * them to the GPU as a single uniform buffer.
 * 
 * @note This class should not be instantiated directly. Use the derived classes:
 *       DirectionalLight, PointLight, or SpotLight.
 * 
 * @see LightComponent for scene graph integration
 * @see LightManager for GPU synchronization
 */
class Light
{
protected:
    /**
     * @brief Ambient color contribution.
     * 
     * Provides base illumination even in shadowed areas.
     */
    glm::vec4 ambient;
    
    /**
     * @brief Diffuse color contribution.
     * 
     * Provides the main color based on surface angle to light.
     */
    glm::vec4 diffuse;
    
    /**
     * @brief Specular color contribution.
     * 
     * Provides highlights based on view angle.
     */
    glm::vec4 specular;

    /**
     * @brief Protected constructor for derived classes.
     * 
     * @param params Base light parameters containing color properties.
     */
    Light(LightParams params)
        : ambient(params.ambient),
          diffuse(params.diffuse),
          specular(params.specular) {}

public:
    /**
     * @brief Virtual destructor for proper cleanup of derived classes.
     */
    virtual ~Light() = default;

    /**
     * @brief Get the type of this light.
     * 
     * This pure virtual method must be implemented by derived classes to return
     * their specific light type. The type is used by the LightManager for runtime
     * type discrimination when packing light data for the GPU.
     * 
     * @return LightType enum value identifying the specific light type.
     * 
     * @see LightType
     * @see LightManager::apply()
     */
    virtual LightType getType() const = 0;

    /**
     * @brief Get the ambient color contribution.
     * 
     * @return Const reference to the ambient color vector.
     */
    const glm::vec4& getAmbient() const { return ambient; }
    
    /**
     * @brief Get the diffuse color contribution.
     * 
     * @return Const reference to the diffuse color vector.
     */
    const glm::vec4& getDiffuse() const { return diffuse; }
    
    /**
     * @brief Get the specular color contribution.
     * 
     * @return Const reference to the specular color vector.
     */
    const glm::vec4& getSpecular() const { return specular; }
};

} // namespace light