#ifndef CAMERA_H
#define CAMERA_H
#pragma once

#include "observed_transform_component.h"
#include "../gl_base/uniforms/ubo.h"
#include "../gl_base/shader.h" // Include for ShaderPtr
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
 * aspect ratio management and the automatic creation and configuration of a
 * UBO for view and projection matrices.
 */
class Camera : public ObservedTransformComponent {
protected:
    float m_aspect_ratio;
    uniform::UBOPtr<CameraMatrices> m_matrices_resource;

    /**
     * @brief Protected constructor that automatically creates and configures the matrices resource.
     * @param matrices_binding_point The binding point for the CameraMatrices UBO.
     * @param priority The update priority for the camera's transform.
     */
    explicit Camera(GLuint matrices_binding_point, unsigned int priority = ComponentPriority::CAMERA)
        : ObservedTransformComponent(transform::Transform::Make(), priority, 
            static_cast<unsigned int>(ComponentPriority::CAMERA), 
            static_cast<unsigned int>(ComponentPriority::GEOMETRY)),
          m_aspect_ratio(16.0f / 9.0f) // Default aspect ratio
    {
        // Automatically create and store the UBO resource.
        m_matrices_resource = uniform::UBO<CameraMatrices>::Make(
            "CameraMatrices", 
            uniform::UpdateMode::PER_FRAME, 
            matrices_binding_point
        );

        // Immediately configure the resource with its data provider.
        m_matrices_resource->setProvider(this->getMatricesProvider());
    }

public:
    virtual ~Camera() = default;

    // --- Concrete Methods for Resource Management & Aspect Ratio ---

    virtual void setAspectRatio(float ratio) { m_aspect_ratio = ratio; }
    virtual float getAspectRatio() const { return m_aspect_ratio; }
    
    /**
     * @brief Returns a function that provides the camera's matrices.
     * This is used by the UBO to get its data each frame.
     * @return A std::function that returns a CameraMatrices struct.
     */
    virtual std::function<CameraMatrices()> getMatricesProvider() {
        return [this]() -> CameraMatrices {
            return {
                this->getViewMatrix(),
                this->getProjectionMatrix()
            };
        };
    }

    // --- NEW: Shader Binding Utilities ---

    /**
     * @brief Binds the camera's UBO resource block to a specific shader.
     * @param shader The shader to bind to.
     */
    virtual void bindToShader(shader::ShaderPtr shader) const {
        if (shader) {
            shader->addResourceBlockToBind("CameraMatrices");
        }
    }

    /**
     * @brief Binds the camera's UBO resource block to a collection of shaders.
     * @param shaders A vector of shaders to bind to.
     */
    virtual void bindToShaders(const std::vector<shader::ShaderPtr>& shaders) const {
        for (const auto& shader : shaders) {
            this->bindToShader(shader);
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

    // --- Type Information ---
    const char* getTypeName() const override { return "Camera"; }
    static const char* getTypeNameStatic() { return "Camera"; }
};

} // namespace component

#endif // CAMERA_H
