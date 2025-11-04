#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H
#pragma once

#include "gl_includes.h" // Assuming this includes <GLFW/glfw3.h>
#include <functional>
#include <string>
#include <vector>
#include <type_traits> // Required for std::is_constructible_v

namespace input {

/**
 * @enum InputType
 * @brief Defines the different types of GLFW callbacks that can be registered.
 * This provides discoverability for the available events.
 */
enum class InputType {
    // --- Keyboard ---
    KEY,
    CHAR,

    // --- Mouse ---
    MOUSE_BUTTON,
    CURSOR_POSITION,
    CURSOR_ENTER,
    SCROLL,

    // --- Other Input ---
    DROP,
    JOYSTICK, // Note: This is a global, not per-window, callback

    // --- Window ---
    WINDOW_POS,
    WINDOW_SIZE,
    WINDOW_CLOSE,
    WINDOW_REFRESH,
    WINDOW_FOCUS,
    WINDOW_ICONIFY,
    WINDOW_MAXIMIZE,
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
    using CharCallback = std::function<void(GLFWwindow*, unsigned int)>;
    using MouseButtonCallback = std::function<void(GLFWwindow*, int, int, int)>;
    using CursorPosCallback = std::function<void(GLFWwindow*, double, double)>;
    using CursorEnterCallback = std::function<void(GLFWwindow*, int)>;
    using ScrollCallback = std::function<void(GLFWwindow*, double, double)>;
    using DropCallback = std::function<void(GLFWwindow*, const std::vector<std::string>&)>;
    using JoystickCallback = std::function<void(int, int)>; // Global: (int jid, int event)
    using WindowPosCallback = std::function<void(GLFWwindow*, int, int)>;
    using WindowSizeCallback = std::function<void(GLFWwindow*, int, int)>;
    using WindowCloseCallback = std::function<void(GLFWwindow*)>;
    using WindowRefreshCallback = std::function<void(GLFWwindow*)>;
    using WindowFocusCallback = std::function<void(GLFWwindow*, int)>;
    using WindowIconifyCallback = std::function<void(GLFWwindow*, int)>;
    using WindowMaximizeCallback = std::function<void(GLFWwindow*, int)>;
    using FramebufferSizeCallback = std::function<void(GLFWwindow*, int, int)>;

    // --- Convenience Macros for Lambda Signatures ---
    #define KEY_HANDLER_ARGS                 GLFWwindow* window, int key, int scancode, int action, int mods
    #define CHAR_HANDLER_ARGS                GLFWwindow* window, unsigned int codepoint
    #define MOUSE_BUTTON_HANDLER_ARGS        GLFWwindow* window, int button, int action, int mods
    #define CURSOR_POS_HANDLER_ARGS          GLFWwindow* window, double xpos, double ypos
    #define CURSOR_ENTER_HANDLER_ARGS        GLFWwindow* window, int entered
    #define SCROLL_HANDLER_ARGS              GLFWwindow* window, double xoffset, double yoffset
    #define DROP_HANDLER_ARGS                GLFWwindow* window, const std::vector<std::string>& paths
    #define JOYSTICK_HANDLER_ARGS            int jid, int event
    #define WINDOW_POS_HANDLER_ARGS          GLFWwindow* window, int xpos, int ypos
    #define WINDOW_SIZE_HANDLER_ARGS         GLFWwindow* window, int width, int height
    #define WINDOW_CLOSE_HANDLER_ARGS        GLFWwindow* window
    #define WINDOW_REFRESH_HANDLER_ARGS      GLFWwindow* window
    #define WINDOW_FOCUS_HANDLER_ARGS        GLFWwindow* window, int focused
    #define WINDOW_ICONIFY_HANDLER_ARGS      GLFWwindow* window, int iconified
    #define WINDOW_MAXIMIZE_HANDLER_ARGS     GLFWwindow* window, int maximized
    #define FRAMEBUFFER_SIZE_HANDLER_ARGS    GLFWwindow* window, int width, int height

    virtual ~InputHandler() = default;

