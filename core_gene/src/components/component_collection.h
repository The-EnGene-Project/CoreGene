#ifndef COMPONENT_COLLECTION_H
#define COMPONENT_COLLECTION_H
#pragma once

#include <vector>
#include <algorithm>
#include "component.h"

class ComponentCollection {
private:
    std::vector<component::ComponentPtr> components;
    bool are_components_sorted = true;

    void sortComponents() {
        std::sort(components.begin(), components.end(),
            [](const component::ComponentPtr& a, const component::ComponentPtr& b) {
                // Sort by the priority number.
                return a->getPriority() < b->getPriority();
            }
        );
        are_components_sorted = true; // Mark as clean
    }
public:

    void addComponent(component::ComponentPtr new_component) {
        components.push_back(new_component);
    }

    // TODO: BACALHAU checar indice e repensar como achar o componente a ser removido...
    void removeComponentByIndex(int index) {
        components.erase(components.begin() + index);
    }

    void removeComponent(int component_id) {
        components.erase(std::remove_if(components.begin(), components.end(),
                                     [&](const component::ComponentPtr c) {
                                         return c->getId() == component_id;
                                     }),
                        components.end());
    }

    virtual void apply() {
        // If the component list has changed, sort it before execution.
        if (!are_components_sorted) {
            sortComponents();
        }

        for (const auto& component : components) {
            component->apply();
        }
    }

    virtual void unapply() {
        if (!are_components_sorted) {
            sortComponents();
        }

        // IMPORTANT: Unapply in REVERSE order of application.
        for (auto it = components.rbegin(); it != components.rend(); ++it) {
            (*it)->unapply();
        }
    }
    
};

#endif