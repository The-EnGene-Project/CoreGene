#ifndef COMPONENT_H
#define COMPONENT_H
#pragma once

#include <memory>

namespace component {

enum class ComponentPriority {
    TRANSFORM = 100,
    CAMERA = 200,      // Example for later
    SHADER = 300,
    APPEARANCE = 400,
    GEOMETRY = 500,       // Drawing should happen last
    CUSTOM_SCRIPT = 600
};

class Component;
using ComponentPtr = std::shared_ptr<Component>;

class Component {
private:
    inline static int next_id = 0;
    int id;
    unsigned int priority;

protected:

    explicit Component(unsigned int p) :
    priority(p)
    {
        id = next_id++;
    }

    explicit Component(ComponentPriority p) :
    priority(static_cast<int>(p))
    {
        id = next_id++;
    }

public:
    virtual void apply() {}
    virtual void unapply() {}

    virtual unsigned int getPriority() {
        return priority;
    }

    int getId() {
        return id;
    }

    virtual const char* getTypeName() const = 0;
};


}


#endif