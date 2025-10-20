#ifndef GLOBAL_RESOURCE_MANAGER_H
#define GLOBAL_RESOURCE_MANAGER_H
#pragma once

#include "shader_resource.h"
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <iostream> // For error/warning logging
#include <algorithm> // For std::remove_if

#include "../shader.h"

namespace uniform {

/**
 * @class GlobalResourceManager
 * @brief A singleton to orchestrate all global ShaderResource objects.
 *
 * This manager handles the registration, unregistration, per-frame updates, and
 * shader binding for all registered UBOs, SSBOs, etc. It is the
 * central component that automates resource management.
 */
class GlobalResourceManager {
private:
    std::unordered_map<std::string, ShaderResourcePtr> m_known_resources;
    std::vector<ShaderResourcePtr> m_per_frame_resources;

    /**
     * @brief Private constructor to enforce the singleton pattern.
     */
    GlobalResourceManager() = default;
    friend GlobalResourceManager& manager();

public:
    // This is a singleton, so it should not be copyable or movable.
    GlobalResourceManager(const GlobalResourceManager&) = delete;
    GlobalResourceManager& operator=(const GlobalResourceManager&) = delete;
    GlobalResourceManager(GlobalResourceManager&&) = delete;
    GlobalResourceManager& operator=(GlobalResourceManager&&) = delete;

    /**
     * @brief Registers a new shader resource, handling name collisions.
     * @param resource The resource to register.
     */
    void registerResource(ShaderResourcePtr resource) {
        if (!resource) return;

        const std::string& name = resource->getName();
        auto it = m_known_resources.find(name);

        if (it != m_known_resources.end()) {
            // A resource with this name already exists. Clean it up before overwriting.
            ShaderResourcePtr oldResource = it->second;
            if (oldResource->getUpdateMode() == UpdateMode::PER_FRAME) {
                // Remove the old resource from the per-frame update list.
                m_per_frame_resources.erase(
                    std::remove(m_per_frame_resources.begin(), m_per_frame_resources.end(), oldResource),
                    m_per_frame_resources.end()
                );
            }
        }

        m_known_resources[name] = resource;

        if (resource->getUpdateMode() == UpdateMode::PER_FRAME) {
            m_per_frame_resources.push_back(resource);
        }
    }

    /**
     * @brief Unregisters a shader resource by its name.
     * @param resourceName The name of the resource to remove.
     */
    void unregisterResource(const std::string& resourceName) {
        auto it = m_known_resources.find(resourceName);
        if (it == m_known_resources.end()) {
            std::cerr << "Warning: Attempted to unregister non-existent resource '" << resourceName << "'." << std::endl;
            return;
        }

        ShaderResourcePtr resource = it->second;
        if (resource->getUpdateMode() == UpdateMode::PER_FRAME) {
            // Remove from the efficient update list first.
            m_per_frame_resources.erase(
                std::remove(m_per_frame_resources.begin(), m_per_frame_resources.end(), resource),
                m_per_frame_resources.end()
            );
        }

        m_known_resources.erase(it);
    }
    
    /**
     * @brief Unregisters all currently managed resources.
     */
    void unregisterAllResources() {
        m_known_resources.clear();
        m_per_frame_resources.clear();
    }

    /**
     * @brief Applies all registered PER_FRAME resources. Called once per frame.
     */
    void applyPerFrame() {
        for (const auto& resource : m_per_frame_resources) {
            resource->apply();
        }
    }

    /**
     * @brief Manually triggers the apply method for a specific ON_DEMAND resource.
     * @param resourceName The name of the resource to apply.
     */
    void applyShaderResource(const std::string& resourceName) {
        auto it = m_known_resources.find(resourceName);
        if (it != m_known_resources.end()) {
            it->second->apply();
        } else {
            std::cerr << "Warning: Attempted to apply non-existent resource '" << resourceName << "'." << std::endl;
        }
    }

    /**
     * @brief Binds a resource's uniform block to a shader program.
     * @param shader_pid The OpenGL program ID of the shader.
     * @param resourceName The name of the registered resource to bind.
     */
    void bindResourceToShader(GLuint shader_pid, const std::string& resourceName) {
        auto it = m_known_resources.find(resourceName);
        if (it == m_known_resources.end()) {
            std::cerr << "Error: Cannot bind resource. Resource '" << resourceName << "' not registered." << std::endl;
            return;
        }

        GLuint block_index = glGetUniformBlockIndex(shader_pid, resourceName.c_str());

        if (block_index == GL_INVALID_INDEX) {
            // This is a valid scenario if a shader doesn't use a global uniform.
            return;
        }

        GLuint binding_point = it->second->getBindingPoint();
        glUniformBlockBinding(shader_pid, block_index, binding_point);
    }

    /**
     * @brief Binds a resource's uniform block to a shader (overload).
     * @param shader A shared pointer to the shader object.
     * @param resourceName The name of the registered resource to bind.
     */
    void bindResourceToShader(ShaderPtr shader, const std::string& resourceName) {
        if (shader) {
            bindResourceToShader(shader->GetShaderID(), resourceName);
        }
    }


    /**
     * @brief Binds all registered resources to a given shader.
     * @param shader A shared pointer to the shader object.
     */
    void bindAllResourcesToShader(ShaderPtr shader) {

        if (!shader) return;

        GLuint shader_pid = shader->GetShaderID();

        for (const auto& pair : m_known_resources) {

            bindResourceToShader(shader_pid, pair.first);

        }
    }

};

/**
 * @brief Provides access to the singleton instance of the GlobalResourceManager.
 * @return A reference to the singleton manager.
 */
inline GlobalResourceManager& manager() {
    static GlobalResourceManager instance;
    return instance;
}

} // namespace uniform

#endif // GLOBAL_RESOURCE_MANAGER_H

