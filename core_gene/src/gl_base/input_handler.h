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

    virtual ~InputHandler() = default;

    void applyCallbacks(GLFWwindow* window) {
        glfwSetWindowUserPointer(window, this);
        glfwSetKeyCallback(window, dispatchKey);
        glfwSetMouseButtonCallback(window, dispatchMouseButton);
        glfwSetCursorPosCallback(window, dispatchCursorPos);
        glfwSetFramebufferSizeCallback(window, dispatchFramebufferSize);
    }

    /**
     * @brief Registers a callable (like a lambda) for a specific input type.
     *
     * This template function deduces the type of the provided callable and attempts
     * to assign it to the correct std::function based on the InputType.
     * This is checked at COMPILE-TIME, providing a simple API with full type safety.
     *
     * @tparam Callable The type of the lambda or other function object.
     * @param type The enum value corresponding to the callback type.
     * @param callback The lambda or function object to register.
     */
    template<typename Callable>
    void registerCallback(InputType type, Callable callback) {
        switch (type) {
            case InputType::KEY:
                // `if constexpr` checks this at compile time.
                if constexpr (std::is_constructible_v<KeyCallback, Callable>) {
                    m_key_callback = callback;
                } else {
                    // This will cause a clear compile error if the lambda signature is wrong.
                    static_assert(false, "The provided lambda/function is not compatible with the KeyCallback signature.");
                }
                break;

            case InputType::MOUSE_BUTTON:
                if constexpr (std::is_constructible_v<MouseButtonCallback, Callable>) {
                    m_mouse_button_callback = callback;
                } else {
                    static_assert(false, "The provided lambda/function is not compatible with the MouseButtonCallback signature.");
                }
                break;

            case InputType::CURSOR_POSITION:
                if constexpr (std::is_constructible_v<CursorPosCallback, Callable>) {
                    m_cursor_pos_callback = callback;
                } else {
                    static_assert(false, "The provided lambda/function is not compatible with the CursorPosCallback signature.");
                }
                break;

            case InputType::FRAMEBUFFER_SIZE:
                 if constexpr (std::is_constructible_v<FramebufferSizeCallback, Callable>) {
                    m_framebuffer_size_callback = callback;
                } else {
                    static_assert(false, "The provided lambda/function is not compatible with the FramebufferSizeCallback signature.");
                }
                break;
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

