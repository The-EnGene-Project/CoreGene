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
 * Exemplo de uso:
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

    virtual void apply() override {
        auto shader = shader::stack()->peek();
        if (!shader) return;

        GLuint programID = shader->GetShaderID();
        for (auto& u : m_uniforms) {
            u->findLocation(programID);
            if (u->isValid())
                u->apply();
        }
    }

    virtual void unapply() override {
    }
};

} // namespace component

#endif
