#ifndef STRUCT_RESOURCE_H
#define STRUCT_RESOURCE_H
#pragma once

#include "shader_resource.h"
#include <functional>
#include <vector>
#include <type_traits>

namespace uniform {

/**
 * @struct DirtyRegion
 * @brief Describes a sub-region of a buffer to be updated.
 */
struct DirtyRegion {
    size_t offset = 0; // The offset in bytes from the beginning of the buffer.
    size_t size = 0;   // The size in bytes of the data to update.
};

/**
 * @class StructResource
 * @brief An abstract, templated base class for shader resources that map
 * directly to a single C++ POD struct (e.g., UBOs, fixed-size SSBOs).
 *
 * @tparam T A POD struct that matches the GLSL block layout.
 *
 * This class implements the provider-based update logic for fixed-size buffers.
 */
template <typename T>
class StructResource : public ShaderResource {
    // Ensure that the template type T is a POD type for safe memory mapping.
    static_assert(std::is_standard_layout<T>::value && std::is_trivial<T>::value, 
                  "StructResource template type must be a POD struct.");

protected:
    GLenum m_buffer_type;

    // Providers for buffer data
    std::function<T()> m_full_provider;
    std::function<DirtyRegion(T& data)> m_partial_provider;

    /**
     * @brief Constructs the StructResource, allocating its GPU memory.
     * @param name A unique name for this resource.
     * @param mode The update frequency.
     * @param bindingPoint The binding point for this resource.
     * @param bufferType The specific OpenGL buffer type (e.g., GL_UNIFORM_BUFFER).
     */
    explicit StructResource(std::string name, UpdateMode mode, GLuint bindingPoint, GLenum bufferType)
        : ShaderResource(std::move(name), mode, bindingPoint),
          m_buffer_type(bufferType)
    {
        glBindBuffer(m_buffer_type, m_buffer_id);
        // Allocate a fixed-size block of memory on the GPU.
        glBufferData(m_buffer_type, sizeof(T), nullptr, GL_DYNAMIC_DRAW);
        // Bind the buffer to the specified indexed binding point.
        glBindBufferBase(m_buffer_type, m_binding_point, m_buffer_id);
        glBindBuffer(m_buffer_type, 0);
    }

public:
    /**
     * @brief Sets a simple data provider that returns the entire data structure.
     * @param provider A function (e.g., lambda) that returns a T object.
     */
    void setProvider(std::function<T()> provider) {
        m_full_provider = std::move(provider);
        m_partial_provider = nullptr; // Ensure only one provider type is active.
    }

    /**
     * @brief Sets an advanced data provider for partial buffer updates.
     * @param provider A function that takes a T&, modifies it, and returns a DirtyRegion.
     */
    void setPartialProvider(std::function<DirtyRegion(T& data)> provider) {
        m_partial_provider = std::move(provider);
        m_full_provider = nullptr; // Ensure only one provider type is active.
    }

    /**
     * @brief Executes the update logic by calling the configured provider
     * and pushing the resulting data to the GPU via glBufferSubData.
     */
    void apply() override {
        if (!m_full_provider && !m_partial_provider) {
            return; // No provider set, nothing to do.
        }

        glBindBuffer(m_buffer_type, m_buffer_id);

        if (m_full_provider) {
            T data = m_full_provider();
            glBufferSubData(m_buffer_type, 0, sizeof(T), &data);
        } else if (m_partial_provider) {
            // Use a stack-allocated buffer for efficiency if small, else heap.
            std::vector<char> temp_buffer(sizeof(T));
            T& typed_data = *reinterpret_cast<T*>(temp_buffer.data());
            
            DirtyRegion region = m_partial_provider(typed_data);
            
            if (region.size > 0) {
                glBufferSubData(m_buffer_type, region.offset, region.size, 
                                temp_buffer.data() + region.offset);
            }
        }

        glBindBuffer(m_buffer_type, 0);
    }
};

} // namespace uniform

#endif // STRUCT_RESOURCE_H
