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

    ArcBallController& getController() {
        return *m_controller;
    }
    
    // --- Simple Attachment Interface ---
    
    /**
     * @brief Attaches arcball input handling to another input handler.
     * This registers the arcball callbacks with the target handler, allowing
     * the arcball to work through that handler's input system.
     * @param target_handler Reference to the input handler to attach to
     */
    void attachTo(input::InputHandler& target_handler) {
        if (!m_controller) return;
        
        // Register arcball callbacks with the target handler
        target_handler.registerCallback<input::InputType::MOUSE_BUTTON>(
            createMouseButtonCallback(m_controller));
        target_handler.registerCallback<input::InputType::CURSOR_POSITION>(
            createCursorPosCallback(m_controller));
        target_handler.registerCallback<input::InputType::SCROLL>(
            createScrollCallback(m_controller));
    }
    
    /**
     * @brief Detaches arcball input handling from another input handler.
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

// --- Convenience Static Functions ---

/**
 * @brief Convenience function to quickly attach arcball controls to an existing input handler.
 * Creates an arcball controller from the current camera and attaches it to the target handler.
 * @param target_handler Reference to the input handler to attach arcball controls to
 * @return Shared pointer to the created ArcBallInputHandler for further configuration
 */
inline std::shared_ptr<ArcBallInputHandler> attachArcballTo(input::InputHandler& target_handler) {
    auto arcball_handler = ArcBallInputHandler::CreateFromCamera();
    arcball_handler->attachTo(target_handler);
    return arcball_handler;
}

/**
 * @brief Convenience function to quickly attach arcball controls with a custom controller.
 * @param target_handler Reference to the input handler to attach arcball controls to
 * @param controller Shared pointer to the ArcBall controller to use
 * @return Shared pointer to the created ArcBallInputHandler for further configuration
 */
inline std::shared_ptr<ArcBallInputHandler> attachArcballTo(input::InputHandler& target_handler, 
                                                           std::shared_ptr<ArcBallController> controller) {
    auto arcball_handler = std::make_shared<ArcBallInputHandler>(controller);
    arcball_handler->attachTo(target_handler);
    return arcball_handler;
}

/**
 * @brief Convenience function to quickly detach arcball controls from an input handler.
* This clears all arcball-related callbacks from the target handler.
 * @param target_handler Reference to the input handler to detach arcball controls from
 */
inline void detachArcballFrom(input::InputHandler& target_handler) {
    // Create a temporary handler just to call detachFrom
    auto temp_handler = std::make_shared<ArcBallInputHandler>(nullptr);
    temp_handler->detachFrom(target_handler);
}

} // namespace arcball

#endif // ARCBALL_INPUT_HANDLER_H