#ifndef OBSERVED_TRANSFORM_COMPONENT_H
#define OBSERVED_TRANSFORM_COMPONENT_H
#pragma once

#include "transform_component.h"
#include "../utils/observer_interface.h"

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
protected:
    glm::mat4 m_world_transform_cache;
    bool m_is_dirty;
    bool m_observers_registered;
    std::vector<transform::TransformPtr> m_observed_transforms;

    ObservedTransformComponent(
        transform::TransformPtr t, 
        unsigned int priority = static_cast<unsigned int>(ComponentPriority::TRANSFORM), 
        unsigned int min_bound = 0, 
        unsigned int max_bound = static_cast<unsigned int>(ComponentPriority::CAMERA)
    ) :
        TransformComponent(t, priority, min_bound, max_bound),
        m_world_transform_cache(1.0f),
        m_is_dirty(true),
        m_observers_registered(false)
    {
        // ** CRITICAL **: Subscribe to the raw transform object's notifications.
        getTransform()->addObserver(this);
    }

    /**
     * @brief Registers this component as an observer to all relevant transforms.
     * This includes:
     * - All ancestor transforms (from parent nodes up to root)
     * - Sibling transforms with lower priority (on the same node)
     */
    void registerTransformObservers() {
        // First, unregister from any previously observed transforms
        unregisterTransformObservers();

        if (!m_owner) {
            return;
        }

        // 1. Register sibling transforms with lower priority on the same node
        auto sibling_transforms = m_owner->payload().getAll<TransformComponent>();
        for (const auto& transform_comp : sibling_transforms) {
            if (transform_comp && transform_comp->getTransform() && 
                transform_comp.get() != this && // Don't observe ourselves
                transform_comp->getPriority() < this->getPriority()) { // Only lower priority
                
                transform_comp->getTransform()->addObserver(this);
                m_observed_transforms.push_back(transform_comp->getTransform());
            }
        }

        // 2. Walk up the scene graph and register as observer to all ancestor transforms
        scene::SceneNodePtr current_parent = m_owner->getParent();
        while (current_parent) {
            auto ancestor_transforms = current_parent->payload().getAll<TransformComponent>();
            
            for (const auto& transform_comp : ancestor_transforms) {
                if (transform_comp && transform_comp->getTransform()) {
                    transform_comp->getTransform()->addObserver(this);
                    m_observed_transforms.push_back(transform_comp->getTransform());
                }
            }
            
            current_parent = current_parent->getParent();
        }
    }

    /**
     * @brief Unregisters this component from all transform observations.
     */
    void unregisterTransformObservers() {
        for (auto& transform : m_observed_transforms) {
            if (transform) {
                transform->removeObserver(this);
            }
        }
        m_observed_transforms.clear();
    }



    /**
     * @brief A helper to calculate the combined local transform for a given node.
     * It fetches all TransformComponents, sorts them by priority, and multiplies their
     * matrices in the correct order.
     * @param node The node to process.
     * @return The combined local transform matrix for the node.
     */
    glm::mat4 calculateCombinedLocalTransform(const scene::SceneNodePtr node) {
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
        // Clean up all subscriptions when this component is destroyed.
        if (getTransform()) {
            getTransform()->removeObserver(this);
        }
        unregisterTransformObservers();
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
     * @brief Called when any observed transform changes (own, sibling, or ancestor).
     * Since we observe all relevant transforms, we only need to mark ourselves as dirty.
     * Descendants will be notified directly by their own ancestor observers.
     */
    void onNotify(const ISubject* subject) override {
        m_is_dirty = true;
    }

    // --- Layer 2: The Traversal Update and Second Notification ---
    /**
     * @brief During the scene traversal, this updates the cache and notifies final listeners.
     * Also registers as observer to relevant transforms on first apply.
     */
    void apply() override {
        // Register transform observers on first apply (when component is attached to scene graph)
        if (!m_observers_registered && m_owner) {
            registerTransformObservers();
            m_observers_registered = true;
        }

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
        scene::SceneNodePtr current_parent = m_owner->getParent();
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

    /**
     * @brief Manually refresh transform observers.
     * Call this if the scene graph hierarchy changes or components are added/removed
     * after this component is added.
     */
    void refreshTransformObservers() {
        unregisterTransformObservers();
        registerTransformObservers();
    }
};

} // namespace component

#endif // OBSERVED_TRANSFORM_COMPONENT_H