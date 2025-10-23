#ifndef SCENE_H
#define SCENE_H
#pragma once

#include <memory>
#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>

// Core engine headers
#include "node.h"
#include "../components/component_collection.h"
#include "../3d/camera/camera.h"
#include "../3d/camera/orthographic_camera.h"
#include "../gl_base/transform.h"
#include "../exceptions/node_not_found_exception.h"


namespace scene {

// Forward-declare the builder class for our specific SceneNode
class SceneNodeBuilder;

// --- Scene-Specific Node Definition ---
using SceneNode = node::Node<ComponentCollection>;
using SceneNodePtr = std::shared_ptr<SceneNode>;

class SceneGraph;
using SceneGraphPtr = std::shared_ptr<SceneGraph>;

class SceneGraph {
private:
    SceneNodePtr root;
    std::unordered_map<std::string, SceneNodePtr> name_map;
    std::unordered_map<int, SceneNodePtr> node_map;
    component::CameraPtr m_active_camera;

    SceneGraph() {
        root = SceneNode::Make("root");
        // ** Important: ** The root node must also be configured with behavior.
        configureNodeForDrawing(root);

        name_map["root"] = root;
        node_map[root->getId()] = root;

        // --- Create and set a default Orthographic Camera ---
        // 1. Create the node and add it to the graph using the internal method.
        //    (This is what the builder's 'addNode' would have done).
        SceneNodePtr default_cam_node = this->addNode("_default_camera", root);

        // 2. Check for successful creation and add the component.
        if (default_cam_node) {
            // 3. Add the component directly.
            //    (This is what the builder's '.with<>()' would have done).
            default_cam_node->payload().addComponent(component::OrthographicCamera::Make(), default_cam_node);

            // 4. Set the active camera from the new component.
            m_active_camera = default_cam_node->payload().get<component::Camera>();
        } else {
            // This else block handles the case where addNode returns nullptr
            std::cerr << "CRITICAL ERROR: Failed to create default camera node in SceneGraph constructor." << std::endl;
            m_active_camera = nullptr; // Explicitly set to null
        }

        // The active camera is the component we just added to the new node.
        m_active_camera = default_cam_node->payload().get<component::Camera>();

        if (!m_active_camera) {
            // This else block handles the case where ComponentCollection::get returns nullptr
            std::cerr << "CRITICAL ERROR: Failed to find default camera component." << std::endl;
        }
    }

    // The builder class for SceneNodes is a friend.
    friend class SceneNodeBuilder;
    friend SceneGraphPtr graph();

    void registerNode(SceneNodePtr node) {
        if (!node) return;
        if (name_map.count(node->getName())) {
            std::cerr << "Warning: A node with name '" << node->getName() << "' is already registered." << std::endl;
        }
        name_map[node->getName()] = node;
        node_map[node->getId()] = node;
    }

    /**
     * @brief A centralized helper to attach drawing behavior to a node.
     * This is the core of the Strategy Pattern implementation for drawing.
     */
    void configureNodeForDrawing(const SceneNodePtr& node) {
        if (!node) return;

        // The pre-visit action: apply the node's components.
        node->onPreVisit([](SceneNode& n) {
            n.payload().apply(); // The payload is the ComponentCollection
        });

        // The post-visit action: unapply the node's components.
        node->onPostVisit([](SceneNode& n) {
            n.payload().unapply();
        });
    }

public:
    // --- Builder Pattern Entry Point ---
    /**
     * @brief Begins a fluent build process by adding a new node to the root of the scene.
     * @param name The name of the new node.
     * @return A SceneNodeBuilder for the newly created node.
     */
    SceneNodeBuilder addNode(const std::string& name);

    // --- Core Graph Management (used by the builder and for direct manipulation) ---
    SceneNodePtr getRoot() const { return root; }

    SceneNodePtr getNodeByName(const std::string& name) {
        auto it = name_map.find(name);
        return (it != name_map.end()) ? it->second : nullptr;
    }

    SceneNodePtr getNodeById(int id) {
        auto it = node_map.find(id);
        return (it != node_map.end()) ? it->second : nullptr;
    }

    /**
     * @brief Finds a node by name and returns a SceneNodeBuilder to begin a fluent build process from it.
     * @param name The name of the existing node to find.
     * @return A SceneNodeBuilder for the found node.
     * @throws NodeNotFoundException if the node is not found.
     */
    SceneNodeBuilder buildAt(const std::string& name);

