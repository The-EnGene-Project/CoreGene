#ifndef CAMERA_H
#define CAMERA_H
#pragma once

#include "../../components/observed_transform_component.h"
#include "../../gl_base/uniforms/ubo.h"
#include "../../gl_base/shader.h" // Include for ShaderPtr
#include <glm/glm.hpp>
#include <functional>
#include <vector>

namespace component {

// Forward-declare for the smart pointer
class Camera;
using CameraPtr = std::shared_ptr<Camera>;

// --- Data Struct for the Camera UBO ---

/**
 * @struct CameraMatrices
 * @brief A POD struct matching the layout of the global CameraMatrices UBO block.
 */
struct CameraMatrices {
    glm::mat4 view;
    glm::mat4 projection;
};

/**
 * @class Camera
 * @brief Abstract base class for all camera components.
 *
 * This class provides the core camera interface and functionality, including
 * aspect ratio management and a static global UBO for camera matrices.
 */
class Camera : public ObservedTransformComponent {
private:
    // Static global UBO shared by all cameras
    static uniform::UBOPtr<CameraMatrices> s_matrices_ubo;
    static bool s_ubo_initialized;

protected:
    float m_aspect_ratio;

    /**
     * @brief Protected constructor for camera components.
     * @param priority The update priority for the camera's transform.
     */
    explicit Camera(unsigned int priority = static_cast<unsigned int>(ComponentPriority::CAMERA))
        : ObservedTransformComponent(transform::Transform::Make(), priority, 
            static_cast<unsigned int>(ComponentPriority::CAMERA), 
            static_cast<unsigned int>(ComponentPriority::GEOMETRY)),
          m_aspect_ratio(16.0f / 9.0f) // Default aspect ratio
    {
        // Ensure the static UBO is initialized
        initializeStaticUBO();
    }

    /**
     * @brief Initializes the static UBO if not already initialized.
     */
    static void initializeStaticUBO() {
        if (!s_ubo_initialized) {
            s_matrices_ubo = uniform::UBO<CameraMatrices>::Make(
                "CameraMatrices",
                uniform::UpdateMode::PER_FRAME,
                0 // Binding point 0
            );
            s_ubo_initialized = true;
        }
    }

public:
    virtual ~Camera() = default;

    // --- Concrete Methods for Resource Management & Aspect Ratio ---

    virtual void setAspectRatio(float ratio) { m_aspect_ratio = ratio; }
    virtual float getAspectRatio() const { return m_aspect_ratio; }
    
    /**
     * @brief Returns a function that provides this camera's matrices.
     * This is used by the scene graph to update the static UBO when this camera becomes active.
     * @return A std::function that returns a CameraMatrices struct.
     */
    virtual std::function<CameraMatrices()> getMatricesProvider() {
        return [this]() -> CameraMatrices {
            CameraMatrices matrices;
            matrices.view = this->getViewMatrix();
            matrices.projection = this->getProjectionMatrix();
            return matrices;
        };
    }

    /**
     * @brief Updates the static UBO to use this camera's provider.
     * Called by the scene graph when this camera becomes active.
     */
    void activateAsGlobalCamera() {
        initializeStaticUBO();
        if (s_matrices_ubo) {
            s_matrices_ubo->setProvider(getMatricesProvider());
        }
    }

    // --- Shader Binding Utilities ---

    /**
     * @brief Binds the global CameraMatrices UBO to a specific shader.
     * @param shader The shader to bind to.
     */
    static void bindToShader(shader::ShaderPtr shader) {
        initializeStaticUBO();
        if (shader) {
            shader->addResourceBlockToBind("CameraMatrices");
        }
    }

    /**
     * @brief Binds the global CameraMatrices UBO to a collection of shaders.
     * @param shaders A vector of shaders to bind to.
     */
    static void bindToShaders(const std::vector<shader::ShaderPtr>& shaders) {
        for (const auto& shader : shaders) {
            Camera::bindToShader(shader);
        }
    }

    // --- Pure Virtual Interface for Concrete Cameras ---

    /**
     * @brief Calculates and returns the view matrix (world-to-camera space).
     * @return A 4x4 matrix representing the camera's view transformation.
     */
    virtual glm::mat4 getViewMatrix() = 0; // Made non-const to allow for caching

    /**
     * @brief Calculates and returns the projection matrix (camera-to-clip space).
     * @return A 4x4 matrix representing the camera's projection.
     */
    virtual glm::mat4 getProjectionMatrix() const = 0;

    /**
     * @brief Sets the target for the camera to look at.
     * @param target A shared pointer to an ObservedTransformComponent to follow.
     */
    virtual void setTarget(ObservedTransformComponentPtr target) = 0;

    /**
     * @brief Gets the current target that the camera is looking at.
     * @return A shared pointer to the target component, or nullptr if no target is set.
     */
    virtual ObservedTransformComponentPtr getTarget() const = 0;

    // --- Type Information ---
    const char* getTypeName() const override { return "Camera"; }
    static const char* getTypeNameStatic() { return "Camera"; }
};

// --- Static Member Definitions ---
// Note: Header-only library, so we use inline to avoid ODR violations
inline uniform::UBOPtr<CameraMatrices> Camera::s_matrices_ubo = nullptr;
inline bool Camera::s_ubo_initialized = false;

} // namespace component

#endif // CAMERA_H
