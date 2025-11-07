#ifndef ARCBALL_CONTROLLER_H
#define ARCBALL_CONTROLLER_H
#pragma once

#include "../../gl_base/gl_includes.h"
#include "../../components/observed_transform_component.h"
#include "../../core/scene.h"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <algorithm>
#include <iostream>

namespace arcball {

// --- Mathematical Utility Functions ---

/**
 * @brief Converts spherical coordinates to cartesian position with target offset.
 * @param radius Distance from target
 * @param theta Azimuthal angle (horizontal rotation, 0 to 2π)
 * @param phi Polar angle (vertical rotation, 0 to π)
 * @param target Target point to offset from
 * @return Cartesian position in world space
 */
inline glm::vec3 sphericalToCartesian(float radius, float theta, float phi, const glm::vec3& target) {
    float x = radius * sin(phi) * cos(theta);
    float y = radius * cos(phi);
    float z = radius * sin(phi) * sin(theta);
    return target + glm::vec3(x, y, z);
}

/**
 * @brief Converts cartesian position to spherical coordinates for initialization.
 * @param position Camera position in world space
 * @param target Target point to calculate relative to
 * @param radius Output radius (distance from target)
 * @param theta Output azimuthal angle
 * @param phi Output polar angle
 */
inline void cartesianToSpherical(const glm::vec3& position, const glm::vec3& target, 
                                float& radius, float& theta, float& phi) {
    glm::vec3 relative_pos = position - target;
    radius = glm::length(relative_pos);
    
    if (radius > 0.0f) {
        phi = acos(glm::clamp(relative_pos.y / radius, -1.0f, 1.0f));
        theta = atan2(relative_pos.z, relative_pos.x);
        
        // Normalize theta to [0, 2π]
        if (theta < 0.0f) theta += 2.0f * glm::pi<float>();
    } else {
        // Default values for zero radius
        phi = glm::pi<float>() / 2.0f;
        theta = 0.0f;
    }
}

/**
 * @brief Normalizes an angle to the range [0, 2π].
 * @param angle Input angle in radians
 * @return Normalized angle in [0, 2π] range
 */
inline float normalizeAngle(float angle) {
    while (angle < 0.0f) angle += 2.0f * glm::pi<float>();
    while (angle >= 2.0f * glm::pi<float>()) angle -= 2.0f * glm::pi<float>();
    return angle;
}

/**
 * @brief Clamps polar angle to prevent gimbal lock at poles.
 * @param phi Input polar angle in radians
 * @return Clamped angle avoiding poles
 */
inline float clampPolarAngle(float phi) {
    return glm::clamp(phi, 0.01f, glm::pi<float>() - 0.01f);
}

/**
 * @brief Converts mouse delta to spherical angle changes.
 * @param dx Mouse delta X
 * @param dy Mouse delta Y
 * @param sensitivity Rotation sensitivity multiplier
 * @param theta Current azimuthal angle (will be modified)
 * @param phi Current polar angle (will be modified)
 */
inline void mouseDeltaToSphericalDelta(double dx, double dy, float sensitivity, 
                                      float& theta, float& phi) {
    theta += dx * sensitivity;  // Horizontal movement affects azimuthal angle
    phi -= dy * sensitivity;    // Vertical movement affects polar angle
    
    // Apply angle constraints
    phi = clampPolarAngle(phi);
    theta = normalizeAngle(theta);
}

/**
 * @brief Calculates screen space to world space panning translation.
 * @param dx Mouse delta X
 * @param dy Mouse delta Y
 * @param view_matrix Camera view matrix
 * @param radius Current camera distance from target
 * @param sensitivity Pan sensitivity multiplier
 * @return World space translation vector
 */
inline glm::vec3 screenToWorldPan(float dx, float dy, const glm::mat4& view_matrix, 
                                 float radius, float sensitivity) {
    // Extract camera right and up vectors from view matrix
    glm::vec3 right = glm::vec3(view_matrix[0][0], view_matrix[1][0], view_matrix[2][0]);
    glm::vec3 up = glm::vec3(view_matrix[0][1], view_matrix[1][1], view_matrix[2][1]);
    
    // Scale by distance for consistent pan speed
    float scale_factor = radius * sensitivity;
    
    return (-dx * right + dy * up) * scale_factor;
}

/**
 * @brief Calculates zoom distance change from scroll input.
 * @param scroll_delta Scroll wheel delta value
 * @param current_radius Current camera distance from target
 * @param zoom_sensitivity Zoom sensitivity multiplier
 * @param min_radius Minimum allowed radius
 * @param max_radius Maximum allowed radius
 * @return New radius value after zoom
 */
inline float calculateZoomRadius(double scroll_delta, float current_radius, float zoom_sensitivity,
                               float min_radius, float max_radius) {
    float zoom_factor = 1.0f + (scroll_delta * zoom_sensitivity * 0.1f);
    float new_radius = current_radius / zoom_factor; // Invert for intuitive zoom direction
    
    // Enforce zoom limits
    return glm::clamp(new_radius, min_radius, max_radius);
}

/**
 * @class ArcBallController
 * @brief Core controller that manages camera state and performs mathematical transformations
 * for intuitive 3D camera manipulation through mouse interactions.
 * 
 * The controller translates mouse input into camera transformations using spherical coordinate
 * mathematics to provide smooth orbiting, zooming, and panning operations around a target point.
 */
class ArcBallController : public std::enable_shared_from_this<ArcBallController> {
public:
    // --- Constructor and Destructor ---
    
