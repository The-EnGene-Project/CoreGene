#ifndef PERSPECTIVE_CAMERA_H
#define PERSPECTIVE_CAMERA_H
#pragma once

#include "3d_camera.h"
#include "../utils/observer_interfaces.h" // Needed for IObserver
#include <glm/gtc/matrix_transform.hpp>  // For glm::perspective and glm::lookAt

namespace component {

class PerspectiveCamera;
using PerspectiveCameraPtr = std::shared_ptr<PerspectiveCamera>;

/**
 * @class PerspectiveCamera
 * @brief A concrete camera that provides a 3D perspective projection.
 *
 * This class inherits from 3DCamera and implements all remaining abstract
 * methods to function as a complete camera. It also observes its own transform
 * to cache its view matrix and trigger updates for its position resource.
 */
class PerspectiveCamera : public 3DCamera, public IObserver {
private:
    // --- Members for Projection ---
    float m_fov_degrees;
    float m_near_plane;
    float m_far_plane;

    // --- Members for View Matrix & Caching ---
    ObservedTransformComponentPtr m_target;
    mutable glm::mat4 m_cached_view_matrix;
    mutable bool m_is_view_matrix_dirty;

protected:
    explicit PerspectiveCamera(
        GLuint matrices_binding_point, GLuint position_binding_point,
        float fov_degrees, float near_plane, float far_plane) 
        :
        // Pass binding points up to the parent constructors to automate resource creation.
        3DCamera(matrices_binding_point, position_binding_point),
        m_fov_degrees(fov_degrees),
        m_near_plane(near_plane),
        m_far_plane(far_plane),
        m_target(nullptr),
        m_cached_view_matrix(1.0f),
        m_is_view_matrix_dirty(true)
    {
        // A camera must observe its own transform to know when it has moved.
        this->addObserver(this);
    }

public:
    /**
     * @brief Static factory method for creating a PerspectiveCamera.
     * @param matrices_binding_point The binding point for the CameraMatrices UBO.
     * @param position_binding_point The binding point for the CameraPosition UBO.
     */
    static PerspectiveCameraPtr Make(
        GLuint matrices_binding_point, GLuint position_binding_point,
        float fov_degrees = 45.0f, float near_plane = 0.1f, float far_plane = 100.0f) 
    {
        return PerspectiveCameraPtr(
            new PerspectiveCamera(matrices_binding_point, position_binding_point, fov_degrees, near_plane, far_plane)
        );
    }

    ~PerspectiveCamera() {
        setTarget(nullptr);
        this->removeObserver(this);
    }

    // --- Implementation of Inherited Pure Virtuals ---

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
            glm::vec3 forward_vector = -glm::vec3(eye_world_transform[2]);
            target_position = eye_position + forward_vector;
        }

        m_cached_view_matrix = glm::lookAt(eye_position, target_position, up_vector);
        m_is_view_matrix_dirty = false;
        return m_cached_view_matrix;
    }

    glm::mat4 getProjectionMatrix() const override {
        return glm::perspective(
            glm::radians(m_fov_degrees),
            m_aspect_ratio, // Inherited from Camera base class
            m_near_plane,
            m_far_plane
        );
    }

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

        // Trigger the ON_DEMAND update for the position resource.
        // The check for m_position_resource is implicit, as this method won't
        // be called unless the object (and its members) have been constructed.
        uniform::manager().applyShaderResource("CameraPosition");
    }

    // --- Getters and Setters ---
    void setFov(float fov_degrees) { m_fov_degrees = fov_degrees; }
    // ... other getters/setters ...

    // --- Type Information ---
    const char* getTypeName() const override { return "PerspectiveCamera"; }
    static const char* getTypeNameStatic() { return "PerspectiveCamera"; }
};

} // namespace component

#endif // PERSPECTIVE_CAMERA_H
