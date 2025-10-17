#ifndef COMPONENT_H
#define COMPONENT_H
#pragma once

#include <memory>
#include <string>

// Forward-declare SceneNode to avoid circular header includes.
namespace scene { class SceneNode; }

namespace component {

enum class ComponentPriority {
    TRANSFORM = 100,
    CAMERA = 200,      // Example for later
    SHADER = 300,
    APPEARANCE = 400,
    GEOMETRY = 500,    // Drawing should happen last
    CUSTOM_SCRIPT = 600
};

class Component;
using ComponentPtr = std::shared_ptr<Component>;

class Component {
private:
    int id;
    unsigned int priority;
    std::string m_name; // The optional unique name for this component instance
    inline static int next_id = 0;

protected:
    scene::SceneNode* m_owner;

    explicit Component(unsigned int p, const std::string& name = "") 
        : priority(p), m_name(name), m_owner(nullptr) {
        id = next_id++;
    }

    explicit Component(ComponentPriority p, const std::string& name = "") 
        : priority(static_cast<int>(p)), m_name(name), m_owner(nullptr) {
        id = next_id++;
    }

public:
    virtual ~Component() = default;

    void setOwner(scene::SceneNode* owner) {
        m_owner = owner;
    }
    
    // Static version for ComponentCollection messages
    static const char* getTypeNameStatic() { return "Component"; }
    
    virtual void apply() {}
    virtual void unapply() {}
    virtual unsigned int getPriority() { return priority; }
    int getId() { return id; }
    const std::string& getName() const { return m_name; }
    virtual const char* getTypeName() const = 0;
};

}
#endif