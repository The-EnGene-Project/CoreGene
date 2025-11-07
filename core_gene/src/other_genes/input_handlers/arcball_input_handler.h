#ifndef ARCBALL_INPUT_HANDLER_H
#define ARCBALL_INPUT_HANDLER_H
#pragma once

#include "../../gl_base/gl_includes.h"
#include "../../gl_base/input_handler.h"
#include "arcball_controller.h"
#include <memory>

namespace arcball {

// --- Lambda Function Providers ---
/**
 * @brief Creates a mouse button callback lambda compatible with InputHandler.
 * @param controller Shared pointer to the ArcBall controller
 * @return Lambda function for mouse button handling
 */
inline input::InputHandler::MouseButtonCallback createMouseButtonCallback(
    std::shared_ptr<ArcBallController> controller) {
    return [controller](GLFWwindow* window, int button, int action, int mods) {
        if (!controller) return;
        
        double mouse_x, mouse_y;
        glfwGetCursorPos(window, &mouse_x, &mouse_y);
        
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                controller->startOrbit(mouse_x, mouse_y);
            } else if (action == GLFW_RELEASE) {
                controller->endOrbit();
            }
        } else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
            if (action == GLFW_PRESS) {
                controller->startPan(mouse_x, mouse_y);
            } else if (action == GLFW_RELEASE) {
                controller->endPan();
            }
        }
    };
}

/**
 * @brief Creates a cursor position callback lambda compatible with InputHandler.
 * @param controller Shared pointer to the ArcBall controller
 * @return Lambda function for cursor position handling
 */
inline input::InputHandler::CursorPosCallback createCursorPosCallback(
    std::shared_ptr<ArcBallController> controller) {
    return [controller](GLFWwindow* window, double xpos, double ypos) {
        if (!controller) return;
        
        controller->updateOrbit(xpos, ypos);
        controller->updatePan(xpos, ypos);
    };
}

/**
 * @brief Creates a scroll callback lambda compatible with InputHandler.
 * @param controller Shared pointer to the ArcBall controller
 * @return Lambda function for scroll handling
 */
inline input::InputHandler::ScrollCallback createScrollCallback(
    std::shared_ptr<ArcBallController> controller) {
    return [controller](GLFWwindow* window, double xoffset, double yoffset) {
        if (!controller) return;
        
        controller->zoom(yoffset);
    };
}


/**
 * @class ArcBallInputHandler
 * @brief InputHandler extension that provides complete ArcBall input handling.
 *  
 * This class extends the InputHandler system to provide a complete input handling
 * solution for ArcBall camera controls, managing all GLFW callbacks internally.
 */
class ArcBallInputHandler : public input::InputHandler {
public:
    /**
     * @brief Constructs an ArcBall input handler with the given controller.
     * @param controller Shared pointer to the ArcBall controller to use
     */
    explicit ArcBallInputHandler(std::shared_ptr<ArcBallController> controller)
        : m_controller(controller)
    {
    }
    
    /**
     * @brief Static factory method to create an ArcBall input handler initialized from current camera state.
     * @return Shared pointer to a new ArcBallInputHandler with controller initialized from the active camera
     */
    static std::shared_ptr<ArcBallInputHandler> CreateFromCamera() {
        auto controller = ArcBallController::CreateFromCamera();
        return std::make_shared<ArcBallInputHandler>(controller);
    }
    
    /**
     * @brief Static factory method to create an ArcBall input handler from a specific camera node.
     * @param camera_node_name Name of the scene node containing the camera
     * @return Shared pointer to a new ArcBallInputHandler initialized from the specified camera
     */
    static std::shared_ptr<ArcBallInputHandler> CreateFromCameraNode(const std::string& camera_node_name) {
        auto controller = ArcBallController::CreateFromCameraNode(camera_node_name);
        return std::make_shared<ArcBallInputHandler>(controller);
    }
    