    /**
     * @brief Constructs an ArcBall controller with default settings.
     * Initializes with origin target, default radius, and standard sensitivity values.
     * @param initialize_from_camera If true, initializes state from current camera position
     */
    ArcBallController(bool initialize_from_camera = false)
        : m_target(0.0f, 0.0f, 0.0f)
        , m_target_component(nullptr)
        , m_radius(5.0f)
        , m_theta(0.0f)
        , m_phi(glm::pi<float>() / 2.0f)
        , m_rotation_sensitivity(0.005f)
        , m_zoom_sensitivity(1.0f)
        , m_pan_sensitivity(0.001f)
        , m_min_radius(0.1f)
        , m_max_radius(100.0f)
        , m_is_orbiting(false)
        , m_is_panning(false)
        , m_last_mouse_x(0.0)
        , m_last_mouse_y(0.0)
    {
        if (initialize_from_camera) {
            // Initialize from current camera state
            initializeFromCamera();
        } else {
            // Initialize camera position based on default spherical coordinates
            updateCameraPosition();
        }
    }
    
    /**
     * @brief Virtual destructor for proper cleanup.
     */
    virtual ~ArcBallController() = default;
    
    /**
     * @brief Static factory method to create an ArcBall controller initialized from current camera state.
     * @return Shared pointer to a new ArcBallController initialized from the active camera
     */
    static std::shared_ptr<ArcBallController> CreateFromCamera() {
        return std::shared_ptr<ArcBallController>(new ArcBallController(true));
    }
    
    /**
     * @brief Static factory method to create an ArcBall controller from a specific camera node.
     * @param camera_node_name Name of the scene node containing the camera
     * @return Shared pointer to a new ArcBallController initialized from the specified camera
     */
    static std::shared_ptr<ArcBallController> CreateFromCameraNode(const std::string& camera_node_name) {
        auto controller = std::make_shared<ArcBallController>(false);
        controller->initializeFromCameraNode(camera_node_name);
        return controller;
    }
    
    /**
     * @brief Static factory method to create an ArcBall controller from a specific camera node.
     * @param camera_node Shared pointer to the scene node containing the camera
     * @return Shared pointer to a new ArcBallController initialized from the specified camera
     */
    static std::shared_ptr<ArcBallController> CreateFromCameraNode(scene::SceneNodePtr camera_node) {
        auto controller = std::make_shared<ArcBallController>(false);
        controller->initializeFromCameraNode(camera_node);
        return controller;
    }

    // --- Configuration Methods ---
    
    /**
     * @brief Sets the target point for camera orbiting.
     * @param target World space position to orbit around
     */
    void setTarget(const glm::vec3& target) {
        m_target = target;
        m_target_component = nullptr; // Clear component target when setting fixed target
        updateCameraPosition();
    }
    
    /**
     * @brief Sets the target to follow a transform component.
     * @param target Shared pointer to transform component to track
     */
    void setTarget(component::ObservedTransformComponentPtr target) {
        m_target_component = target;
        if (target) {
            // Update target position from component
            const glm::mat4& world_transform = target->getCachedWorldTransform();
            m_target = glm::vec3(world_transform[3]); // Extract translation from transform matrix
        }
        updateCameraPosition();
    }
    
    /**
     * @brief Sets the camera distance from the target.
     * @param radius Distance in world units
     */
    void setRadius(float radius) {
        m_radius = glm::clamp(radius, m_min_radius, m_max_radius);
        updateCameraPosition();
    }
    
