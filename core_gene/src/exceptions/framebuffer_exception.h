#ifndef FRAMEBUFFER_EXCEPTION_H
#define FRAMEBUFFER_EXCEPTION_H
#pragma once

#include "base_exception.h"
#include <string>

namespace exception {

/**
 * @class FramebufferException
 * @brief Specific exception for errors during framebuffer creation, attachment, or validation.
 * 
 * This exception is thrown when:
 * - Framebuffer object creation fails
 * - Attachment creation or configuration fails
 * - Framebuffer completeness validation fails
 * - Invalid attachment points or formats are specified
 * - Texture retrieval fails (invalid name or renderbuffer access)
 */
class FramebufferException : public exception::EnGeneException {
public:
    explicit FramebufferException(const std::string& message)
        : EnGeneException("Framebuffer Error: " + message) {}
};

} // namespace exception

#endif // FRAMEBUFFER_EXCEPTION_H