    /**
     * @brief Applies all window-specific callbacks to the given GLFW window.
     */
    void applyCallbacks(GLFWwindow* window) {
        glfwSetWindowUserPointer(window, this);

        // Keyboard
        glfwSetKeyCallback(window, dispatchKey);
        glfwSetCharCallback(window, dispatchChar);

        // Mouse
        glfwSetMouseButtonCallback(window, dispatchMouseButton);
        glfwSetCursorPosCallback(window, dispatchCursorPos);
        glfwSetCursorEnterCallback(window, dispatchCursorEnter);
        glfwSetScrollCallback(window, dispatchScroll);

        // Other Input
        glfwSetDropCallback(window, dispatchDrop);
        
        // Window
        glfwSetWindowPosCallback(window, dispatchWindowPos);
        glfwSetWindowSizeCallback(window, dispatchWindowSize);
        glfwSetWindowCloseCallback(window, dispatchWindowClose);
        glfwSetWindowRefreshCallback(window, dispatchWindowRefresh);
        glfwSetWindowFocusCallback(window, dispatchWindowFocus);
        glfwSetWindowIconifyCallback(window, dispatchWindowIconify);
        glfwSetWindowMaximizeCallback(window, dispatchWindowMaximize);
        glfwSetFramebufferSizeCallback(window, dispatchFramebufferSize);
    }

    /**
     * @brief Applies all global (non-window) GLFW callbacks.
     */
    static void applyGlobalCallbacks() {
        glfwSetJoystickCallback(dispatchJoystick);
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
            static_assert_assignable<KeyCallback, Callable>();
            m_key_callback = callback;
        }
        else if constexpr (T == InputType::CHAR) {
            static_assert_assignable<CharCallback, Callable>();
            m_char_callback = callback;
        }
        else if constexpr (T == InputType::MOUSE_BUTTON) {
            static_assert_assignable<MouseButtonCallback, Callable>();
            m_mouse_button_callback = callback;
        }
        else if constexpr (T == InputType::CURSOR_POSITION) {
            static_assert_assignable<CursorPosCallback, Callable>();
            m_cursor_pos_callback = callback;
        }
        else if constexpr (T == InputType::CURSOR_ENTER) {
            static_assert_assignable<CursorEnterCallback, Callable>();
            m_cursor_enter_callback = callback;
        }
        else if constexpr (T == InputType::SCROLL) {
            static_assert_assignable<ScrollCallback, Callable>();
            m_scroll_callback = callback;
        }
        else if constexpr (T == InputType::DROP) {
            static_assert_assignable<DropCallback, Callable>();
            m_drop_callback = callback;
        }
        else if constexpr (T == InputType::WINDOW_POS) {
            static_assert_assignable<WindowPosCallback, Callable>();
            m_window_pos_callback = callback;
        }
        else if constexpr (T == InputType::WINDOW_SIZE) {
            static_assert_assignable<WindowSizeCallback, Callable>();
            m_window_size_callback = callback;
        }
        else if constexpr (T == InputType::WINDOW_CLOSE) {
            static_assert_assignable<WindowCloseCallback, Callable>();
            m_window_close_callback = callback;
        }
        else if constexpr (T == InputType::WINDOW_REFRESH) {
            static_assert_assignable<WindowRefreshCallback, Callable>();
            m_window_refresh_callback = callback;
        }
        else if constexpr (T == InputType::WINDOW_FOCUS) {
            static_assert_assignable<WindowFocusCallback, Callable>();
            m_window_focus_callback = callback;
        }
        else if constexpr (T == InputType::WINDOW_ICONIFY) {
            static_assert_assignable<WindowIconifyCallback, Callable>();
            m_window_iconify_callback = callback;
        }
        else if constexpr (T == InputType::WINDOW_MAXIMIZE) {
            static_assert_assignable<WindowMaximizeCallback, Callable>();
            m_window_maximize_callback = callback;
        }
        else if constexpr (T == InputType::FRAMEBUFFER_SIZE) {
            static_assert_assignable<FramebufferSizeCallback, Callable>();
            m_framebuffer_size_callback = callback;
        }
        else if constexpr (T == InputType::JOYSTICK) {
            // This is a compile-time error!
            static_assert(!std::is_same_v<Callable, Callable>,
                "JOYSTICK is a global callback. Use the static 'registerJoystickCallback' method instead.");
        }
    }

