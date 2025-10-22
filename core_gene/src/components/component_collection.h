#ifndef COMPONENT_COLLECTION_H
#define COMPONENT_COLLECTION_H
#pragma once

#include <vector>
#include <algorithm>
#include <memory>
#include <unordered_map>
#include <typeindex>
#include <iostream>
#include <stdexcept>
#include "component.h"

namespace scene {
    using SceneNode = node::Node<ComponentCollection>;
    using SceneNodePtr = std::shared_ptr<SceneNode>;
}

class ComponentCollection {
private:
    // For fast lookup of ALL components of a given type.
    std::unordered_map<std::type_index, std::vector<component::ComponentPtr>> m_type_map;
    
    // For fast lookup of NAMED components. Enforces name uniqueness.
    std::unordered_map<std::string, component::ComponentPtr> m_name_map;
    
    // For sorted iteration during apply/unapply. Contains all components.
    std::vector<component::ComponentPtr> m_components_vector;
    
    bool m_is_vector_sorted = false;

    void sortComponents() {
        std::sort(m_components_vector.begin(), m_components_vector.end(),
            [](const component::ComponentPtr& a, const component::ComponentPtr& b) {
                return a->getPriority() < b->getPriority();
            }
        );
        m_is_vector_sorted = true;
    }

public:
    /**
     * @brief Adds a component to the collection. A node can have multiple components of the same type.
     * @tparam T The concrete component type.
     * @param new_component A shared pointer to the component to add.
     * @param owner A pointer to the node that owns this collection.
     */
    template <typename T>
    void addComponent(std::shared_ptr<T> new_component, scene::SceneNodePtr owner) {
        if (!new_component) return;

        const std::string& name = new_component->getName();
        if (!name.empty()) {
            if (m_name_map.count(name)) {
                throw std::runtime_error("Component with name '" + name + "' already exists on this node.");
            }
            m_name_map[name] = new_component;
        }

        new_component->setOwner(owner);
        m_type_map[std::type_index(typeid(*new_component))].push_back(new_component);
        m_components_vector.push_back(new_component);
        m_is_vector_sorted = false;
    }

    // --- Component Getters ---

    /**
     * @brief Retrieves the FIRST UNNAMED component of a specific type.
     * Useful for components you often expect to be unique, like a camera.
     * @tparam T The component type to retrieve.
     * @return A shared_ptr to the component, or nullptr if not found.
     */
    template <typename T>
    std::shared_ptr<T> get() const {
        auto map_it = m_type_map.find(std::type_index(typeid(T)));
        if (map_it != m_type_map.end()) {
            for (const auto& comp : map_it->second) {
                if (comp->getName().empty()) {
                    return std::dynamic_pointer_cast<T>(comp);
                }
            }
        }
        return nullptr;
    }

    /**
     * @brief Gets a component by its unique name. This is the fastest way to get a specific component.
     */
    template <typename T>
    std::shared_ptr<T> get(const std::string& name) const {
        auto map_it = m_name_map.find(name);
        if (map_it != m_name_map.end()) {
            return std::dynamic_pointer_cast<T>(map_it->second);
        }
        return nullptr;
    }

    /**
     * @brief Retrieves ALL components of a specific type.
     * @tparam T The component type to retrieve.
     * @return A vector of shared_ptrs to the components. The vector will be empty if none are found.
     */
    template <typename T>
    std::vector<std::shared_ptr<T>> getAll() const {
        std::vector<std::shared_ptr<T>> result;
        auto map_it = m_type_map.find(std::type_index(typeid(T)));
        if (map_it != m_type_map.end()) {
            result.reserve(map_it->second.size());
            for (const auto& base_ptr : map_it->second) {
                if (auto derived_ptr = std::dynamic_pointer_cast<T>(base_ptr)) {
                    result.push_back(derived_ptr);
                }
            }
        }
        return result;
    }

    // --- Component Removers ---

    /**
     * @brief Removes a component instance directly. This is the safest removal method.
     * @return True if the component was found and removed, false otherwise.
     */
    bool removeComponent(const component::ComponentPtr& component_to_remove) {
        if (!component_to_remove) return false;

        // Erase from the main vector
        auto& vec = m_components_vector;
        auto it = std::remove(vec.begin(), vec.end(), component_to_remove);
        if (it == vec.end()) return false; // Not found
        vec.erase(it, vec.end());

        // Erase from name map if named
        if (!component_to_remove->getName().empty()) {
            m_name_map.erase(component_to_remove->getName());
        }

        // Erase from type map
        auto& type_vec = m_type_map.at(std::type_index(typeid(*component_to_remove)));
        type_vec.erase(std::remove(type_vec.begin(), type_vec.end(), component_to_remove), type_vec.end());

        return true;
    }

    /**
     * @brief Removes a component by its unique name.
     * @return True if the component was found and removed, false otherwise.
     */
    bool removeComponent(const std::string& name) {
        auto it = m_name_map.find(name);
        if (it != m_name_map.end()) {
            return removeComponent(it->second);
        }
        return false;
    }

    /**
     * @brief Removes a component by its unique runtime ID.
     * @return True if the component was found and removed, false otherwise.
     */
    bool removeComponent(int component_id) {
        component::ComponentPtr component_to_remove = nullptr;
        for (const auto& comp : m_components_vector) {
            if (comp->getId() == component_id) {
                component_to_remove = comp;
                break;
            }
        }
        if (component_to_remove) {
            return removeComponent(component_to_remove);
        }
        return false;
    }
    
    // --- Lifecycle Methods ---

    void apply(bool print=false) {
        if (!m_is_vector_sorted) {
            sortComponents();
        }
        for (const auto& component : m_components_vector) {
            if (print) std::cout << "Component Type: " << component->getTypeName() << std::endl;
            component->apply();
        }
        if (print) std::cout << std::endl;
    }

    void unapply() {
        if (!m_is_vector_sorted) {
            sortComponents();
        }
        // IMPORTANT: Unapply in REVERSE order of application.
        for (auto it = m_components_vector.rbegin(); it != m_components_vector.rend(); ++it) {
            (*it)->unapply();
        }
    }
};

#endif