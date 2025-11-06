#ifndef TEXTURE_EXCEPTION_H
#define TEXTURE_EXCEPTION_H
#pragma once

#include "base_exception.h"

namespace exception {

/**
 * @class TextureException
 * @brief Exception class for texture loading and configuration errors.
 * 
 * This exception is thrown when texture operations fail, such as:
 * - File not found
 * - Dimension mismatch in cubemap faces
 * - Invalid image format
 * - GPU upload failures
 */
class TextureException : public EnGeneException {
public:
    // Inherit the constructors from the base class
    using EnGeneException::EnGeneException;
};

} // namespace exception

#endif // TEXTURE_EXCEPTION_H
