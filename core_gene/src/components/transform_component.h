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
        if (p > max_priority || p < 0) throw std::invalid_argument(
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

    static TransformComponentPtr Make(transform::TransformPtr t) {
        return TransformComponentPtr(new TransformComponent(t));
    }

    static TransformComponentPtr Make(transform::TransformPtr t, unsigned int priority) {
        return TransformComponentPtr(new TransformComponent(t, priority));
    }
    
    virtual void apply() override {
        transform::stack()->push(m_transform->getMatrix());
    }

    virtual void unapply() override {
        transform::stack()->pop();
    }

    virtual const char* getTypeName() const override {
        return "TransformComponent";
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