    /**
     * @brief Registers a callable for the global joystick connection/disconnection event.
     */
    template<typename Callable>
    static void registerJoystickCallback(Callable callback) {
        static_assert_assignable<JoystickCallback, Callable>();
        s_joystick_callback = callback;
    }

protected:
    // --- Virtual Handlers for Inheritance ---
    // These act as the fallback if no specific callback is registered.
    virtual void handleKey(KEY_HANDLER_ARGS) {}
    virtual void handleChar(CHAR_HANDLER_ARGS) {}
    virtual void handleMouseButton(MOUSE_BUTTON_HANDLER_ARGS) {}
    virtual void handleCursorPos(CURSOR_POS_HANDLER_ARGS) {}
    virtual void handleCursorEnter(CURSOR_ENTER_HANDLER_ARGS) {}
    virtual void handleScroll(SCROLL_HANDLER_ARGS) {}
    virtual void handleDrop(DROP_HANDLER_ARGS) {}
    // Note: No virtual handleJoystick, as it's global and has no 'this' instance

    virtual void handleWindowPos(WINDOW_POS_HANDLER_ARGS) {}
    virtual void handleWindowSize(WINDOW_SIZE_HANDLER_ARGS) {}
    virtual void handleWindowClose(WINDOW_CLOSE_HANDLER_ARGS) {}
    virtual void handleWindowRefresh(WINDOW_REFRESH_HANDLER_ARGS) {}
    virtual void handleWindowFocus(WINDOW_FOCUS_HANDLER_ARGS) {}
    virtual void handleWindowIconify(WINDOW_ICONIFY_HANDLER_ARGS) {}
    virtual void handleWindowMaximize(WINDOW_MAXIMIZE_HANDLER_ARGS) {}

    virtual void handleFramebufferSize(FRAMEBUFFER_SIZE_HANDLER_ARGS) {
        glViewport(0, 0, width, height);
    }

private:
    // --- Helper for static_assert ---
    template<typename T, typename U>
    static constexpr void static_assert_assignable() {
        static_assert(std::is_constructible_v<T, U>,
            "The provided function is not compatible with the required callback signature.");
    }

    // --- Callback Storage (for Composition) ---
    KeyCallback m_key_callback = nullptr;
    CharCallback m_char_callback = nullptr;
    MouseButtonCallback m_mouse_button_callback = nullptr;
    CursorPosCallback m_cursor_pos_callback = nullptr;
    CursorEnterCallback m_cursor_enter_callback = nullptr;
    ScrollCallback m_scroll_callback = nullptr;
    DropCallback m_drop_callback = nullptr;
    WindowPosCallback m_window_pos_callback = nullptr;
    WindowSizeCallback m_window_size_callback = nullptr;
    WindowCloseCallback m_window_close_callback = nullptr;
    WindowRefreshCallback m_window_refresh_callback = nullptr;
    WindowFocusCallback m_window_focus_callback = nullptr;
    WindowIconifyCallback m_window_iconify_callback = nullptr;
    WindowMaximizeCallback m_window_maximize_callback = nullptr;
    FramebufferSizeCallback m_framebuffer_size_callback = nullptr;

    // Static storage for global joystick callback
    inline static JoystickCallback s_joystick_callback = nullptr;

    // --- Static Dispatcher Functions ---

    static InputHandler* getInstance(GLFWwindow* window) {
        return static_cast<InputHandler*>(glfwGetWindowUserPointer(window));
    }

    static void dispatchKey(KEY_HANDLER_ARGS) {
        if (InputHandler* instance = getInstance(window)) {
            if (instance->m_key_callback)
                instance->m_key_callback(window, key, scancode, action, mods);
            else
                instance->handleKey(window, key, scancode, action, mods);
        }
    }

    static void dispatchChar(CHAR_HANDLER_ARGS) {
        if (InputHandler* instance = getInstance(window)) {
            if (instance->m_char_callback)
                instance->m_char_callback(window, codepoint);
            else
                instance->handleChar(window, codepoint);
        }
    }

    static void dispatchMouseButton(MOUSE_BUTTON_HANDLER_ARGS) {
        if (InputHandler* instance = getInstance(window)) {
            if (instance->m_mouse_button_callback)
                instance->m_mouse_button_callback(window, button, action, mods);
            else
                instance->handleMouseButton(window, button, action, mods);
        }
    }

