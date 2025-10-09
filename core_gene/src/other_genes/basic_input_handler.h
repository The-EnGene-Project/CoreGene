#ifndef BASIC_INPUT_HANDLER_H
#define BASIC_INPUT_HANDLER_H
#pragma once

#include "../gl_base/input_handler.h"
#include <iostream>

namespace input {

/**
 * @class BasicInputHandler
 * @brief A concrete implementation of InputHandler using the inheritance pattern.
 *
 * This class encapsulates a complete, stateful input scheme. It defines behaviors
 * for closing the window ('Q') and toggling wireframe mode ('T'), and it owns
 * the state associated with that logic (m_wireframe_mode).
 */
class BasicInputHandler : public InputHandler {
public:
    BasicInputHandler() = default;
    ~BasicInputHandler() override = default;

protected:
    void handleKey(GLFWwindow* window, int key, int scancode, int action, int mods) override {
        if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        // Toggle wireframe mode
        else if (key == GLFW_KEY_T && action == GLFW_PRESS) {
            m_wireframe_mode = !m_wireframe_mode;
            if (m_wireframe_mode) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            } else {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }
            std::cout << "Wireframe mode " << (m_wireframe_mode ? "ON" : "OFF") << std::endl;
        }
    }

    // UPDATED: Implemented the NDC calculation from the old version.
    void handleMouseButton(GLFWwindow* win, int button, int action, int mods) override {
        if (action == GLFW_PRESS) {
            double xpos, ypos;
            glfwGetCursorPos(win, &xpos, &ypos);

            int fb_w, fb_h;
            glfwGetFramebufferSize(win, &fb_w, &fb_h);
            
            // Convert screen coordinates (pixels) to Normalized Device Coordinates (NDC) [-1, 1]
            float x_ndc = ((float)xpos / (float)fb_w) * 2.0f - 1.0f;
            float y_ndc = (1.0f - ((float)ypos / (float)fb_h)) * 2.0f - 1.0f;

            std::cout << "Mouse click at NDC: (" << x_ndc << ", " << y_ndc << ")" << std::endl;
        }
    }

    // NEW: Added the missing cursor position handler.
    void handleCursorPos(GLFWwindow* win, double xpos, double ypos) override {
        // Convert screen pos (upside down) to framebuffer pos (e.g., for retina displays)
        int wn_w, wn_h, fb_w, fb_h;
        glfwGetWindowSize(win, &wn_w, &wn_h);
        glfwGetFramebufferSize(win, &fb_w, &fb_h);
        
        double x = xpos * fb_w / wn_w;
        double y = (wn_h - ypos) * fb_h / wn_h; // Invert Y-axis
        
        std::cout << "Cursor at Framebuffer Coords: (" << x << ", " << y << ")" << std::endl;
    }

private:
    // State is a regular member variable, encapsulated within this class.
    bool m_wireframe_mode = false;
};

} // namespace input

#endif // BASIC_INPUT_HANDLER_H