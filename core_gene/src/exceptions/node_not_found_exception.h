#ifndef NODE_NOT_FOUND_EXCEPTION_H
#define NODE_NOT_FOUND_EXCEPTION_H
#pragma once

#include "base_exception.h"
#include <string>

namespace exception {

/**
 * @class NodeNotFoundException
 * @brief Exception thrown when a scene graph lookup fails for a given node name.
 */
class NodeNotFoundException : public EnGeneException {
public:
    explicit NodeNotFoundException(const std::string& name) 
        : EnGeneException("Node not found: '" + name + "'") {}
};

} // namespace exception

#endif // NODE_NOT_FOUND_EXCEPTION_H