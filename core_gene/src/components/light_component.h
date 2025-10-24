#ifndef LIGHT_COMPONENT_H
#define LIGHT_COMPONENT_H
#pragma once

#include "observed_transform_component.h"
#include "../3d/lights/light_manager.h"
#include "../3d/lights/light.h"
#include "../gl_base/transform.h"
#include <memory>
#include <stdexcept>

// Forward declaration of light manager
namespace light {
    template<size_t MAX_LIGHTS> class LightManagerImpl;
    using LightManager = LightManagerImpl<MAX_SCENE_LIGHTS>;
    LightManager& manager();
}

namespace component {

class LightComponent;
using LightComponentPtr = std::shared_ptr<LightComponent>;

/**
 * @class LightComponent
 * @brief A component that integrates lights into the scene graph.
 * 
 * LightComponent wraps a light::Light object and attaches it to a scene node,
 * allowing the light to inherit transformations from its parent nodes. The component
 * automatically registers itself with the LightManager on construction and unregisters
 * on destruction.
 * 
 * The component inherits from ObservedTransformComponent to track world-space transforms,
 * which are used by the LightManager to transform light directions and positions into
 * world space before uploading to the GPU.
 * 
 * @note This component uses CUSTOM_SCRIPT priority to avoid conflicts with other components.
 * 
 * @see light::Light
 * @see light::LightManager
 * @see ObservedTransformComponent
 */
class LightComponent : public ObservedTransformComponent, public std::enable_shared_from_this<LightComponent> {
private:
    light::LightPtr m_light;

    /**
     * @brief Private constructor to enforce factory pattern.
     * 
     * @param light The light object to attach to the scene graph.
     * @param transform The transform defining the light's local transformation.
     * @throws std::invalid_argument if light pointer is null.
     */
    LightComponent(light::LightPtr light, transform::TransformPtr transform)
        : ObservedTransformComponent(
            transform,
            static_cast<unsigned int>(ComponentPriority::CUSTOM_SCRIPT),
            static_cast<unsigned int>(ComponentPriority::CUSTOM_SCRIPT),
            static_cast<unsigned int>(ComponentPriority::CUSTOM_SCRIPT)
        ),
          m_light(light)
    {
        if (!light) {
            throw std::invalid_argument("LightComponent requires a valid light pointer");
        }
        // Register with the light manager
        light::manager().registerLight(this->shared_from_this());
    }

public:
    /**
     * @brief Destructor that automatically unregisters from the LightManager.
     */
    ~LightComponent() {
        // Unregister from the light manager
        light::manager().unregisterLight(this->shared_from_this());
    }

    /**
     * @brief Factory method to create a LightComponent.
     * 
     * Creates a new LightComponent that wraps the given light and transform.
     * The component automatically registers itself with the LightManager upon
     * construction.
     * 
     * @param light The light object to attach to the scene graph.
     * @param transform The transform defining the light's local transformation.
     * @return Shared pointer to the created LightComponent.
     * @throws std::invalid_argument if light pointer is null.
     * 
     * @example
     * @code
     * auto light = light::DirectionalLight::Make({...});
     * auto transform = transform::Transform::Make();
     * auto lightComp = component::LightComponent::Make(light, transform);
     * myNode->payload().add(lightComp);
     * @endcode
     */
    static LightComponentPtr Make(light::LightPtr light, transform::TransformPtr transform) {
        return LightComponentPtr(new LightComponent(light, transform));
    }

    /**
     * @brief Factory method to create a named LightComponent.
     * 
     * Creates a new LightComponent with a specific name. This is useful for
     * debugging and for retrieving specific components from a node's payload.
     * The component automatically registers itself with the LightManager upon
     * construction.
     * 
     * @param light The light object to attach to the scene graph.
     * @param transform The transform defining the light's local transformation.
     * @param name The name to assign to this component instance.
     * @return Shared pointer to the created LightComponent.
     * @throws std::invalid_argument if light pointer is null.
     * 
     * @example
     * @code
     * auto light = light::PointLight::Make({...});
     * auto transform = transform::Transform::Make();
     * auto lightComp = component::LightComponent::Make(light, transform, "MainLight");
     * myNode->payload().add(lightComp);
     * @endcode
     */
    static LightComponentPtr Make(light::LightPtr light, transform::TransformPtr transform, 
                                   const std::string& name) {
        auto comp = LightComponentPtr(new LightComponent(light, transform));
        comp->setName(name);
        return comp;
    }

    /**
     * @brief Updates the component's transform cache during scene traversal.
     * 
     * This method is called during scene traversal and updates the world transform
     * cache by calling the parent class implementation.
     */
    void apply() override {
        // Update transform cache by calling parent implementation
        ObservedTransformComponent::apply();
        // Light data extraction happens in LightManager::apply()
    }

    /**
     * @brief Get the type name of this component.
     * 
     * @return "LightComponent"
     */
    const char* getTypeName() const override {
        return "LightComponent";
    }

    /**
     * @brief Get the static type name of this component class.
     * 
     * @return "LightComponent"
     */
    static const char* getTypeNameStatic() {
        return "LightComponent";
    }

    /**
     * @brief Get the light object attached to this component.
     * 
     * @return Shared pointer to the light object.
     */
    light::LightPtr getLight() const {
        return m_light;
    }
};

} // namespace component

#endif // LIGHT_COMPONENT_H
