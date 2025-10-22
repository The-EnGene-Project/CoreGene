#ifndef LIGHT_MANAGER_H
#define LIGHT_MANAGER_H
#pragma once

#include "light_config.h"
#include "light_data.h"
#include "../components/light_component.h"
#include "../3d/directional_light.h"
#include "../3d/point_light.h"
#include "../3d/spot_light.h"
#include "../gl_base/uniforms/ubo.h"
#include "../gl_base/uniforms/global_resource_manager.h"
#include <vector>
#include <algorithm>
#include <iostream>
#include <glm/glm.hpp>

namespace light {

/**
 * @brief Templated implementation of the light manager.
 * 
 * This class manages all lights in the scene, collecting light data from registered
 * LightComponent instances and synchronizing it to the GPU via a Uniform Buffer Object.
 * 
 * The manager uses a singleton pattern to ensure there is only one instance managing
 * all scene lights. It automatically handles registration/unregistration of light
 * components and provides an apply() method to update GPU data.
 * 
 * @tparam MAX_LIGHTS Maximum number of lights supported in the scene.
 *                    This should match MAX_SCENE_LIGHTS from light_config.h.
 * 
 * @note This is a header-only implementation to support template instantiation.
 * @note Access the singleton instance via the light::manager() function.
 * 
 * @see LightComponent for scene graph integration
 * @see SceneLights for the GPU data structure
 */
template<size_t MAX_LIGHTS>
class LightManagerImpl {
private:
    /**
     * @brief Vector of registered light components.
     * 
     * This vector tracks all LightComponent instances that have been registered
     * with the manager. Components are automatically added on construction and
     * removed on destruction.
     */
    std::vector<component::LightComponent*> m_registered_lights;
    
    /**
     * @brief Uniform Buffer Object for GPU light data.
     * 
     * This UBO holds the SceneLights structure and is bound to binding point 0.
     * It uses ON_DEMAND update mode, meaning it only uploads data when apply()
     * is explicitly called.
     */
    uniform::UBOPtr<SceneLights<MAX_LIGHTS>> m_light_resource;
    
    /**
     * @brief CPU-side buffer for light data.
     * 
     * This structure mirrors the GPU UBO layout and is used to pack light data
     * before uploading to the GPU. It is updated in the apply() method.
     */
    SceneLights<MAX_LIGHTS> m_scene_data;
    
    /**
     * @brief Private constructor to enforce singleton pattern.
     * 
     * Initializes all lights as INACTIVE, creates the UBO, and sets up the
     * data provider lambda for GPU uploads.
     */
    LightManagerImpl() {
        // Initialize all lights as inactive
        for (size_t i = 0; i < MAX_LIGHTS; ++i) {
            m_scene_data.lights[i].type = static_cast<int>(LightType::INACTIVE);
        }
        m_scene_data.active_light_count = 0;
        
        // Create UBO with ON_DEMAND update mode
        m_light_resource = uniform::UBO<SceneLights<MAX_LIGHTS>>::Make(
            "SceneLights",
            uniform::UpdateMode::ON_DEMAND,
            0  // Binding point 0
        );
        
        // Set the data provider lambda
        m_light_resource->setProvider([this]() { return m_scene_data; });
    }
    
    // Friend declaration for singleton accessor
    friend LightManagerImpl& manager();

public:
    // Delete copy and move constructors/operators to enforce singleton
    LightManagerImpl(const LightManagerImpl&) = delete;
    LightManagerImpl& operator=(const LightManagerImpl&) = delete;
    LightManagerImpl(LightManagerImpl&&) = delete;
    LightManagerImpl& operator=(LightManagerImpl&&) = delete;
    
    /**
     * @brief Registers a light component with the manager.
     * 
     * This method is called automatically by LightComponent's constructor.
     * It adds the component to the tracking vector if it's not already registered.
     * 
     * @param component Pointer to the LightComponent to register.
     *                  If null, the method returns early without doing anything.
     * 
     * @note Duplicate registrations are automatically prevented.
     */
    void registerLight(component::LightComponent* component) {
        if (!component) return;
        
        // Avoid duplicate registrations
        auto it = std::find(m_registered_lights.begin(), m_registered_lights.end(), component);
        if (it == m_registered_lights.end()) {
            m_registered_lights.push_back(component);
        }
    }
    
    /**
     * @brief Unregisters a light component from the manager.
     * 
     * This method is called automatically by LightComponent's destructor.
     * It removes the component from the tracking vector if it exists.
     * 
     * @param component Pointer to the LightComponent to unregister.
     */
    void unregisterLight(component::LightComponent* component) {
        auto it = std::find(m_registered_lights.begin(), m_registered_lights.end(), component);
        if (it != m_registered_lights.end()) {
            m_registered_lights.erase(it);
        }
    }
    