    /**
     * @brief Configures input sensitivity for all operations.
     * @param rotation Sensitivity for orbit operations (default: 0.005f)
     * @param zoom Sensitivity for zoom operations (default: 1.0f)  
     * @param pan Sensitivity for pan operations (default: 0.001f)
     */
    void setSensitivity(float rotation, float zoom, float pan) {
        m_rotation_sensitivity = rotation;
        m_zoom_sensitivity = zoom;
        m_pan_sensitivity = pan;
    }
    
    /**
     * @brief Sets minimum and maximum zoom distance constraints.
     * @param min_radius Minimum allowed distance from target
     * @param max_radius Maximum allowed distance from target
     */
    void setZoomLimits(float min_radius, float max_radius) {
        m_min_radius = std::max(0.01f, min_radius); // Ensure minimum is positive
        m_max_radius = std::max(m_min_radius, max_radius); // Ensure max >= min
        
        // Clamp current radius to new limits
        m_radius = glm::clamp(m_radius, m_min_radius, m_max_radius);
        updateCameraPosition();
    }

    // --- Input Handling Methods ---
    
    /**
     * @brief Begins orbit operation at the given mouse position.
     * @param mouse_x Current mouse X coordinate
     * @param mouse_y Current mouse Y coordinate
     */
    void startOrbit(double mouse_x, double mouse_y) {
        m_is_orbiting = true;
        m_last_mouse_x = mouse_x;
        m_last_mouse_y = mouse_y;
    }
    
    /**
     * @brief Updates orbit operation with new mouse position.
     * @param mouse_x Current mouse X coordinate
     * @param mouse_y Current mouse Y coordinate
     */
    void updateOrbit(double mouse_x, double mouse_y) {
        if (!m_is_orbiting) return;
        
        double dx = mouse_x - m_last_mouse_x;
        double dy = mouse_y - m_last_mouse_y;
        
        // Update spherical coordinates using utility function
        mouseDeltaToSphericalDelta(dx, dy, m_rotation_sensitivity, m_theta, m_phi);
        
        updateCameraPosition();
        
        m_last_mouse_x = mouse_x;
        m_last_mouse_y = mouse_y;
    }
    
    /**
     * @brief Ends the current orbit operation.
     */
    void endOrbit() {
        m_is_orbiting = false;
    }
    
    /**
     * @brief Begins pan operation at the given mouse position.
     * @param mouse_x Current mouse X coordinate
     * @param mouse_y Current mouse Y coordinate
     */
    void startPan(double mouse_x, double mouse_y) {
        m_is_panning = true;
        m_last_mouse_x = mouse_x;
        m_last_mouse_y = mouse_y;
    }
    
    /**
     * @brief Updates pan operation with new mouse position.
     * @param mouse_x Current mouse X coordinate
     * @param mouse_y Current mouse Y coordinate
     */
    void updatePan(double mouse_x, double mouse_y) {
        if (!m_is_panning) return;
        
        double dx = mouse_x - m_last_mouse_x;
        double dy = mouse_y - m_last_mouse_y;
        
        // Get view matrix for pan calculation
        auto camera = scene::graph()->getActiveCamera();
        if (!camera) return;
        
        glm::mat4 view_matrix = camera->getViewMatrix();
        
        // Convert screen space delta to world space translation using utility function
        glm::vec3 pan_delta = screenToWorldPan(dx, dy, view_matrix, m_radius, m_pan_sensitivity);
        
        // Update both target and camera position
        m_target += pan_delta;
        updateCameraPosition();
        
        m_last_mouse_x = mouse_x;
        m_last_mouse_y = mouse_y;
    }
    
    /**
     * @brief Ends the current pan operation.
     */
    void endPan() {
        m_is_panning = false;
    }
    
    /**
     * @brief Performs zoom operation based on scroll input.
     * @param scroll_delta Scroll wheel delta value
     */
    void zoom(double scroll_delta) {
        // Calculate new radius using utility function
        m_radius = calculateZoomRadius(scroll_delta, m_radius, m_zoom_sensitivity, 
                                     m_min_radius, m_max_radius);
        
        updateCameraPosition();
    }

    // --- Utility Methods ---
    