    /**
     * @brief Static factory method to create an ArcBall input handler from a specific camera node.
     * @param camera_node Shared pointer to the scene node containing the camera
     * @return Shared pointer to a new ArcBallInputHandler initialized from the specified camera
     */
    static std::shared_ptr<ArcBallInputHandler> CreateFromCameraNode(scene::SceneNodePtr camera_node) {
        auto controller = ArcBallController::CreateFromCameraNode(camera_node);
        return std::make_shared<ArcBallInputHandler>(controller);
    }

    /**
     * @brief Gets the underlying controller for direct access.
     * @return Reference to the ArcBallController
     */
    ArcBallController& getController() {
        return *m_controller;
    }
    
    /**
     * @brief Synchronizes the arcball controller's target with the current camera target.
     * This should be called each frame before rendering to ensure the arcball
     * controller is following the correct target component.
     */
    void syncWithCameraTarget() {
        if (!m_controller) return;
        
        m_controller->syncWithCameraTarget();
    }
    
    // --- Configuration Pass-through Methods ---
    
    /**
     * @brief Sets the target point for camera orbiting.
     * @param target World space position to orbit around
     */
    void setTarget(const glm::vec3& target) {
        if (m_controller) {
            m_controller->setTarget(target);
        }
    }
    
    /**
     * @brief Sets the target to follow a transform component.
     * @param target Shared pointer to transform component to track
     */
    void setTarget(component::ObservedTransformComponentPtr target) {
        if (m_controller) {
            m_controller->setTarget(target);
        }
    }
    
    /**
     * @brief Configures input sensitivity for all operations.
     * @param rotation Sensitivity for orbit operations
     * @param zoom Sensitivity for zoom operations
     * @param pan Sensitivity for pan operations
     */
    void setSensitivity(float rotation, float zoom, float pan) {
        if (m_controller) {
            m_controller->setSensitivity(rotation, zoom, pan);
        }
    }
    
    /**
     * @brief Sets minimum and maximum zoom distance constraints.
     * @param min_radius Minimum allowed distance from target
     * @param max_radius Maximum allowed distance from target
     */
    void setZoomLimits(float min_radius, float max_radius) {
        if (m_controller) {
            m_controller->setZoomLimits(min_radius, max_radius);
        }
    }
    
    /**
     * @brief Resets camera to default position and orientation.
     */
    void reset() {
        if (m_controller) {
            m_controller->reset();
        }
    }

protected:
    // --- InputHandler Overrides ---
    
    /**
     * @brief Handles mouse button press/release events.
     * Maps left mouse button to orbit and middle mouse button to pan operations.
     */
    void handleMouseButton(GLFWwindow* window, int button, int action, int mods) override {
        if (!m_controller) return;
        
        double mouse_x, mouse_y;
        glfwGetCursorPos(window, &mouse_x, &mouse_y);
        
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                m_controller->startOrbit(mouse_x, mouse_y);
            } else if (action == GLFW_RELEASE) {
                m_controller->endOrbit();
            }
        } else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
            if (action == GLFW_PRESS) {
                m_controller->startPan(mouse_x, mouse_y);
            } else if (action == GLFW_RELEASE) {
                m_controller->endPan();
            }
        }
    }
    
    /**
     * @brief Handles continuous mouse movement for orbit and pan updates.
     */
    void handleCursorPos(GLFWwindow* window, double xpos, double ypos) override {
        if (!m_controller) return;
        
        m_controller->updateOrbit(xpos, ypos);
        m_controller->updatePan(xpos, ypos);
    }
    
    /**
     * @brief Handles scroll wheel input for zoom operations.
     */
    void handleScroll(GLFWwindow* window, double xoffset, double yoffset) override {
        if (!m_controller) return;
        
        m_controller->zoom(yoffset);
    }

private:
    /// Shared pointer to the ArcBall controller
    std::shared_ptr<ArcBallController> m_controller;
};

} // namespace arcball

#endif // ARCBALL_INPUT_HANDLER_H