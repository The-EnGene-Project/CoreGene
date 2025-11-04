#ifndef MATERIAL_COMPONENT_H
#define MATERIAL_COMPONENT_H
#pragma once

#include "component.h"
#include "../gl_base/material.h"
#include <memory>

namespace component {

class MaterialComponent;
using MaterialComponentPtr = std::shared_ptr<MaterialComponent>;

/**
 * @class MaterialComponent
 * @brief A component that manages material property state in the scene graph.
 *
 * This component wraps a material::Material shared pointer. When applied, it pushes
 * the material onto the global material::stack(), making its properties active for
 * rendering. When unapplied, it pops the stack, restoring the previous material state.
 * This is designed to work within a scene graph traversal system and integrates
 * seamlessly with the shader dynamic uniform system.
 */
class MaterialComponent : public Component {
private:
    material::MaterialPtr m_material;

protected:
    /**
     * @brief Constructs a MaterialComponent.
     * @param material The material resource to manage.
     */
    explicit MaterialComponent(material::MaterialPtr material)
        : Component(ComponentPriority::APPEARANCE), // Materials are part of appearance (priority 300)
          m_material(material)
    {
    }

public:
    // Delete copy constructor and assignment operator to prevent accidental copying.
    MaterialComponent(const MaterialComponent&) = delete;
    MaterialComponent& operator=(const MaterialComponent&) = delete;

    /**
     * @brief Factory function to create a new MaterialComponent.
     * @param material The material resource to manage.
     * @return A shared pointer to the newly created MaterialComponent.
     */
    static MaterialComponentPtr Make(material::MaterialPtr material) {
        return MaterialComponentPtr(new MaterialComponent(material));
    }

    /**
     * @brief Factory function to create a new MaterialComponent with a name.
     * @param material The material resource to manage.
     * @param name The name for this component instance.
     * @return A shared pointer to the newly created MaterialComponent.
     */
    static MaterialComponentPtr Make(material::MaterialPtr material, const std::string& name) {
        auto comp = MaterialComponentPtr(new MaterialComponent(material));
        comp->setName(name);
        return comp;
    }

    /**
     * @brief Pushes the component's material onto the material stack, making it active.
     * This method is intended to be called during a "pre-order" scene graph traversal.
     */
    void apply() override {
        if (m_material) {
            material::stack()->push(m_material);
        }
    }

    /**
     * @brief Pops from the material stack, restoring the previous material state.
     * This method is intended to be called during a "post-order" scene graph traversal.
     */
    void unapply() override {
        if (m_material) {
            material::stack()->pop();
        }
    }

    /**
     * @brief Returns the type name of this component.
     * @return "MaterialComponent"
     */
    virtual const char* getTypeName() const override {
        return "MaterialComponent";
    }
};

} // namespace component

#endif // MATERIAL_COMPONENT_H
