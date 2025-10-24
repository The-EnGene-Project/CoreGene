#ifndef I_SHADER_H
#define I_SHADER_H
#pragma once

#include "gl_includes.h" // Or wherever GLuint comes from
#include <memory>

namespace shader {

/**
 * @class IShader
 * @brief An abstract interface representing any object that
 * has an OpenGL Shader Program ID.
 */
class IShader {
public:
    virtual ~IShader() = default;
    virtual GLuint GetShaderID() const = 0;
};

using IShaderPtr = std::shared_ptr<IShader>;

} // namespace shader
#endif