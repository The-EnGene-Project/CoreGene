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

    ObservedTransformComponent(transform::TransformPtr t) :
        TransformComponent(t),
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
     * This version correctly handles multiple TransformComponents (including inherited types)
     * on a single node, combining them based on their priority.
     * @return A const reference to the up-to-date world transform matrix.
     */
    const glm::mat4& getWorldTransform() {
        if (!m_is_dirty) {
            return m_world_transform_cache;
        }

        glm::mat4 parent_world_transform(1.0f);

        // Recursively get the parent's world transform. Its getWorldTransform() will
        // correctly perform this same logic for the parent node.
        if (m_owner && m_owner->getParent()) {
            if (auto parent_comp = m_owner->getParent()->payload().get<ObservedTransformComponent>()) {
                parent_world_transform = parent_comp->getWorldTransform();
            }
        }

        // --- NEW LOGIC: Calculate this node's combined local transform ---
        if (m_owner) {
            // 1. Get all components that inherit from TransformComponent. This is polymorphic and
            // will correctly fetch both TransformComponent and ObservedTransformComponent instances.
            auto all_local_transforms = m_owner->payload().getAll<TransformComponent>();

            // 2. Sort them by priority (ascending).
            std::sort(all_local_transforms.begin(), all_local_transforms.end(),
                [](const std::shared_ptr<TransformComponent>& a, const std::shared_ptr<TransformComponent>& b) {
                    return a->getPriority() < b->getPriority();
                }
            );

            // 3. Combine them by left-multiplying in REVERSE priority order.
            // This is equivalent to applying transformations on a stack: T * R * S * Vertex
            glm::mat4 combined_local_transform(1.0f);
            for (auto it = all_local_transforms.rbegin(); it != all_local_transforms.rend(); ++it) {
                combined_local_transform = (*it)->getTransform()->getMatrix() * combined_local_transform;
            }

            // Calculate our new world transform.
            m_world_transform_cache = parent_world_transform * combined_local_transform;
        } else {
             // Fallback for a component with no owner: world transform is its local transform.
            m_world_transform_cache = getTransform()->getMatrix();
        }

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