#ifndef OBSERVED_TRANSFORM_COMPONENT_H
#define OBSERVED_TRANSFORM_COMPONENT_H
#pragma once

#include "transform_component.h"
#include "../gl_base/observer_interfaces.h"
#include "../core/node.h" // Include the full node definition for hierarchy traversal

namespace component {

class ObservedTransformComponent;
using ObservedTransformComponentPtr = std::shared_ptr<ObservedTransformComponent>;

/**
 * @class ObservedTransformComponent
 * @brief A specialized transform component that caches its world transform during
 * scene traversal and notifies final observers of the updated value.
 *
 * It acts as three things:
 * 1. A TransformComponent: To integrate with the scene graph's transform stack.
 * 2. An IObserver: To listen for changes from the raw `transform::Transform` object.
 * 3. An ISubject: To notify final listeners (like cameras) after its cache is updated.
 */
class ObservedTransformComponent : public TransformComponent, public IObserver, public ISubject {
private:
    glm::mat4 m_world_transform_cache;
    bool m_is_dirty;

    ObservedTransformComponent(
        transform::TransformPtr t, 
        unsigned int priority = ComponentPriority::TRANSFORM, 
        unsigned int min_bound = 0, 
        unsigned int max_bound = ComponentPriority::CAMERA
    ) :
        TransformComponent(t, priority, min_bound, max_bound),
        m_world_transform_cache(1.0f),
        m_is_dirty(true) // Start dirty to guarantee an initial update
    {
        // ** CRITICAL **: Subscribe to the raw transform object's notifications.
        getTransform()->addObserver(this);
    }

    /**
     * @brief Marks this component and all descendant components as dirty.
     *
     * This method uses the Node's `visit` traversal functionality to ensure
     * that the dirty flag is propagated throughout the entire node hierarchy,
     * even if intermediate nodes do not have an ObservedTransformComponent.
     */
    void markTransformAsDirty() {
        // OPTIMIZATION: If we are already dirty, our children have already been marked.
        // This prevents redundant work on sequential transform changes.
        if (m_is_dirty) {
            return;
        }

        // Mark myself as dirty.
        m_is_dirty = true;

        // If this component isn't attached to a node, we can't propagate.
        if (!m_owner) {
            return;
        }

        // Define the action to perform on the owner and every descendant node.
        auto propagate_dirty_flag = [](Node& node) {
            // Use getAll<> for robustness, in case a node has multiple components of this type.
            auto components = node.payload().getAll<ObservedTransformComponent>();
            for (const auto& comp : components) {
                if (comp) {
                    // The `if (m_is_dirty)` check at the top of this function will
                    // efficiently stop further recursion down this branch if it has already been visited.
                    comp->markTransformAsDirty();
                }
            }
        };

        m_owner->visit(propagate_dirty_flag, {}, true);
    }

    /**
     * @brief A helper to calculate the combined local transform for a given node.
     * It fetches all TransformComponents, sorts them by priority, and multiplies their
     * matrices in the correct order.
     * @param node The node to process.
     * @return The combined local transform matrix for the node.
     */
    glm::mat4 calculateCombinedLocalTransform(const scene::SceneNode* node) {
        if (!node) return glm::mat4(1.0f);

        auto all_local_transforms = node->payload().getAll<TransformComponent>();
        std::sort(all_local_transforms.begin(), all_local_transforms.end(),
            [](const std::shared_ptr<TransformComponent>& a, const std::shared_ptr<TransformComponent>& b) {
                return a->getPriority() < b->getPriority();
            }
        );

        glm::mat4 combined_local_transform(1.0f);
        for (auto it = all_local_transforms.rbegin(); it != all_local_transforms.rend(); ++it) {
            combined_local_transform = (*it)->getTransform()->getMatrix() * combined_local_transform;
        }
        return combined_local_transform;
    }

public:
    ~ObservedTransformComponent() {
        // Clean up the subscription when this component is destroyed.
        if (getTransform()) {
            getTransform()->removeObserver(this);
        }
    }

    static ObservedTransformComponentPtr Make(transform::TransformPtr t) {
        return ObservedTransformComponentPtr(new ObservedTransformComponent(t));
    }

    static ObservedTransformComponentPtr Make(transform::TransformPtr t, const std::string& name) {
        auto comp = ObservedTransformComponentPtr(new ObservedTransformComponent(t));
        comp->setName(name);
        return comp;
    }
    
    // Add static typename for better error messages
    static const char* getTypeNameStatic() { return "ObservedTransformComponent"; }

    // --- Layer 1: The First Notification ---
    /**
     * @brief Called by the raw `transform::Transform` when the local matrix changes.
     */
    void onNotify(const ISubject* subject) override {
        // The raw local transform has changed. Mark our cache as dirty.
        // Kick off the self-contained, recursive dirtying process.
        markTransformAsDirty();
    }

    // --- Layer 2: The Traversal Update and Second Notification ---
    /**
     * @brief During the scene traversal, this updates the cache and notifies final listeners.
     */
    void apply() override {
        // First, perform the normal transform stack operation.
        TransformComponent::apply();

        // Now, check if we need to update our cache.
        if (m_is_dirty) {
            // The top of the stack IS our current world transform. Cache it.
            m_world_transform_cache = transform::stack()->top();
            m_is_dirty = false;

            // ** CRITICAL **: Notify the final listeners (camera, physics, etc.)
            // that the new, updated data is available in our cache.
            notify();
        }
    }

    // --- Public Access for Final Listeners ---
    const glm::mat4& getCachedWorldTransform() const {
        return m_world_transform_cache;
    }

    /**
     * @brief [ON-DEMAND UPDATE] Calculates and returns the world transform immediately.
     * This version uses an iterative approach, walking up the scene graph hierarchy
     * from this component's owner to the root, accumulating transformations at each level.
     * @return A const reference to the up-to-date world transform matrix.
     */
    const glm::mat4& getWorldTransform() {
        if (!m_is_dirty) {
            return m_world_transform_cache;
        }

        if (!m_owner) {
             // Fallback for a component with no owner: world transform is its local transform.
            m_world_transform_cache = getTransform()->getMatrix();
            m_is_dirty = false;
            notify();
            return m_world_transform_cache;
        }

        // Start with the combined local transform of the node this component is attached to.
        glm::mat4 final_world_transform = calculateCombinedLocalTransform(m_owner);

        // --- NEW LOGIC: Iteratively walk up the parent hierarchy ---
        const scene::SceneNode* current_parent = m_owner->getParent();
        while (current_parent) {
            // Get the parent's combined local transform.
            glm::mat4 parent_local_transform = calculateCombinedLocalTransform(current_parent);
            
            // Pre-multiply our result with the parent's transform.
            final_world_transform = parent_local_transform * final_world_transform;
            
            // Move up to the next parent.
            current_parent = current_parent->getParent();
        }

        // Cache the final result.
        m_world_transform_cache = final_world_transform;

        m_is_dirty = false;
        notify();
        return m_world_transform_cache;
    }

    const char* getTypeName() const override {
        return "ObservedTransformComponent";
    }
};

} // namespace component

#endif // OBSERVED_TRANSFORM_COMPONENT_H