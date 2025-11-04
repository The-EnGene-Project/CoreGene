#ifndef VARIABLE_COMPONENT_T_H
#define VARIABLE_COMPONENT_T_H
#pragma once

#include "component.h"
#include "../gl_base/shader_stack.h"
#include "../gl_base/shader.h"
#include <string>
#include <memory>

namespace component {

/**
 * @brief VariableComponent
 * 
 * Um componente de variável uniforme tipado em tempo de compilação.
 * 
 * Exemplo:
 * ```cpp
 * auto fogDensity = component::VariableComponent<float>::Make("fdensity", 0.03f);
 * auto fogColor   = component::VariableComponent<glm::vec4>::Make("fcolor", glm::vec4(1.0f));
 * node.with(fogDensity).with(fogColor);
 * ```
 */
template<typename T>
class VariableComponent : public Component {
private:
    std::string m_uniformName;
    T m_value;

protected:
    VariableComponent(const std::string& name, const T& value)
        : Component(ComponentPriority::APPEARANCE), m_uniformName(name), m_value(value) {}

public:
    using Ptr = std::shared_ptr<VariableComponent<T>>;

    static Ptr Make(const std::string& name, const T& value) {
        return Ptr(new VariableComponent<T>(name, value));
    }

    virtual const char* getTypeName() const override {
        return "VariableComponent";
    }

    void setValue(const T& value) { m_value = value; }

    virtual void apply() override {
        auto shader = shader::stack()->peek();
        if (shader) shader->setUniform(m_uniformName, m_value);
    }

    virtual void unapply() override {}
};

} // namespace component

#endif
