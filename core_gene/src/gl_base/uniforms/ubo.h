#ifndef UBO_H
#define UBO_H
#pragma once

#include "struct_resource.h"
// Include the manager header for registration in the Make function.
#include "global_resource_manager.h" 
#include <string>

namespace uniform {

template <typename T> class UBO; // Forward declaration

template <typename T>
using UBOPtr = std::shared_ptr<UBO<T>>;

/**
 * @class UBO
 * @brief A concrete implementation of a ShaderResource for a Uniform Buffer Object.
 *
 * @tparam T A POD struct that matches the GLSL uniform block layout.
 *
 * This class inherits all its logic from StructResource<T> and simply
 * specifies GL_UNIFORM_BUFFER as its type.
 */
template <typename T>
class UBO : public StructResource<T> {
protected:
    /**
     * @brief Constructs the UBO.
     * @param name A unique name for this buffer.
     * @param mode The update frequency of this buffer.
     * @param bindingPoint The binding point to associate with this buffer.
     */
    explicit UBO(std::string name, UpdateMode mode, GLuint bindingPoint)
        : StructResource<T>(std::move(name), mode, bindingPoint, GL_UNIFORM_BUFFER)
    {
    }

public:
    // This object manages an OpenGL resource, so it should not be copyable.
    UBO(const UBO&) = delete;
    UBO& operator=(const UBO&) = delete;

    /**
     * @brief Factory function to create and register a new UBO.
     * @param name A unique name for this buffer (for manager lookups).
     * @param mode The update frequency of this buffer.
     * @param bindingPoint The binding point to associate with this buffer.
     * @return A shared pointer to the newly created UBO.
     */
    static UBOPtr<T> Make(std::string name, UpdateMode mode, GLuint bindingPoint) {
        auto ubo_ptr = UBOPtr<T>(new UBO<T>(std::move(name), mode, bindingPoint));
        
        // Automatically register with the central manager.
        uniform::manager().registerResource(ubo_ptr);

        return ubo_ptr;
    }
};

} // namespace uniform

#endif // UBO_H