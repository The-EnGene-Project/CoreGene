#ifndef NODE_H
#define NODE_H
#pragma once

#include <memory>
#include <vector>
#include <algorithm>
#include <string>
#include <iostream>
#include "gl_base/error.h"
#include "components/component_collection.h"
#include "components/component.h"

// #include "geometry.h"
// #include "transform.h"
// #include "shader.h"

namespace scene {
    class SceneGraph;
}

namespace node {

class Node;
using NodePtr = std::shared_ptr<Node>;

using ParentPtr = std::weak_ptr<Node>; // ponteiro fraco para evitar ciclos de referência

class Node : public std::enable_shared_from_this<Node>{
private:
    int id;
    inline static int next_id = 0;
    std::string name;
    std::vector<NodePtr> children;
    int child_count = 0;
    ParentPtr parent; // pode ser nulo
    bool applicability = true;
    bool local_applicability = true;
    ComponentCollection components;

    
    friend class scene::SceneGraph;

protected:
    
    Node(std::string name) : 
    name(name)
    {
        id = next_id++;
    }

    void setName(const std::string& new_name) {
        name = new_name;
    }

    void setParent(NodePtr new_parent) {
        parent = new_parent;
    }

    void setApplicability(bool new_applicability) {
        applicability = new_applicability;
    }

    // ComponentCollection getCollectionCopy() {
    //     return components.copy();
    // }

public:

    static NodePtr Make(std::string name) {
        return NodePtr(new Node(name));
    }

    static NodePtr Make(std::string name, NodePtr parent) {
        NodePtr n = NodePtr(new Node(name));
        n->parent = parent;
        return n;
    }

    int getId() const {
        return id;
    }

    const std::string& getName() const {
        return name;
    }

    NodePtr getParent() const {
        return parent.lock();
    }

    bool getApplicability() const {
        return applicability;
    }

    bool getLocalApplicability() const {
        return local_applicability;
    }

    void addComponent(component::ComponentPtr new_component) {
        components.addComponent(new_component);
    }

    void removeComponent(int component_id) {
        components.removeComponent(component_id);
    }

    void removeComponentByIndex(int index) {
        components.removeComponentByIndex(index);
    }

    int getChildCount() const {
        return child_count;
    }

    void updateChildCount() {
        child_count = children.size();
    }

    NodePtr getChild(int index) const {
        if (index < 0 || index >= children.size()) {
            std::cerr << "Index out of bounds in getChild" << std::endl;
            return nullptr;
        }
        return children[index];
    }

    NodePtr getChildByName(const std::string& child_name) const {
        for (NodePtr child : children) {
            if (child->name == child_name) {
                return child;
            }
        }
        std::cerr << "Child with name " << child_name << " not found in getChildByName" << std::endl;
        return nullptr;
    }

    int getChildIndex(std::string& child) const {
        for (int i = 0; i < children.size(); i++) {
            if (children[i]->name == child) {
                return i;
            }
        }
        std::cerr << "Child with name " << child << " not found in getChildIndex" << std::endl;
        return -1;
    }

    int getChildIndex(NodePtr child) const {
        for (int i = 0; i < children.size(); i++) {
            if (children[i] == child) {
                return i;
            }
        }
        std::cerr << "Child not found in getChildIndex" << std::endl;
        return -1;
    }

    void addChild(NodePtr child) {
        children.push_back(child);
        child->setParent(shared_from_this());
    }

    void addChild(NodePtr child, int index) {
        if (index < 0 || index > children.size()) {
            std::cerr << "Index out of bounds in addChild" << std::endl;
            return;
        }
        children.insert(children.begin() + index, child);
        child->setParent(shared_from_this());
    }

    void addChildFront(NodePtr child) {
        children.insert(children.begin(), child);
        child->setParent(shared_from_this());
    }

    void addChildAfter(NodePtr child, NodePtr after) {
        auto it = std::find(children.begin(), children.end(), after);
        if (it != children.end()) {
            children.insert(it + 1, child);
        } else {
            std::cerr << "Reference child not found in addChildAfter" << std::endl;
        }
        child->setParent(shared_from_this());
    }

    void moveChild(int from_idx, int to_idx) {
        if (from_idx < 0 || from_idx >= children.size() || to_idx < 0 || to_idx >= children.size()) {
            std::cerr << "Index out of bounds in moveChild" << std::endl;
            return;
        }
        NodePtr child = children[from_idx];
        children.erase(children.begin() + from_idx);
        children.insert(children.begin() + to_idx, child);
    }

    void swapChildren(int idx1, int idx2) {
        if (idx1 < 0 || idx1 >= children.size() || idx2 < 0 || idx2 >= children.size()) {
            std::cerr << "Index out of bounds in swapChildren" << std::endl;
            return;
        }
        std::swap(children[idx1], children[idx2]);
    }

    void removeChild(NodePtr child) {
        auto it = std::find(children.begin(), children.end(), child);
        if (it != children.end()) {
            (*it)->setParent(nullptr);
            children.erase(it);
        } else {
            std::cerr << "Child not found in removeChild" << std::endl;
        }
    }

    void draw(bool print = false) {
        if (!applicability) return;
        if (print) printf("Drawing node %s (id=%d) (parent=%s)\n", name.c_str(), id, parent.lock()? parent.lock()->getName().c_str() : "NONE");

        Error::Check("scene::Node::draw start");

        if (!local_applicability) components.apply(print);

        Error::Check("scene::Node::draw after apply");

        // Desenha os filhos
        for (NodePtr child : children) {
            child->draw(print);
        }
        Error::Check("scene::Node::draw after drawing children");

        if (!local_applicability) components.unapply();

        Error::Check("scene::Node::draw end");
        
    }
};

}


#endif