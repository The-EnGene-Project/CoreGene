#ifndef PENDING_UNIFORM_COMMAND_H
#define PENDING_UNIFORM_COMMAND_H
#pragma once

#include <variant>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../gl_includes.h"

namespace uniform {

// A variant type to hold any of the supported uniform data types.
using UniformData = std::variant<
    int, float,
    glm::vec2, glm::vec3, glm::vec4,
    glm::mat3, glm::mat4
>;

/**
 * @struct PendingUniformCommand
 * @brief Represents a deferred uniform update command.
 *
 * This struct is used by the Shader class to queue uniform updates that are
 * set while the shader is not active. The command is executed when the shader
 * is next activated.
 */
struct PendingUniformCommand {
    GLint location;
    UniformData data;

    /**
     * @brief Executes the pending uniform command, sending the data to OpenGL.
     */
    void Execute() const {
        std::visit([this](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, int>) {
                glUniform1i(location, arg);
            } else if constexpr (std::is_same_v<T, float>) {
                glUniform1f(location, arg);
            } else if constexpr (std::is_same_v<T, glm::vec2>) {
                glUniform2fv(location, 1, glm::value_ptr(arg));
            } else if constexpr (std::is_same_v<T, glm::vec3>) {
                glUniform3fv(location, 1, glm::value_ptr(arg));
            } else if constexpr (std::is_same_v<T, glm::vec4>) {
                glUniform4fv(location, 1, glm::value_ptr(arg));
            } else if constexpr (std::is_same_v<T, glm::mat3>) {
                glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(arg));
            } else if constexpr (std::is_same_v<T, glm::mat4>) {
                glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(arg));
            }
        }, data);
    }
};

} // namespace uniform

#endif // PENDING_UNIFORM_COMMAND_H
