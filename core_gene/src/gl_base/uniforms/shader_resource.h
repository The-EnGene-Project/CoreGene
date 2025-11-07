#ifndef SHADER_RESOURCE_H
#define SHADER_RESOURCE_H
#pragma once

#include "../gl_includes.h"
#include <memory>
#include <string>

namespace uniform {

class ShaderResource; // Forward declaration
using ShaderResourcePtr = std::shared_ptr<ShaderResource>;

/**
 * @enum UpdateMode
 * @brief Defines how frequently a buffer object should be updated.
 */
enum class UpdateMode {
    PER_FRAME, // Automatically updated once per frame by the manager.
    ON_DEMAND  // Updated only when manually triggered.
};

/**
 * @class ShaderResource
 * @brief The abstract base class for all GPU buffer resources (UBOs, SSBOs).
 *
 * This non-templated class serves as the polymorphic interface for the
 * GlobalBufferManager, allowing it to manage various buffer types in a
 * uniform way. It holds the common data and behavior for all buffer objects.
 */
class ShaderResource {
protected:
    GLuint m_buffer_id = 0;
    std::string m_name;
    GLuint m_binding_point;
    UpdateMode m_update_mode;

    /**
     * @brief Protected constructor for the abstract base class.
     * @param name A unique name for this resource (for manager lookups).
     * @param mode The update frequency of this resource.
     * @param bindingPoint The binding point to associate with this resource.
     */
    explicit ShaderResource(std::string name, UpdateMode mode, GLuint bindingPoint)
        : m_name(std::move(name)),
          m_binding_point(bindingPoint),
          m_update_mode(mode)
    {
        glGenBuffers(1, &m_buffer_id);
    }

public:
    // This object manages an OpenGL resource, so it should not be copyable.
    ShaderResource(const ShaderResource&) = delete;
    ShaderResource& operator=(const ShaderResource&) = delete;

    /**
     * @brief Virtual destructor to ensure proper cleanup in derived classes.
     */
    virtual ~ShaderResource() {
        if (m_buffer_id != 0) {
            glDeleteBuffers(1, &m_buffer_id);
        }
    }

    /**
     * @brief Applies the buffer's data. For fixed-size buffers, this pushes
     * data from a provider to the GPU. For dynamic buffers, it may be a no-op.
     * This is the primary method called by the manager during the render loop.
     */
    virtual void apply() = 0;

    /**
     * @brief Retrieves the OpenGL buffer ID (handle).
     */
    virtual GLuint getBufferId() const { return m_buffer_id; }
    
    /**
     * @brief Retrieves the user-defined name of this resource.
     */
    virtual const std::string& getName() const { return m_name; }
    
    /**
     * @brief Retrieves the buffer's binding point.
     */
    virtual GLuint getBindingPoint() const { return m_binding_point; }
    
    /**
     * @brief Retrieves the buffer's configured update mode.
     */
    virtual UpdateMode getUpdateMode() const { return m_update_mode; }
};

} // namespace uniform

#endif // SHADER_RESOURCE_H