    static void dispatchCursorPos(CURSOR_POS_HANDLER_ARGS) {
        if (InputHandler* instance = getInstance(window)) {
            if (instance->m_cursor_pos_callback)
                instance->m_cursor_pos_callback(window, xpos, ypos);
            else
                instance->handleCursorPos(window, xpos, ypos);
        }
    }

    static void dispatchCursorEnter(CURSOR_ENTER_HANDLER_ARGS) {
        if (InputHandler* instance = getInstance(window)) {
            if (instance->m_cursor_enter_callback)
                instance->m_cursor_enter_callback(window, entered);
            else
                instance->handleCursorEnter(window, entered);
        }
    }

    static void dispatchScroll(SCROLL_HANDLER_ARGS) {
        if (InputHandler* instance = getInstance(window)) {
            if (instance->m_scroll_callback)
                instance->m_scroll_callback(window, xoffset, yoffset);
            else
                instance->handleScroll(window, xoffset, yoffset);
        }
    }

    static void dispatchDrop(GLFWwindow* window, int count, const char** paths) {
        if (InputHandler* instance = getInstance(window)) {
            // Convert C-style array to C++ vector for safety and convenience
            std::vector<std::string> path_vec;
            path_vec.reserve(count);
            for (int i = 0; i < count; ++i) {
                path_vec.emplace_back(paths[i]);
            }

            if (instance->m_drop_callback)
                instance->m_drop_callback(window, path_vec);
            else
                instance->handleDrop(window, path_vec);
        }
    }

    static void dispatchWindowPos(WINDOW_POS_HANDLER_ARGS) {
        if (InputHandler* instance = getInstance(window)) {
            if (instance->m_window_pos_callback)
                instance->m_window_pos_callback(window, xpos, ypos);
            else
                instance->handleWindowPos(window, xpos, ypos);
        }
    }

    static void dispatchWindowSize(WINDOW_SIZE_HANDLER_ARGS) {
        if (InputHandler* instance = getInstance(window)) {
            if (instance->m_window_size_callback)
                instance->m_window_size_callback(window, width, height);
            else
                instance->handleWindowSize(window, width, height);
        }
    }

    static void dispatchWindowClose(WINDOW_CLOSE_HANDLER_ARGS) {
        if (InputHandler* instance = getInstance(window)) {
            if (instance->m_window_close_callback)
                instance->m_window_close_callback(window);
            else
                instance->handleWindowClose(window);
        }
    }

    static void dispatchWindowRefresh(WINDOW_REFRESH_HANDLER_ARGS) {
        if (InputHandler* instance = getInstance(window)) {
            if (instance->m_window_refresh_callback)
                instance->m_window_refresh_callback(window);
            else
                instance->handleWindowRefresh(window);
        }
    }

    static void dispatchWindowFocus(WINDOW_FOCUS_HANDLER_ARGS) {
        if (InputHandler* instance = getInstance(window)) {
            if (instance->m_window_focus_callback)
                instance->m_window_focus_callback(window, focused);
            else
                instance->handleWindowFocus(window, focused);
        }
    }

    static void dispatchWindowIconify(WINDOW_ICONIFY_HANDLER_ARGS) {
        if (InputHandler* instance = getInstance(window)) {
            if (instance->m_window_iconify_callback)
                instance->m_window_iconify_callback(window, iconified);
            else
                instance->handleWindowIconify(window, iconified);
        }
    }

    static void dispatchWindowMaximize(WINDOW_MAXIMIZE_HANDLER_ARGS) {
        if (InputHandler* instance = getInstance(window)) {
            if (instance->m_window_maximize_callback)
                instance->m_window_maximize_callback(window, maximized);
            else
                instance->handleWindowMaximize(window, maximized);
        }
    }

static void dispatchFramebufferSize(FRAMEBUFFER_SIZE_HANDLER_ARGS) {
        if (InputHandler* instance = getInstance(window)) {
            if (instance->m_framebuffer_size_callback)
                instance->m_framebuffer_size_callback(window, width, height);
            else
                instance->handleFramebufferSize(window, width, height);
        }
    }

    // --- Global Dispatcher ---
    static void dispatchJoystick(JOYSTICK_HANDLER_ARGS) {
        if (s_joystick_callback) {
            s_joystick_callback(jid, event);
        }
        // No virtual fallback here, as it's a static/global event
        // with no 'this' instance.
    }
};

} // namespace input

#endif // INPUT_HANDLER_H