    /**
     * @brief Updates light data and synchronizes it to the GPU.
     * 
     * This method should be called once per frame before rendering. It:
     * 1. Resets all lights to INACTIVE
     * 2. Iterates through registered components
     * 3. Extracts light data and transforms it to world space
     * 4. Packs data into the CPU buffer
     * 5. Triggers GPU upload via the uniform manager
     * 
     * The method handles all three light types (directional, point, spot) and
     * logs a warning if the scene contains more lights than MAX_LIGHTS.
     * 
     * @note This method uses dynamic_cast to determine light types at runtime.
     * @note Lights beyond MAX_LIGHTS are ignored with a warning message.
     */
    void apply() {
        // Reset active count
        m_scene_data.active_light_count = 0;
        
        // Mark all lights as inactive initially
        for (size_t i = 0; i < MAX_LIGHTS; ++i) {
            m_scene_data.lights[i].type = static_cast<int>(LightType::INACTIVE);
        }
        
        // Process each registered component
        size_t light_index = 0;
        for (auto* component : m_registered_lights) {
            if (light_index >= MAX_LIGHTS) {
                std::cerr << "Warning: Scene has more lights than MAX_SCENE_LIGHTS (" 
                          << MAX_LIGHTS << "). Extra lights will be ignored." << std::endl;
                break;
            }
            
            light::LightPtr light = component->getLight();
            if (!light) continue;
            
            LightData& data = m_scene_data.lights[light_index];
            glm::mat4 world_transform = component->getWorldTransform();
            
            // Pack common properties
            data.ambient = light->getAmbient();
            data.diffuse = light->getDiffuse();
            data.specular = light->getSpecular();
            data.type = static_cast<int>(light->getType());
            
            // Type-specific packing
            if (auto* dir_light = dynamic_cast<DirectionalLight*>(light.get())) {
                // Transform direction to world space (w=0 for directions)
                glm::vec4 world_dir = world_transform * glm::vec4(dir_light->getBaseDirection(), 0.0f);
                data.direction = glm::normalize(world_dir);
                data.position = glm::vec4(0.0f);  // Unused for directional
                data.attenuation = glm::vec4(0.0f);  // Unused for directional
            }
            else if (auto* spot_light = dynamic_cast<SpotLight*>(light.get())) {
                // Transform position to world space
                data.position = world_transform * spot_light->getPosition();
                // Transform direction to world space and normalize
                glm::vec4 world_dir = world_transform * glm::vec4(spot_light->getBaseDirection(), 0.0f);
                data.direction = glm::normalize(world_dir);
                // Pack constant, linear, quadratic, cutoff_angle
                data.attenuation = glm::vec4(
                    spot_light->getConstant(),
                    spot_light->getLinear(),
                    spot_light->getQuadratic(),
                    spot_light->getCutoffAngle()
                );
            }
            else if (auto* point_light = dynamic_cast<PointLight*>(light.get())) {
                // Transform position to world space (w=1 for positions)
                data.position = world_transform * point_light->getPosition();
                // Pack constant, linear, quadratic into attenuation vec4
                data.attenuation = glm::vec4(
                    point_light->getConstant(),
                    point_light->getLinear(),
                    point_light->getQuadratic(),
                    0.0f  // cutoff unused for point lights
                );
                data.direction = glm::vec4(0.0f);  // Unused for point lights
            }
            
            light_index++;
        }
        
        m_scene_data.active_light_count = static_cast<int>(light_index);
        
        // Trigger GPU upload
        uniform::manager().applyShaderResource("SceneLights");
    }
};

/**
 * @brief Type alias for the light manager with the configured maximum light count.
 * 
 * This alias uses the MAX_SCENE_LIGHTS constant from light_config.h to instantiate
 * the template with the correct size.
 */
using LightManager = LightManagerImpl<MAX_SCENE_LIGHTS>;

/**
 * @brief Provides access to the singleton light manager instance.
 * 
 * This function returns a reference to the singleton LightManager instance.
 * The instance is created on first access using C++11 static initialization,
 * which is thread-safe.
 * 
 * @return Reference to the singleton LightManager instance.
 * 
 * @example
 * @code
 * // Register a light (done automatically by LightComponent)
 * light::manager().registerLight(myLightComponent);
 * 
 * // Update all lights and upload to GPU (call once per frame)
 * light::manager().apply();
 * @endcode
 */
inline LightManager& manager() {
    static LightManager instance;
    return instance;
}

} // namespace light

#endif // LIGHT_MANAGER_H
