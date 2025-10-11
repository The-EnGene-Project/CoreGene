#ifndef BASE_EXCEPTION_H
#define BASE_EXCEPTION_H
#pragma once

#include <stdexcept>

namespace exception {

/**
 * @class EnGeneException
 * @brief A base class for all exceptions thrown by the EnGene engine.
 *
 * Inheriting from this base allows for catching all engine-specific
 * exceptions with a single catch block: catch(const exception::EnGeneException& e).
 */
class EnGeneException : public std::runtime_error {
public:
    // Inherit the constructors from the base class
    using std::runtime_error::runtime_error;
};

} // namespace exception

#endif // BASE_EXCEPTION_H