    /**
     * @brief Resets camera to default position and orientation.
     * Clears any active input states and restores default spherical coordinates.
     */
    void reset() {
        // Reset to default spherical coordinates
        m_theta = 0.0f;
        m_phi = glm::pi<float>() / 2.0f;
        m_radius = 5.0f;
        m_target = glm::vec3(0.0f, 0.0f, 0.0f);
        m_target_component = nullptr;
        
        // Clear input states
        m_is_orbiting = false;
        m_is_panning = false;
        
        updateCameraPosition();
    }
    
    /**
     * @brief Gets the current target position.
     * @return Current target point in world space
     */
    glm::vec3 getTarget() const {
        if (m_target_component) {
            // Return current position from component if available
            const glm::mat4& world_transform = m_target_component->getCachedWorldTransform();
            return glm::vec3(world_transform[3]);
        }
        return m_target;
    }
    
    /**
     * @brief Checks if the camera has a target set and uses it as the arcball center.
     * This method should be called to synchronize the arcball target with the camera's target.
     * @return True if the camera target was used, false if no camera target was found
     */
    bool syncWithCameraTarget() {
        auto camera = scene::graph()->getActiveCamera();
        if (!camera) return false;
        
        auto camera_target = camera->getTarget();
        if (camera_target) {
            // Use the camera's target as the arcball center
            setTarget(camera_target);
            return true;
        }
        return false;
    }
    
    /**
     * @brief Gets the current camera distance from target.
     * @return Current radius value
     */
    float getRadius() const {
        return m_radius;
    }
    
    // --- Input Handler Integration ---
    
    /**
     * @brief Attaches arcball input handling to an input handler.
     * This registers the arcball callbacks with the target handler, allowing
     * the arcball to work through that handler's input system.
     * @param target_handler Reference to the input handler to attach to
     */
    void attachTo(input::InputHandler& target_handler) {
        // Create a shared pointer from 'this' for the callbacks
        // Note: This assumes the controller is managed by a shared_ptr
        auto self = shared_from_this();
        
        // Register arcball callbacks with the target handler
        target_handler.registerCallback<input::InputType::MOUSE_BUTTON>(
            [self](GLFWwindow* window, int button, int action, int mods) {
                if (!self) return;
                
                double mouse_x, mouse_y;
                glfwGetCursorPos(window, &mouse_x, &mouse_y);
                
                if (button == GLFW_MOUSE_BUTTON_LEFT) {
                    if (action == GLFW_PRESS) {
                        self->startOrbit(mouse_x, mouse_y);
                    } else if (action == GLFW_RELEASE) {
                        self->endOrbit();
                    }
                } else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
                    if (action == GLFW_PRESS) {
                        self->startPan(mouse_x, mouse_y);
                    } else if (action == GLFW_RELEASE) {
                        self->endPan();
                    }
                }
            });
        
        target_handler.registerCallback<input::InputType::CURSOR_POSITION>(
            [self](GLFWwindow* window, double xpos, double ypos) {
                if (!self) return;
                self->updateOrbit(xpos, ypos);
                self->updatePan(xpos, ypos);
            });
        
        target_handler.registerCallback<input::InputType::SCROLL>(
            [self](GLFWwindow* window, double xoffset, double yoffset) {
                if (!self) return;
                self->zoom(yoffset);
            });
    }
    
    /**
     * @brief Detaches arcball input handling from an input handler.
     * This clears the arcball callbacks from the target handler by registering
     * null callbacks, effectively disabling arcball input.
     * @param target_handler Reference to the input handler to detach from
     */
    void detachFrom(input::InputHandler& target_handler) {
        // Register null callbacks to clear arcball handling
        target_handler.registerCallback<input::InputType::MOUSE_BUTTON>(
            input::InputHandler::MouseButtonCallback{});
        target_handler.registerCallback<input::InputType::CURSOR_POSITION>(
            input::InputHandler::CursorPosCallback{});
        target_handler.registerCallback<input::InputType::SCROLL>(
            input::InputHandler::ScrollCallback{});
    }
    
