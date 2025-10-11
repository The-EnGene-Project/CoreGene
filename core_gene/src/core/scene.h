#ifndef SCENE_H
#define SCENE_H
#pragma once

#include <memory>
#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>

#include "node.h"
#include "node_builder.h"
#include "../gl_base/shader.h"
#include "../gl_base/transform.h"
#include "../components/component.h"
#include "../exceptions/node_not_found_exception.h"

namespace scene {

// Forward-declare the NodeBuilder class. Its full definition is in node_builder.h
class NodeBuilder;

class SceneGraph;
using SceneGraphPtr = std::shared_ptr<SceneGraph>;

class SceneGraph {
private:
    node::NodePtr root;
    std::unordered_map<std::string, node::NodePtr> name_map; // Mapeia nomes para nós
    std::unordered_map<int, node::NodePtr> node_map;      // Mapeia IDs para nós
    transform::TransformPtr view_transform;

    SceneGraph() {
        root = node::Node::Make("root");
        name_map["root"] = root;
        node_map[root->getId()] = root;
        view_transform = transform::Transform::Make();
        view_transform->orthographic(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f); // Ortográfica padrão
    }

    friend class NodeBuilder;
    friend SceneGraphPtr graph();

    void registerNode(node::NodePtr node) {
        if (name_map.count(node->getName())) {
            std::cerr << "Warning: A node with name '" << node->getName() << "' is already registered. Names should be unique." << std::endl;
        }
        name_map[node->getName()] = node;
        node_map[node->getId()] = node;
    }

public:
    // --- Builder Pattern Entry Point ---
    /**
     * @brief Begins a fluent build process by adding a new node to the root of the scene.
     * @param name The name of the new node.
     * @return A NodeBuilder for the newly created node.
     */
    NodeBuilder addNode(const std::string& name); // Implementation is in node_builder.h

    // --- Core Graph Management (used by the builder and for direct manipulation) ---
    node::NodePtr getRoot() const {
        return root;
    }

    node::NodePtr getNodeByName(const std::string& name) {
        auto it = name_map.find(name);
        return (it != name_map.end()) ? it->second : nullptr;
    }

    node::NodePtr getNodeById(int id) {
        auto it = node_map.find(id);
        return (it != node_map.end()) ? it->second : nullptr;
    }

    /**
     * @brief Finds a node by name and returns a NodeBuilder to begin a fluent build process from it.
     * @param name The name of the existing node to find.
     * @return A NodeBuilder for the found node.
     * @throws NodeNotFoundException if the node is not found.
     */
    NodeBuilder buildAt(const std::string& name) {
        node::NodePtr node = getNodeByName(name);
        if (node) {
            return NodeBuilder(node);
        }
        throw exception::NodeNotFoundException(name);
    }

    // This method is still useful for direct manipulation and is used by the NodeBuilder.
    node::NodePtr addNode(const std::string& name, node::NodePtr parent) {
        if (name_map.find(name) != name_map.end()) {
            std::cerr << "Node with name " << name << " already exists!" << std::endl;
            return nullptr;
        }
        if (!parent) {
            parent = root;
        }
        node::NodePtr new_node = node::Node::Make(name);
        parent->addChild(new_node);
        registerNode(new_node);
        return new_node;
    }

    bool renameNode(node::NodePtr node_to_rename, const std::string& new_name) {
        if (!node_to_rename) {
            std::cerr << "Cannot rename a null node!" << std::endl;
            return false;
        }
        if (name_map.find(new_name) != name_map.end()) {
            std::cerr << "Node with name " << new_name << " already exists!" << std::endl;
            return false;
        }
        name_map.erase(node_to_rename->getName());
        node_to_rename->setName(new_name);
        name_map[new_name] = node_to_rename;
        return true;
    }

    void removeNode(node::NodePtr node_to_remove) {
        if (!node_to_remove || node_to_remove == root) {
            std::cerr << "Cannot remove null or root node!" << std::endl;
            return;
        }
        node::NodePtr parent = node_to_remove->getParent();
        if (parent) {
            parent->removeChild(node_to_remove);
        }
        name_map.erase(node_to_remove->getName());
        node_map.erase(node_to_remove->getId());
    }

    node::NodePtr duplicateNode(const std::string& source_name, const std::string& new_name) {
        if (name_map.find(new_name) != name_map.end()) {
            std::cerr << "Node with name " << new_name << " already exists!" << std::endl;
            return nullptr;
        }
        node::NodePtr node_to_copy = getNodeByName(source_name);
        if (node_to_copy) {
            node::NodePtr new_node = node::Node::Make(new_name);
            // TODO: Deep copy components from node_to_copy to new_node
            
            node::NodePtr parent = node_to_copy->getParent();
            if (parent) {
                parent->addChild(new_node);
                registerNode(new_node);
                return new_node;
            } else {
                std::cerr << "Source node has no parent, cannot duplicate!" << std::endl;
                return nullptr;
            }
        } else {
            std::cerr << "Node with name " << source_name << " not found!" << std::endl;
            return nullptr;
        }
    }

    void setView(float left, float right, float bottom, float top, float near, float far) {
        view_transform->orthographic(left, right, bottom, top, near, far);
    }

    void clearGraph() {
        root = node::Node::Make("root");
        name_map.clear();
        node_map.clear();
        name_map["root"] = root;
        node_map[root->getId()] = root;
    }

    void draw(bool print = false) {
        transform::stack()->push(view_transform->getMatrix());
        if (root) {
            root->draw(print);
        }
        transform::stack()->pop();
        if (print) printf("\n--------------------------------\n\n");
    }

    void drawSubtree(node::NodePtr node, bool print = false) {
        transform::stack()->push(view_transform->getMatrix());
        if (node) {
            node->draw(print);
        }
        transform::stack()->pop();
        if (print) printf("\n--------------------------------\n\n");
    }

    void drawSubtree(const std::string& node_name) {
        node::NodePtr node = getNodeByName(node_name);
        if (node) {
            drawSubtree(node);
        } else {
            std::cerr << "Node with name " << node_name << " not found!" << std::endl;
        }
    }

    void drawSubtree(const int node_id) {
        node::NodePtr node = getNodeById(node_id);
        if (node) {
            drawSubtree(node);
        } else {
            std::cerr << "Node with id " << node_id << " not found!" << std::endl;
        }
    }
};

inline SceneGraphPtr graph() {
    static SceneGraphPtr instance = SceneGraphPtr(new SceneGraph());
    return instance;
}

} // namespace scene

#endif // SCENE_H