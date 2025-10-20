#ifndef THREED_CAMERA_H
#define THREED_CAMERA_H
#pragma once

#include "camera.h"

namespace component {

// --- Data Struct for the Camera Position UBO ---
/**
 * @struct CameraPosition
 * @brief A POD struct matching the layout of the global CameraPosition UBO block.
 */
struct CameraPosition {
    // Using vec4 for std140 memory alignment. The w-component can be unused.
    glm::vec4 worldPosition;
};

/**
 * @class 3DCamera
 * @brief An abstract intermediate class for cameras operating in 3D space.
 *
 * This class extends the base Camera by adding support for a world-space
 * position resource, which is useful for lighting calculations.
 */
class 3DCamera : public Camera {
protected:
    uniform::UBOPtr<CameraPosition> m_position_resource;

    /**
     * @brief Protected constructor that chains to the base and adds the position resource.
     * @param matrices_binding_point The binding point for the CameraMatrices UBO.
     * @param position_binding_point The binding point for the CameraPosition UBO.
     * @param priority The update priority for the camera's transform.
     */
    explicit 3DCamera(GLuint matrices_binding_point, GLuint position_binding_point, 
                      unsigned int priority = ComponentPriority::CAMERA)
        : Camera(matrices_binding_point, priority)
    {
        // Automatically create and store the ON_DEMAND position resource.
        m_position_resource = uniform::UBO<CameraPosition>::Make(
            "CameraPosition",
            uniform::UpdateMode::ON_DEMAND,
            position_binding_point
        );

        // Immediately configure it with its data provider.
        m_position_resource->setProvider(this->getPositionProvider());
    }

public:
    virtual ~3DCamera() = default;

    /**
     * @brief Returns a function that provides the camera's world position.
     * @return A std::function that returns a CameraPosition struct.
     */
    virtual std::function<CameraPosition()> getPositionProvider() {
        return [this]() -> CameraPosition {
            return {
                glm::vec4(this->getWorldPosition(), 1.0f)
            };
        };
    }

    // --- New Pure Virtual for Concrete 3D Cameras ---
    
    /**
     * @brief Retrieves the camera's world-space position vector.
     * @return A glm::vec3 representing the position.
     */
    virtual glm::vec3 getWorldPosition() = 0;
};

} // namespace component

#endif // THREED_CAMERA_H
