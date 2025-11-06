#ifndef PENDING_UNIFORM_COMMAND_H
#define PENDING_UNIFORM_COMMAND_H
#pragma once

#include <variant>
#include <string>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../gl_includes.h"

namespace uniform {

// Note: Sampler types are defined in uniform.h
// No forward declarations needed since uniform.h is included before this file in shader.h

// A variant type to hold any of the supported uniform data types.
// This is used for queuing uniform updates when a shader is not active.
using UniformData = std::variant<
    int, float,
    glm::vec2, glm::vec3, glm::vec4,
    glm::mat3, glm::mat4,
    detail::Sampler     // Generic sampler for all sampler types
>;

/**
 * @struct PendingUniformCommand
 * @brief Represents a deferred uniform update command.
 *
 * This struct is used by the Shader class to queue uniform updates that are
 * set while the shader is not active. The command is executed when the shader
 * is next activated. The location is retrieved just-in-time when Execute() is called.
 */
struct PendingUniformCommand {
    std::string name;
    UniformData data;

    /**
     * @brief Executes the pending uniform command, sending the data to OpenGL.
     * @param pid The OpenGL program ID to set the uniform on.
     */
    void Execute(GLuint pid) const {
        // [Suggestion 2] Get location just-in-time
        GLint location = glGetUniformLocation(pid, name.c_str());
        if (location == -1) {
            std::cerr << "Warning: Queued uniform '" << name << "' not found in shader (at flush time)." << std::endl;
            return;
        }

        std::visit([this, location](auto&& arg) {
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
            } else if constexpr (std::is_same_v<T, detail::Sampler>) {
                glUniform1i(location, arg.unit);
            }
        }, data);
    }
};

} // namespace uniform

#endif // PENDING_UNIFORM_COMMAND_H