    // Internal workhorse method used by the builder.
    SceneNodePtr addNode(const std::string& name, SceneNodePtr parent) {
        if (name_map.find(name) != name_map.end()) {
            std::cerr << "Node with name " << name << " already exists!" << std::endl;
            return nullptr;
        }
        
        SceneNodePtr new_node = SceneNode::Make(name);
        // ** CRITICAL **: Every new node gets configured for drawing by default.
        configureNodeForDrawing(new_node);

        (parent ? parent : root)->addChild(new_node);
        registerNode(new_node);
        return new_node;
    }

    bool renameNode(SceneNodePtr node_to_rename, const std::string& new_name) {
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
    void removeNode(SceneNodePtr node_to_remove) {
        if (!node_to_remove || node_to_remove == root) {
            std::cerr << "Cannot remove null or root node!" << std::endl;
            return;
        }
        
        SceneNodePtr parent = node_to_remove->getParent();
        if (parent) {
            parent->removeChild(node_to_remove);
        }
        name_map.erase(node_to_remove->getName());
        node_map.erase(node_to_remove->getId());
    }

    SceneNodePtr duplicateNode(const std::string& source_name, const std::string& new_name) {
        if (name_map.find(new_name) != name_map.end()) {
            std::cerr << "Node with name " << new_name << " already exists!" << std::endl;
            return nullptr;
        }
        SceneNodePtr node_to_copy = getNodeByName(source_name);
        if (node_to_copy) {
            SceneNodePtr new_node = SceneNode::Make(new_name);
            // TODO: Deep copy components from node_to_copy to new_node
            
            SceneNodePtr parent = node_to_copy->getParent();
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
    // --- NEW --- Camera Management ---
    /**
     * @brief Sets the active camera by finding a node by name.
     * If the node is not found or has no camera, the active camera is not changed.
     */
    void setActiveCamera(const std::string& node_name) {
        SceneNodePtr node = getNodeByName(node_name);
        if (!node) {
            std::cerr << "Warning: Cannot set active camera. Node '" << node_name << "' not found." << std::endl;
            return;
        }
        setActiveCamera(node); // Delegate to the node-based version
    }

    /**
     * @brief Sets the active camera from a node that contains an Camera component.
     * If the node is null or has no camera, the active camera is not changed.
     */
    void setActiveCamera(SceneNodePtr camera_node) {
        if (!camera_node) {
            std::cerr << "Warning: Cannot set active camera from a null node." << std::endl;
            return;
        }
        auto camera = camera_node->payload().get<component::Camera>();
        if (!camera) {
            std::cerr << "Warning: Node '" << camera_node->getName() << "' has no Camera component. Active camera unchanged." << std::endl;
            return;
        }
        setActiveCamera(camera); // Delegate to the component-based version
    }

    /**
     * @brief Sets the active camera directly from a camera component pointer.
     * This is the primary setter. If the camera is null, the active camera is not changed.
     */
    void setActiveCamera(component::CameraPtr camera) {
        if (!camera) {
            std::cerr << "Warning: Attempted to set a null camera pointer as active. Active camera unchanged." << std::endl;
            return;
        }
        m_active_camera = camera;
    }

    component::CameraPtr getActiveCamera() const {
        return m_active_camera;
    }

    /**
     * @brief Draws the entire scene by initiating a visit from the root.
     * It uses the active camera to set up the view and projection matrices.
     */
    void draw(float aspect_ratio = 1.0f) {
        if (root) {
            root->visit();
        }
    }

    /**
     * @brief Draws a specific subtree by initiating a visit from the given node.
     */
    void drawSubtree(SceneNodePtr node, float aspect_ratio = 1.0f) {
        if (node) {
            node->visit();
        }
    }

    void drawSubtree(const std::string& node_name, float aspect_ratio = 1.0f) { // --- MODIFIED ---
        SceneNodePtr node = getNodeByName(node_name);
        if (node) {
            drawSubtree(node, aspect_ratio);
        } else {
            std::cerr << "Node with name " << node_name << " not found!" << std::endl;
        }
    }
    
    void clearGraph() {
        root = SceneNode::Make("root");
        configureNodeForDrawing(root);
        name_map.clear();
        node_map.clear();
        name_map["root"] = root;
        node_map[root->getId()] = root;
    }
};

inline SceneGraphPtr graph() {
    static SceneGraphPtr instance = SceneGraphPtr(new SceneGraph());
    return instance;
}

} // namespace scene

#endif // SCENE_H