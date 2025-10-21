#ifndef ORTHOGRAPHIC_CAMERA_H
#define ORTHOGRAPHIC_CAMERA_H
#pragma once

#include "camera_3d.h"
#include "../utils/observer_interfaces.h" // For IObserver
#include "../gl_base/uniforms/manager.h"   // For uniform::manager()
#include <glm/gtc/matrix_transform.hpp>

namespace component {

// Forward-declare for the smart pointer
class OrthographicCamera;
using OrthographicCameraPtr = std::shared_ptr<OrthographicCamera>;

/**
 * @class OrthographicCamera
 * @brief A camera that provides an orthographic projection.
 *
 * This camera removes all perspective distortion, making it ideal for 2D rendering,
 * UI elements, or isometric 3D views. It inherits from Camera3D to ensure it
 * can still provide its world position for effects that might require it.
 */
class OrthographicCamera : public Camera3D, public IObserver {
protected:
    // --- Members for Projection ---
    float m_left, m_right, m_bottom, m_top;
    float m_near_plane, m_far_plane;

    // --- Members for View Matrix & Caching ---
    ObservedTransformComponentPtr m_target;
    mutable glm::mat4 m_cached_view_matrix;
    mutable bool m_is_view_matrix_dirty;

    /**
     * @brief Protected constructor to create an orthographic camera.
     * @param matrices_binding_point The UBO binding point for CameraMatrices.
     * @param position_binding_point The UBO binding point for CameraPosition.
     */
    explicit OrthographicCamera(GLuint matrices_binding_point, GLuint position_binding_point)
        : Camera3D(matrices_binding_point, position_binding_point),
          m_left(-10.0f), m_right(10.0f), 
          m_bottom(-10.0f), m_top(10.0f),
          m_near_plane(0.1f), m_far_plane(100.0f),
          m_target(nullptr),
          m_cached_view_matrix(1.0f),
          m_is_view_matrix_dirty(true)
    {
        // A camera must observe its own transform to know when it has moved.
        this->addObserver(this);
    }

public:
    virtual ~OrthographicCamera() {
        setTarget(nullptr); // Clean up observer connection to target
        this->removeObserver(this); // Clean up observer connection to self
    }

    /**
     * @brief Factory function to create a new OrthographicCamera.
     * @param matrices_binding_point The UBO binding point for CameraMatrices. Defaults to 0.
     * @param position_binding_point The UBO binding point for CameraPosition. Defaults to 1.
     * @return A shared pointer to the newly created OrthographicCamera.
     */
    static OrthographicCameraPtr Make(GLuint matrices_binding_point = 0, GLuint position_binding_point = 1) {
        return std::shared_ptr<OrthographicCamera>(new OrthographicCamera(matrices_binding_point, position_binding_point));
    }

    // --- Configuration ---

    /**
     * @brief Sets the orthographic projection volume.
     * @param left The coordinate for the left vertical clipping plane.
     * @param right The coordinate for the right vertical clipping plane.
     * @param bottom The coordinate for the bottom horizontal clipping plane.
     * @param top The coordinate for the top horizontal clipping plane.
     * @param near_plane The distance to the near clipping plane.
     * @param far_plane The distance to the far clipping plane.
     */
    void setProjection(float left, float right, float bottom, float top, float near_plane = 0.1f, float far_plane = 100.0f) {
        m_left = left;
        m_right = right;
        m_bottom = bottom;
        m_top = top;
        m_near_plane = near_plane;
        m_far_plane = far_plane;
        // Note: Changing projection does not dirty the view matrix.
    }

    // --- Overridden Interface ---

    /**
     * @brief Calculates and returns the orthographic projection matrix.
     */
    glm::mat4 getProjectionMatrix() const override {
        return glm::ortho(m_left, m_right, m_bottom, m_top, m_near_plane, m_far_plane);
    }

    /**
     * @brief Calculates and returns the view matrix based on the camera's transform.
     * If a target is set, the camera will look at the target's position.
     */
    glm::mat4 getViewMatrix() override {
        if (!m_is_view_matrix_dirty) {
            return m_cached_view_matrix;
        }

        const glm::mat4& eye_world_transform = this->getWorldTransform();
        glm::vec3 eye_position = glm::vec3(eye_world_transform[3]);
        glm::vec3 up_vector = glm::vec3(eye_world_transform[1]);
        glm::vec3 target_position;

        if (m_target) {
            const glm::mat4& target_world_transform = m_target->getWorldTransform();
            target_position = glm::vec3(target_world_transform[3]);
        } else {
            // If no target, look along the local -Z axis (forward).
            glm::vec3 forward_vector = -glm::vec3(eye_world_transform[2]);
            target_position = eye_position + forward_vector;
        }

        uniform::manager().applyShaderResource("CameraPosition");

        m_cached_view_matrix = glm::lookAt(eye_position, target_position, up_vector);
        m_is_view_matrix_dirty = false;
        return m_cached_view_matrix;
    }

    /**
     * @brief Sets an optional target for the camera to follow.
     */
    void setTarget(ObservedTransformComponentPtr target) override {
        if (m_target) {
            m_target->removeObserver(this);
        }
        m_target = target;
        if (m_target) {
            m_target->addObserver(this);
        }
        m_is_view_matrix_dirty = true;
    }

    /**
     * @brief Retrieves the camera's world-space position directly from its transform.
     */
    glm::vec3 getWorldPosition() override {
        // getWorldTransform() calculates and caches the latest world matrix.
        // We can then extract the position from the final column.
        return glm::vec3(this->getWorldTransform()[3]);
    }

    // --- IObserver Interface Implementation ---

    /**
     * @brief Called when this camera's transform or its target's transform changes.
     */
    void onNotify(const ISubject* subject) override {
        // Mark the view matrix cache as dirty for the next render frame.
        m_is_view_matrix_dirty = true;
    }

    // --- Type Information ---
    const char* getTypeName() const override { return "OrthographicCamera"; }
    static const char* getTypeNameStatic() { return "OrthographicCamera"; }
};

} // namespace component

#endif // ORTHOGRAPHIC_CAMERA_H
