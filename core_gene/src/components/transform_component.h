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
    
protected:
    // The validation function now takes the bounds as arguments
    unsigned int validatePriority(unsigned int p, unsigned int min_bound, unsigned int max_bound) const {
        if (p < min_bound || p > max_bound) {
            throw std::invalid_argument(
                "Priority " + std::to_string(p) + " is outside the valid bounds [" +
                std::to_string(min_bound) + ", " + std::to_string(max_bound) + "]."
            );
        }
        return p;
    }

    // The constructor now accepts the validation bounds
    TransformComponent(transform::TransformPtr t, unsigned int priority, unsigned int min_bound, unsigned int max_bound) :
        Component(validatePriority(priority, min_bound, max_bound)), // Use the new validation
        m_transform(t)
    {}

    // A default constructor for standard transforms
    TransformComponent(transform::TransformPtr t) :
        TransformComponent(
            t,
            static_cast<unsigned int>(ComponentPriority::TRANSFORM), // Default priority
            0,                                                      // Default min bound
            static_cast<unsigned int>(ComponentPriority::CAMERA)    // Default max bound
        )
    {}

public:

    static TransformComponentPtr Make(transform::TransformPtr t) {
        return TransformComponentPtr(new TransformComponent(t));
    }

    static TransformComponentPtr Make(transform::TransformPtr t, unsigned int priority) {
        return TransformComponentPtr(new TransformComponent(t, priority));
    }

    static TransformComponentPtr Make(transform::TransformPtr t, const std::string& name) {
        auto comp = TransformComponentPtr(new TransformComponent(t));
        comp->setName(name);
        return comp;
    }

    static TransformComponentPtr Make(transform::TransformPtr t, unsigned int priority, const std::string& name) {
        auto comp = TransformComponentPtr(new TransformComponent(t, priority));
        comp->setName(name);
        return comp;
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