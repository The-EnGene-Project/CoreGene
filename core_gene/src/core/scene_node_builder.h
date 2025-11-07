#ifndef SCENE_NODE_BUILDER_H
#define SCENE_NODE_BUILDER_H
#pragma once

#include "scene.h"
#include <string>

namespace scene {

/**
 * @class SceneNodeBuilder
 * @brief A fluent builder for constructing and configuring nodes in the SceneGraph.
 *
 * This class provides a declarative, chainable interface for building a scene graph.
 */
class SceneNodeBuilder {
private:
    SceneNodePtr node_; // A shared pointer to the node being configured.

public:
    /**
     * @brief Constructs a SceneNodeBuilder, wrapping the given node.
     * @param node The node to be configured by this builder.
     */
    explicit SceneNodeBuilder(SceneNodePtr node) : node_(node) {}

    /**
     * @brief Adds an unnamed component to the current node.
     *
     * This is a template method that forwards arguments to the component's
     * static `Make` function, creating and adding it to the node.
     *
     * @tparam T The type of the component to add (e.g., component::TransformComponent).
     * @tparam Args The types of arguments for the component's constructor.
     * @param args The arguments to forward to the component's `T::Make` function.
     * @return A reference to the current builder to allow for chaining.
     */
    template<typename T, typename... Args>
    SceneNodeBuilder& with(Args&&... args) {
        if (node_) {
            auto component = T::Make(std::forward<Args>(args)...);
            // Pass the raw owner pointer to addComponent as required.
            node_->payload().addComponent(component, node_);
        }
        return *this;
    }

    /**
     * @brief Adds a named component to the current node.
     *
     * This provides an explicit way to add a component that can be retrieved later by its name.
     * Convention: Assumes `T::Make` has an overload that takes a `const std::string& name`
     * as its final argument.
     *
     * @tparam T The type of the component to add.
     * @tparam Args The types of arguments for the component's constructor.
     * @param name The unique name for this component instance on this node.
     * @param args The arguments to forward to the component's `T::Make` function.
     * @return A reference to the current builder for chaining.
     */
    template<typename T, typename... Args>
    SceneNodeBuilder& withNamed(const std::string& name, Args&&... args) {
        if (node_) {
            // Forward the arguments and the name to the component's factory function.
            auto component = T::Make(std::forward<Args>(args)..., name);
            node_->payload().addComponent(component, node_);
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
    SceneNodeBuilder addNode(const std::string& name) {
        if (!node_) {
            std::cerr << "Cannot add a child to a null node." << std::endl;
            // Return a builder wrapping a null pointer to prevent further operations
            return SceneNodeBuilder(nullptr);
        }
        // Use the SceneGraph's central method to create and register the node
        SceneNodePtr new_child = scene::graph()->addNode(name, node_);
        return SceneNodeBuilder(new_child);
    }
    
    /**
     * @brief Implicit conversion to a reference to the underlying Node.
     * Allows assigning the result of a build chain to a `node::Node&`.
     */
    operator SceneNode&() const { return *node_; }

    /**
     * @brief Implicit conversion to a shared pointer to the underlying Node.
     * Allows assigning the result of a build chain to a `node::NodePtr`.
     */
    operator SceneNodePtr() const { return node_; }
};

// --- Implementation of SceneGraph's Builder Entry Points ---

inline SceneNodeBuilder SceneGraph::addNode(const std::string& name) {
    // This creates a new node attached to the graph's root.
    SceneNodePtr new_node = this->addNode(name, this->root);
    return SceneNodeBuilder(new_node);
}

inline SceneNodeBuilder SceneGraph::buildAt(const std::string& name) {
    SceneNodePtr node = getNodeByName(name);
    if (node) {
        return SceneNodeBuilder(node);
    }
    throw exception::NodeNotFoundException(name);
}

} // namespace scene

#endif // SCENE_NODE_BUILDER_H