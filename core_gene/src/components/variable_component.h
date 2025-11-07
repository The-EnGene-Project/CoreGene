#ifndef VARIABLE_COMPONENT_H
#define VARIABLE_COMPONENT_H
#pragma once

#include "component.h"
#include "../gl_base/shader_stack.h"
#include "../gl_base/shader.h"
#include "../gl_base/uniform.h"
#include <vector>
#include <memory>

namespace component {

/**
 * @brief VariableComponent
 * 
 * @example
 * ```cpp
 * using namespace uniform;
 * using namespace component;
 * 
 * auto fogColor = VariableComponent::Make(
 *     Uniform<glm::vec4>::Make("fcolor", [] { return glm::vec4(1.0f); })
 * );
 * 
 * auto fogDensity = VariableComponent::Make(
 *     Uniform<float>::Make("fdensity", [] { return 0.025f; })
 * );
 * 
 * node.with(fogColor).with(fogDensity);
 * ```
 */
class VariableComponent;
using VariableComponentPtr = std::shared_ptr<VariableComponent>;

class VariableComponent : public Component {
private:
    std::vector<uniform::UniformInterfacePtr> m_uniforms;

protected:
    explicit VariableComponent() : Component(ComponentPriority::APPEARANCE) {}

public:
    static VariableComponentPtr Make(uniform::UniformInterfacePtr uniform) {
        auto comp = std::make_shared<VariableComponent>();
        comp->addUniform(std::move(uniform));
        return comp;
    }

    virtual const char* getTypeName() const override {
        return "VariableComponent";
    }

    void addUniform(uniform::UniformInterfacePtr uniform) {
        m_uniforms.push_back(std::move(uniform));
    }

    // Remove any uniform with the given name from the collection.
    // If multiple uniforms share the same name, all will be removed.
    void removeUniform(const std::string& name) {
    for (auto it = m_uniforms.begin(); it != m_uniforms.end(); ) {
        if (*it && (*it)->getName() == name) {
            it = m_uniforms.erase(it); // erase retorna o iterador para o próximo elemento válido
        } else {
            ++it;
        }
    }
}

    virtual void apply() override {

        for (auto& u : m_uniforms) {
            u->apply();
        }
    }

    virtual void unapply() override {
    }
};

} // namespace component

#endif
