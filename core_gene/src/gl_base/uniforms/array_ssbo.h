#ifndef ARRAY_SSBO_H
#define ARRAY_SSBO_H
#pragma once

#include "shader_resource.h"
#include "global_buffer_manager.h" 
#include <vector>
#include <string>

namespace uniform {

template <typename T> class ArraySSBO; // Forward declaration

template <typename T>
using ArraySSBOPtr = std::shared_ptr<ArraySSBO<T>>;

/**
 * @class ArraySSBO
 * @brief A concrete ShaderResource for a dynamically-sized Shader Storage Buffer Object.
 *
 * @tparam T The type of a single element in the array.
 *
 * This class manages a buffer that can change size at runtime. It does not use
 * the provider model, instead offering an explicit `upload` interface.
 */
template <typename T>
class ArraySSBO : public ShaderResource {
protected:
    /**
     * @brief Constructs the ArraySSBO.
     * @param name A unique name for this buffer.
     * @param bindingPoint The binding point to associate with this buffer.
     */
    explicit ArraySSBO(std::string name, GLuint bindingPoint)
        : ShaderResource(std::move(name), UpdateMode::ON_DEMAND, bindingPoint)
    {
        // Initial buffer creation with no data.
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffer_id);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_binding_point, m_buffer_id);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

public:
    // This object manages an OpenGL resource, so it should not be copyable.
    ArraySSBO(const ArraySSBO&) = delete;
    ArraySSBO& operator=(const ArraySSBO&) = delete;

    /**
     * @brief Factory function to create and register a new ArraySSBO.
     * @param name A unique name for this buffer.
     * @param bindingPoint The binding point to associate with this buffer.
     * @return A shared pointer to the newly created ArraySSBO.
     */
    static ArraySSBOPtr<T> Make(std::string name, GLuint bindingPoint) {
        auto ssbo_ptr = ArraySSBOPtr<T>(new ArraySSBO<T>(std::move(name), bindingPoint));
        
        uniform::manager().registerBuffer(ssbo_ptr);
        
        return ssbo_ptr;
    }
    
    /**
     * @brief Uploads a vector of data to the GPU, reallocating the buffer storage.
     * @param data The vector of data to upload.
     * @param usage The OpenGL usage hint (e.g., GL_DYNAMIC_DRAW).
     */
    void upload(const std::vector<T>& data, GLenum usage = GL_DYNAMIC_DRAW) {
        // --- STUB ---
        // Implementation would bind the buffer and call glBufferData
        // with data.size() * sizeof(T) and data.data().
    }

    /**
     * @brief Reallocates the buffer to a new size without uploading data.
     * @param element_count The new number of elements of type T.
     * @param usage The OpenGL usage hint (e.g., GL_DYNAMIC_DRAW).
     */
    void resize(size_t element_count, GLenum usage = GL_DYNAMIC_DRAW) {
        // --- STUB ---
        // Implementation would bind the buffer and call glBufferData
        // with element_count * sizeof(T) and nullptr.
    }

    /**
     * @brief The apply method for dynamic SSBOs is a no-op, as updates
     * are handled explicitly via the `upload` method.
     */
    void apply() override {
        // No-op for this buffer type.
    }
};

} // namespace uniform

#endif // ARRAY_SSBO_H
