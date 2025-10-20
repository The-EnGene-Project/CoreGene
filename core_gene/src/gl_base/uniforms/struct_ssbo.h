#ifndef STRUCT_SSBO_H
#define STRUCT_SSBO_H
#pragma once

#include "struct_resource.h"
// Include the manager header for registration in the Make function.
#include "global_buffer_manager.h" 
#include <string>

namespace uniform {

template <typename T> class StructSSBO; // Forward declaration

template <typename T>
using StructSSBOPtr = std::shared_ptr<StructSSBO<T>>;

/**
 * @class StructSSBO
 * @brief A concrete implementation of a ShaderResource for a fixed-size
 * Shader Storage Buffer Object.
 *
 * @tparam T A POD struct that matches the GLSL block layout.
 *
 * This class inherits all its logic from StructResource<T> and simply
 * specifies GL_SHADER_STORAGE_BUFFER as its type.
 */
template <typename T>
class StructSSBO : public StructResource<T> {
protected:
    /**
     * @brief Constructs the StructSSBO.
     * @param name A unique name for this buffer.
     * @param mode The update frequency of this buffer.
     * @param bindingPoint The binding point to associate with this buffer.
     */
    explicit StructSSBO(std::string name, UpdateMode mode, GLuint bindingPoint)
        : StructResource<T>(std::move(name), mode, bindingPoint, GL_SHADER_STORAGE_BUFFER)
    {
    }

public:
    // This object manages an OpenGL resource, so it should not be copyable.
    StructSSBO(const StructSSBO&) = delete;
    StructSSBO& operator=(const StructSSBO&) = delete;

    /**
     * @brief Factory function to create and register a new StructSSBO.
     * @param name A unique name for this buffer (for manager lookups).
     * @param mode The update frequency of this buffer.
     * @param bindingPoint The binding point to associate with this buffer.
     * @return A shared pointer to the newly created StructSSBO.
     */
    static StructSSBOPtr<T> Make(std::string name, UpdateMode mode, GLuint bindingPoint) {
        auto ssbo_ptr = StructSSBOPtr<T>(new StructSSBO<T>(std::move(name), mode, bindingPoint));
        
        // Automatically register with the central manager.
        uniform::manager().registerBuffer(ssbo_ptr);

        return ssbo_ptr;
    }
};

} // namespace uniform

#endif // STRUCT_SSBO_H