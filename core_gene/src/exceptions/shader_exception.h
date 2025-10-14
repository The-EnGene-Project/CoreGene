#ifndef SHADER_EXCEPTION_H
#define SHADER_EXCEPTION_H
#pragma once

#include "base_exception.h"
#include <string>

namespace exception {

/**
 * @class ShaderException
 * @brief Specific exception for errors during shader compilation, linking, or configuration.
 */
class ShaderException : public exception::EnGeneException {
public:
    explicit ShaderException(const std::string& message)
        : EnGeneException(message) {}
};

} // namespace exception

#endif // SHADER_EXCEPTION_H