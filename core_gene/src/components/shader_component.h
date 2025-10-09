#ifndef SHADER_COMPONENT_H
#define SHADER_COMPONENT_H
#pragma once

#include <memory>
#include "component.h"
#include "../gl_base/shader.h"

namespace component {

class ShaderComponent;
using ShaderComponentPtr = std::shared_ptr<ShaderComponent>;

class ShaderComponent : virtual public Component {
private:
    shader::ShaderPtr m_shader;
protected:

    ShaderComponent(shader::ShaderPtr s) :
        Component(ComponentPriority::SHADER),
        m_shader(s)
        {}

public:

    ShaderComponentPtr Make(shader::ShaderPtr s) {
        return std::make_shared<ShaderComponent>(s);
    }
    
    virtual void apply() override {
        shader::stack()->push(m_shader);
    }

    virtual void unapply() override {
        shader::stack()->pop();
    }

    shader::ShaderPtr getShader() {
        return m_shader;
    }

    void setShader(shader::ShaderPtr s) {
        m_shader = s;
    }
};


}

#endif