    /**
     * @brief Initializes ArcBall state from the current camera position.
     * Calculates initial spherical coordinates from the camera's transform
     * and sets an appropriate default target based on camera orientation.
     * This method should be called when first setting up the ArcBall controller
     * to match the existing camera state.
     */
    void initializeFromCamera() {
        // Get active camera from scene graph
        auto camera = scene::graph()->getActiveCamera();
        if (!camera) {
            std::cerr << "ArcBall: No active camera available for initialization" << std::endl;
            return;
        }
        
        // Get camera's current world position
        glm::vec3 camera_position = camera->getWorldTransform()[3];
        
        // First, check if the camera has a target set and use it as the arcball center
        auto camera_target = camera->getTarget();
        if (camera_target) {
            setTarget(camera_target);
        } else if (m_target == glm::vec3(0.0f) && !m_target_component) {
            // If no camera target and no arcball target is set, calculate a reasonable target based on camera orientation
            const glm::mat4& camera_world_transform = camera->getWorldTransform();
            glm::vec3 forward = -glm::vec3(camera_world_transform[2]); // Camera looks down -Z
            
            // Set target at a reasonable distance in front of the camera
            float default_target_distance = 5.0f;
            m_target = camera_position + forward * default_target_distance;
        }
        
        // Calculate spherical coordinates from current camera position relative to target
        cartesianToSpherical(camera_position);
        
        // Ensure radius is within configured limits
        m_radius = glm::clamp(m_radius, m_min_radius, m_max_radius);
    }
    
    /**
     * @brief Initializes ArcBall state from a specific camera node by name.
     * @param camera_node_name Name of the scene node containing the camera
     */
    void initializeFromCameraNode(const std::string& camera_node_name) {
        auto camera_node = scene::graph()->getNodeByName(camera_node_name);
        if (!camera_node) {
            std::cerr << "ArcBall: Camera node '" << camera_node_name << "' not found" << std::endl;
            return;
        }
        initializeFromCameraNode(camera_node);
    }
    
    /**
     * @brief Initializes ArcBall state from a specific camera node.
     * @param camera_node Shared pointer to the scene node containing the camera
     */
    void initializeFromCameraNode(scene::SceneNodePtr camera_node) {
        if (!camera_node) {
            std::cerr << "ArcBall: Null camera node provided" << std::endl;
            return;
        }
        
        // Get the camera component from the node
        auto camera = camera_node->payload().get<component::Camera>();
        if (!camera) {
            std::cerr << "ArcBall: Node '" << camera_node->getName() << "' has no Camera component" << std::endl;
            return;
        }
        
        // Get camera's current world position
        glm::vec3 camera_position = camera->getWorldTransform()[3];
        
        // First, check if the camera has a target set and use it as the arcball center
        auto camera_target = camera->getTarget();
        if (camera_target) {
            setTarget(camera_target);
        } else if (m_target == glm::vec3(0.0f) && !m_target_component) {
            // If no camera target and no arcball target is set, calculate a reasonable target based on camera orientation
            const glm::mat4& camera_world_transform = camera->getWorldTransform();
            glm::vec3 forward = -glm::vec3(camera_world_transform[2]); // Camera looks down -Z
            
            // Set target at a reasonable distance in front of the camera
            float default_target_distance = 5.0f;
            m_target = camera_position + forward * default_target_distance;
        }
        
        // Calculate spherical coordinates from current camera position relative to target
        cartesianToSpherical(camera_position);
        
        // Ensure radius is within configured limits
        m_radius = glm::clamp(m_radius, m_min_radius, m_max_radius);
    }

private:
    // --- State Management ---
    
    /// World space target point for camera orbiting
    glm::vec3 m_target;
    
    /// Optional transform component target for dynamic tracking
    component::ObservedTransformComponentPtr m_target_component;
    
    /// Distance from target to camera
    float m_radius;
    
    /// Azimuthal angle (horizontal rotation, 0 to 2π)
    float m_theta;
    
    /// Polar angle (vertical rotation, 0 to π)
    float m_phi;

    // --- Sensitivity Settings ---
    
    /// Rotation sensitivity for orbit operations
    float m_rotation_sensitivity;
    
    /// Zoom sensitivity for scroll operations
    float m_zoom_sensitivity;
    
    /// Pan sensitivity for translation operations
    float m_pan_sensitivity;

    // --- Zoom Constraints ---
    
    /// Minimum allowed distance from target
    float m_min_radius;
    
    /// Maximum allowed distance from target
    float m_max_radius;

    // --- Input State Tracking ---
    
    /// True when orbit operation is active
    bool m_is_orbiting;
    
    /// True when pan operation is active
    bool m_is_panning;
    
    /// Last recorded mouse X position
    double m_last_mouse_x;
    
    /// Last recorded mouse Y position
    double m_last_mouse_y;

    // --- Helper Methods ---
    
