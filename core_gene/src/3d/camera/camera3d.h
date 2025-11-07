#ifndef CAMERA_3D_H
#define CAMERA_3D_H
#pragma once

#include "camera.h"

namespace component {

// Forward-declare for the smart pointer
class Camera3D;
using Camera3DPtr = std::shared_ptr<Camera3D>;

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
 * @class Camera3D
 * @brief An abstract intermediate class for cameras operating in 3D space.
 *
 * This class extends the base Camera by adding support for a world-space
 * position resource, which is useful for lighting calculations.
 */
class Camera3D : public Camera {
private:
    // Static global UBO for camera position shared by all 3D cameras
    static uniform::UBOPtr<CameraPosition> s_position_ubo;
    static bool s_position_ubo_initialized;

protected:
    /**
     * @brief Protected constructor that chains to the base.
     * @param priority The update priority for the camera's transform.
     */
    explicit Camera3D(unsigned int priority = static_cast<unsigned int>(ComponentPriority::CAMERA))
        : Camera(priority)
    {
        // Ensure the static position UBO is initialized
        initializeStaticPositionUBO();
    }

    /**
     * @brief Initializes the static position UBO if not already initialized.
     */
    static void initializeStaticPositionUBO() {
        if (!s_position_ubo_initialized) {
            s_position_ubo = uniform::UBO<CameraPosition>::Make(
                "CameraPosition",
                uniform::UpdateMode::PER_FRAME,
                1 // Binding point 1
            );
            s_position_ubo_initialized = true;
        }
    }

public:
    virtual ~Camera3D() = default;

    /**
     * @brief Returns a function that provides this camera's world position.
     * @return A std::function that returns a CameraPosition struct.
     */
    virtual std::function<CameraPosition()> getPositionProvider() {
        return [this]() -> CameraPosition {
            return {
                glm::vec4(this->getWorldPosition(), 1.0f)
            };
        };
    }

    /**
     * @brief Updates the static position UBO to use this camera's provider.
     * Called when this camera becomes active.
     */
    void activateAsGlobalCamera3D() {
        initializeStaticPositionUBO();
        if (s_position_ubo) {
            s_position_ubo->setProvider(getPositionProvider());
        }
        // Also activate the base camera matrices
        Camera::activateAsGlobalCamera();
    }

    // --- Shader Binding Utilities ---
    
    /**
     * @brief Binds both camera UBOs (matrices and position) to a specific shader.
     * @param shader The shader to bind to.
     */
    static void bindToShader(shader::ShaderPtr shader) {
        initializeStaticPositionUBO();
        Camera::bindToShader(shader); // Bind matrices UBO
        if (shader) {
            shader->addResourceBlockToBind("CameraPosition");
        }
    }

    /**
     * @brief Binds both camera UBOs to a collection of shaders.
     * @param shaders A vector of shaders to bind to.
     */
    static void bindToShaders(const std::vector<shader::ShaderPtr>& shaders) {
        for (const auto& shader : shaders) {
            Camera3D::bindToShader(shader);
        }
    }

    // --- New Pure Virtual for Concrete 3D Cameras ---
    
    /**
     * @brief Retrieves the camera's world-space position vector.
     * @return A glm::vec3 representing the position.
     */
    virtual glm::vec3 getWorldPosition() = 0;
};

// --- Static Member Definitions ---
inline uniform::UBOPtr<CameraPosition> Camera3D::s_position_ubo = nullptr;
inline bool Camera3D::s_position_ubo_initialized = false;

} // namespace component

#endif // CAMERA_3D_H
