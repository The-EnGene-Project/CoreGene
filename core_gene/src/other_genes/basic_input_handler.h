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
    // --- We override the virtual handler methods from the base class ---

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

    void handleMouseButton(GLFWwindow* win, int button, int action, int mods) override {
        if (action == GLFW_PRESS) {
            std::cout << "Mouse button pressed via BasicInputHandler!" << std::endl;
        }
    }

private:
    // State is a regular member variable, encapsulated within this class.
    bool m_wireframe_mode = false;
};

} // namespace input

#endif // BASIC_INPUT_HANDLER_H