    /**
     * @brief Updates the active camera's position based on current spherical coordinates.
     * Accesses the active camera through scene::graph()->getActiveCamera() and applies
     * calculated transformations to the camera's transform component.
     * Ensures compatibility with both perspective and orthographic cameras.
     */
    void updateCameraPosition() {
        // Get active camera from scene graph
        auto camera = scene::graph()->getActiveCamera();
        if (!camera) {
            std::cerr << "ArcBall: No active camera available" << std::endl;
            return;
        }
        
        // Check if we should sync with the camera's target (if we don't have our own target component)
        if (!m_target_component) {
            auto camera_target = camera->getTarget();
            if (camera_target) {
                // If the camera has a target but we don't have a target component, sync with it
                if (!m_target_component || m_target_component != camera_target) {
                    setTarget(camera_target);
                    return; // setTarget will call updateCameraPosition again
                }
            }
        }
        
        // Update target from component if available
        if (m_target_component) {
            const glm::mat4& world_transform = m_target_component->getCachedWorldTransform();
            m_target = glm::vec3(world_transform[3]);
        }
        
        // Calculate new camera position from spherical coordinates
        glm::vec3 camera_position = sphericalToCartesian();
        
        // Update camera transform using the correct transform interface
        auto camera_transform = camera->getTransform();
        if (camera_transform) {
            // Calculate look-at orientation
            glm::vec3 forward = glm::normalize(m_target - camera_position);
            glm::vec3 world_up(0.0f, 1.0f, 0.0f);
            glm::vec3 right = glm::normalize(glm::cross(forward, world_up));
            glm::vec3 up = glm::cross(right, forward);
            
            // Create transformation matrix combining position and orientation
            glm::mat4 transform_matrix(1.0f);
            
            // Set rotation part (3x3 upper-left)
            transform_matrix[0] = glm::vec4(right, 0.0f);
            transform_matrix[1] = glm::vec4(up, 0.0f);
            transform_matrix[2] = glm::vec4(-forward, 0.0f); // Negative because we want to look towards the target
            
            // Set translation part (4th column)
            transform_matrix[3] = glm::vec4(camera_position, 1.0f);
            
            // Apply the complete transformation matrix to the camera
            camera_transform->setMatrix(transform_matrix);
        }
    }
    
    /**
     * @brief Converts spherical coordinates to cartesian position relative to target.
     * @return Camera position in world space
     */
    glm::vec3 sphericalToCartesian() const {
        return arcball::sphericalToCartesian(m_radius, m_theta, m_phi, m_target);
    }
    
    /**
     * @brief Initializes spherical coordinates from a cartesian position.
     * @param position Camera position in world space
     */
    void cartesianToSpherical(const glm::vec3& position) {
        arcball::cartesianToSpherical(position, m_target, m_radius, m_theta, m_phi);
    }
};


// --- Convenience Static Functions ---

/**
 * @brief Convenience function to quickly attach arcball controls to an existing input handler.
 * Creates an arcball controller from the current camera and attaches it to the target handler.
 * @param target_handler Reference to the input handler to attach arcball controls to
 * @return Shared pointer to the created ArcBallController for further configuration
 */
inline std::shared_ptr<ArcBallController> attachArcballTo(input::InputHandler& target_handler) {
    auto controller = ArcBallController::CreateFromCamera();
    controller->attachTo(target_handler);
    return controller;
}

/**
 * @brief Convenience function to quickly attach arcball controls with a custom controller.
 * @param target_handler Reference to the input handler to attach arcball controls to
 * @param controller Shared pointer to the ArcBall controller to use
 * @return The same controller pointer for chaining
 */
inline std::shared_ptr<ArcBallController> attachArcballTo(input::InputHandler& target_handler, 
                                                           std::shared_ptr<ArcBallController> controller) {
    if (controller) {
        controller->attachTo(target_handler);
    }
    return controller;
}

/**
 * @brief Convenience function to quickly detach arcball controls from an input handler.
 * This clears all arcball-related callbacks from the target handler.
 * @param target_handler Reference to the input handler to detach arcball controls from
 */
inline void detachArcballFrom(input::InputHandler& target_handler) {
    // Register null callbacks to clear arcball handling
    target_handler.registerCallback<input::InputType::MOUSE_BUTTON>(
        input::InputHandler::MouseButtonCallback{});
    target_handler.registerCallback<input::InputType::CURSOR_POSITION>(
        input::InputHandler::CursorPosCallback{});
    target_handler.registerCallback<input::InputType::SCROLL>(
        input::InputHandler::ScrollCallback{});
}

} // namespace arcball

#endif // ARCBALL_CONTROLLER_H