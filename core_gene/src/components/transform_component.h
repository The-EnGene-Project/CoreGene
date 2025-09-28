#ifndef TRANSFORM_COMPONENT_H
#define TRANSFORM_COMPONENT_H
#pragma once

#include <memory>
#include <stdexcept>
#include "component.h"
#include "../gl_base/transform.h"

namespace component {

class TransformComponent;
using TransformComponentPtr = std::shared_ptr<TransformComponent>;

class TransformComponent : virtual public Component {
private:
    transform::TransformPtr m_transform;

    unsigned int validatePriority(unsigned int p) {
        const unsigned int max_priority = static_cast<int>(ComponentPriority::CAMERA); 
        if (p < max_priority) throw std::invalid_argument(
            "Invalid priority for TransformComponent: " + std::to_string(p) +
            ". Priority must be between " + std::to_string(0) +
            " and " + std::to_string(max_priority) + "."
        );
        return p;
    }
protected:

    TransformComponent(transform::TransformPtr t) :
    Component(ComponentPriority::TRANSFORM),
    m_transform(t)
    {}

    TransformComponent(transform::TransformPtr t, unsigned int priority) :
    Component(validatePriority(priority)),
    m_transform(t)
    {}

public:

    TransformComponentPtr Make(transform::TransformPtr t) {
        return std::make_shared<TransformComponent>(t);
    }
    
    virtual void apply() override {
        transform::stack()->push(m_transform->getMatrix());
    }

    virtual void unapply() override {
        transform::stack()->pop();
    }

    transform::TransformPtr getTransform() {
        return m_transform;
    }

    void setTransform(transform::TransformPtr t) {
        m_transform = t;
    }

    glm::mat4 getMatrix() {
        return m_transform->getMatrix();
    }

    void setMatrix(glm::mat4 m) {
        m_transform->setMatrix(m);
    }
};


}

#endif