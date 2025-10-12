#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H
#pragma once

#include "gl_includes.h"
#include <functional>
#include <map>
#include <any>
#include <iostream>
#include <type_traits> // Required for std::is_constructible_v

namespace input {

/**
 * @enum InputType
 * @brief Defines the different types of GLFW input callbacks that can be registered.
 * This provides discoverability for the available input events.
 */
enum class InputType {
    KEY,
    MOUSE_BUTTON,
    CURSOR_POSITION,
    FRAMEBUFFER_SIZE
};

/**
 * @class InputHandler
 * @brief A hybrid class for handling GLFW input using both polymorphism and composition.
 *
 * This class supports two patterns:
 * 1. (Inheritance) Create a derived class for a complete, stateful input scheme (e.g., `InGameHandler`).
 * 2. (Composition) Register individual callback functions (lambdas) with an instance for simple or one-off behaviors.
 *
 * The dispatcher will always prioritize a registered callback over the virtual method.
 */
class InputHandler {
public:
    // --- Type aliases for callback signatures ---
    using KeyCallback = std::function<void(GLFWwindow*, int, int, int, int)>;
    using MouseButtonCallback = std::function<void(GLFWwindow*, int, int, int)>;
    using CursorPosCallback = std::function<void(GLFWwindow*, double, double)>;
    using FramebufferSizeCallback = std::function<void(GLFWwindow*, int, int)>;

    // --- Convenience Macros for Lambda Signatures ---
    #define KEY_HANDLER_ARGS 				GLFWwindow* window, int key, int scancode, int action, int mods
    #define MOUSE_BUTTON_HANDLER_ARGS 		GLFWwindow* window, int button, int action, int mods
    #define CURSOR_POS_HANDLER_ARGS 		GLFWwindow* window, double xpos, double ypos
    #define FRAMEBUFFER_SIZE_HANDLER_ARGS 	GLFWwindow* window, int width, int height

    virtual ~InputHandler() = default;

    void applyCallbacks(GLFWwindow* window) {
        glfwSetWindowUserPointer(window, this);
        glfwSetKeyCallback(window, dispatchKey);
        glfwSetMouseButtonCallback(window, dispatchMouseButton);
        glfwSetCursorPosCallback(window, dispatchCursorPos);
        glfwSetFramebufferSizeCallback(window, dispatchFramebufferSize);
    }

    /**
 * @brief Registers a callable for a specific input type using a template parameter.
 *
 * This function uses the InputType enum as a compile-time parameter to ensure
 * the provided lambda has the correct signature.
 *
 * @tparam T The enum value from InputType corresponding to the callback type.
 * @tparam Callable The type of the lambda or other function object.
 * @param callback The lambda or function object to register.
 */
template<InputType T, typename Callable>
void registerCallback(Callable callback) {
    if constexpr (T == InputType::KEY) {
        if constexpr (std::is_constructible_v<KeyCallback, Callable>) {
            m_key_callback = callback;
        } else {
            static_assert(std::is_constructible_v<KeyCallback, Callable>,
                "The provided function is not compatible with the KeyCallback signature.");
        }
    }
    else if constexpr (T == InputType::MOUSE_BUTTON) {
        if constexpr (std::is_constructible_v<MouseButtonCallback, Callable>) {
            m_mouse_button_callback = callback;
        } else {
            static_assert(std::is_constructible_v<MouseButtonCallback, Callable>,
                "The provided function is not compatible with the MouseButtonCallback signature.");
        }
    }
    else if constexpr (T == InputType::CURSOR_POSITION) {
        if constexpr (std::is_constructible_v<CursorPosCallback, Callable>) {
            m_cursor_pos_callback = callback;
        } else {
            static_assert(std::is_constructible_v<CursorPosCallback, Callable>,
                "The provided function is not compatible with the CursorPosCallback signature.");
        }
    }
    else if constexpr (T == InputType::FRAMEBUFFER_SIZE) {
        if constexpr (std::is_constructible_v<FramebufferSizeCallback, Callable>) {
            m_framebuffer_size_callback = callback;
        } else {
            static_assert(std::is_constructible_v<FramebufferSizeCallback, Callable>,
                "The provided function is not compatible with the FramebufferSizeCallback signature.");
        }
    }
}

protected:
    // --- Virtual Handlers for Inheritance ---
    // These act as the fallback if no specific callback is registered.

    virtual void handleKey(GLFWwindow* window, int key, int scancode, int action, int mods) {}
    virtual void handleMouseButton(GLFWwindow* window, int button, int action, int mods) {}
    virtual void handleCursorPos(GLFWwindow* window, double xpos, double ypos) {}
    virtual void handleFramebufferSize(GLFWwindow* window, int width, int height) {
        glViewport(0, 0, width, height);
    }

private:
    // --- Callback Storage (for Composition) ---
    KeyCallback m_key_callback = nullptr;
    MouseButtonCallback m_mouse_button_callback = nullptr;
    CursorPosCallback m_cursor_pos_callback = nullptr;
    FramebufferSizeCallback m_framebuffer_size_callback = nullptr;

    // --- Static Dispatcher Functions ---
    // They now check for a registered callback first, then fall back to the virtual method.

    static void dispatchKey(GLFWwindow* window, int key, int scancode, int action, int mods) {
        InputHandler* instance = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));
        if (instance) {
            if (instance->m_key_callback) {
                instance->m_key_callback(window, key, scancode, action, mods);
            } else {
                instance->handleKey(window, key, scancode, action, mods);
            }
        }
    }

    static void dispatchMouseButton(GLFWwindow* window, int button, int action, int mods) {
        InputHandler* instance = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));
        if (instance) {
            if (instance->m_mouse_button_callback) {
                instance->m_mouse_button_callback(window, button, action, mods);
            } else {
                instance->handleMouseButton(window, button, action, mods);
            }
        }
    }

    static void dispatchCursorPos(GLFWwindow* window, double xpos, double ypos) {
        InputHandler* instance = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));
        if (instance) {
            if (instance->m_cursor_pos_callback) {
                instance->m_cursor_pos_callback(window, xpos, ypos);
            } else {
                instance->handleCursorPos(window, xpos, ypos);
            }
        }
    }

    static void dispatchFramebufferSize(GLFWwindow* window, int width, int height) {
        InputHandler* instance = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));
        if (instance) {
            if (instance->m_framebuffer_size_callback) {
                instance->m_framebuffer_size_callback(window, width, height);
            } else {
                instance->handleFramebufferSize(window, width, height);
            }
        }
    }
};

} // namespace input

#endif // INPUT_HANDLER_H

