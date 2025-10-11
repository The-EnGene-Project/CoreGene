#ifndef NODE_BUILDER_H
#define NODE_BUILDER_H
#pragma once

#include "scene.h" // Required for scene::graph() and SceneGraph class definition
#include "node.h"  // Required for node::NodePtr
#include "../components/component.h" // Required for component::ComponentPtr

namespace scene {

// Forward declaration for the class defined in this file.
class NodeBuilder;

/**
 * @class NodeBuilder
 * @brief A fluent builder for constructing and configuring nodes in the SceneGraph.
 *
 * This class provides a declarative, chainable interface for building a scene graph,
 * replacing the procedural, stateful SceneGraphVisitor.
 */
class NodeBuilder {
private:
    node::NodePtr node_; // A shared pointer to the node being configured.

public:
    /**
     * @brief Constructs a NodeBuilder, wrapping the given node.
     * @param node The node to be configured by this builder.
     */
    explicit NodeBuilder(node::NodePtr node) : node_(node) {}

    /**
     * @brief Adds a component to the current node.
     *
     * This is a template method that forwards arguments to the component's
     * static `Make` function, creating and adding it to the node.
     *
     * @tparam T The type of the component to add (e.g., component::TransformComponent).
     * @tparam Args The types of arguments for the component's constructor.
     * @param args The arguments to forward to the component's constructor.
     * @return A reference to the current builder to allow for chaining.
     */
    template<typename T, typename... Args>
    NodeBuilder& with(Args&&... args) {
        if (node_) {
            // Create the component using its static Make function
            auto component = T::Make(std::forward<Args>(args)...);
            node_->addComponent(std::static_pointer_cast<component::Component>(component));
        }
        return *this;
    }

    /**
     * @brief Adds a new child node to the current node and returns a builder for it.
     *
     * This method is the key to building the hierarchy. It creates a new node,
     * attaches it to the current node, and returns a *new* builder for the child.
     *
     * @param name The name of the new child node.
     * @return A new NodeBuilder instance for the newly created child node.
     */
    NodeBuilder addNode(const std::string& name) {
        if (!node_) {
            std::cerr << "Cannot add a child to a null node." << std::endl;
            // Return a builder wrapping a null pointer to prevent further operations
            return NodeBuilder(nullptr);
        }
        // Use the SceneGraph's central method to create and register the node
        node::NodePtr new_child = scene::graph()->addNode(name, node_);
        return NodeBuilder(new_child);
    }

    /**
     * @brief Implicit conversion to a reference to the underlying Node.
     * Allows assigning the result of a build chain to a `node::Node&`.
     */
    operator node::Node&() const {
        return *node_;
    }

    /**
     * @brief Implicit conversion to a shared pointer to the underlying Node.
     * Allows assigning the result of a build chain to a `node::NodePtr`.
     */
    operator node::NodePtr() const {
        return node_;
    }
};

// --- Implementation of SceneGraph's Builder Entry Point ---
// We define this here to break the circular include dependency.
// scene.h forward-declares NodeBuilder, and this file includes scene.h,
// so now we can define the method that returns a full NodeBuilder object.

inline NodeBuilder SceneGraph::addNode(const std::string& name) {
    // This creates a new node attached to the graph's root.
    node::NodePtr new_node = this->addNode(name, this->root);
    return NodeBuilder(new_node);
}

} // namespace scene

#endif // NODE_BUILDER_H