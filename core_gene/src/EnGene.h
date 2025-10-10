#ifndef ENGENE_H
#define ENGENE_H
#pragma once

#define GLAD_GL_IMPLEMENTATION // Necessary for header-only version.
#include "gl_base/gl_includes.h"
#include "gl_base/input_handler.h"

#include <iostream>
#include <string>
#include <stdexcept>
#include <functional>
#include <memory>

namespace engene {

class EnGene {
public:
    /**
     * @brief Constructs the EnGene application engine.
     *
     * This constructor initializes GLFW, creates a window, sets up an OpenGL context,
     * and prepares the main loop.
     *
     * @param width The width of the window in pixels.
     * @param height The height of the window in pixels.
     * @param title The title of the window.
     * @param handler A pointer to an InputHandler instance. The EnGene will take ownership of this pointer.
     * @param on_initialize A function (often a lambda) that contains the user's one-time setup code.
     * @param on_update A function (often a lambda) that contains the user's drawing and update code, called every frame.
     * @param max_framerate The target maximum framerate for the application.
     */
    EnGene(
        int width,
        int height,
        const std::string& title,
        input::InputHandler* handler,
        std::function<void()> on_initialize,
        std::function<void()> on_update,
        int max_framerate = 60,
        float clear_color[4] = nullptr
    ) : m_width(width),
        m_height(height),
        m_title(title),
        m_input_handler(handler), // Takes ownership of the handler
        m_user_initialize_func(on_initialize),
        m_user_update_func(on_update)
    {
        if (!handler) {
            throw std::runtime_error("InputHandler cannot be null.");
        }
        m_update_interval = 1.0 / static_cast<double>(max_framerate);
        initialize_window();

        if (clear_color) {
            m_clear_color[0] = clear_color[0];
            m_clear_color[1] = clear_color[1];
            m_clear_color[2] = clear_color[2];
            m_clear_color[3] = clear_color[3];
        } else {
            m_clear_color[0] = 0.1f;
            m_clear_color[1] = 0.1f;
            m_clear_color[2] = 0.1f;
            m_clear_color[3] = 1.0f;
        }

        // Set up default OpenGL state
        glClearColor(m_clear_color[0], m_clear_color[1], m_clear_color[2], m_clear_color[3]);
        glEnable(GL_DEPTH_TEST); // Enable depth testing for 3D
    }

    ~EnGene() {
        if (m_window) {
            glfwTerminate();
        }
    }

    // Disable copying
    EnGene(const EnGene&) = delete;
    EnGene& operator=(const EnGene&) = delete;

    /**
     * @brief Starts the main application loop.
     * This function will not return until the user closes the window.
     */
    void run() {
        // Call the user's one-time setup code
        if (m_user_initialize_func) {
            m_user_initialize_func();
        }

        double saved_time = glfwGetTime();
        double update_timer = 0.0;

        while (!glfwWindowShouldClose(m_window)) {
            double current_time = glfwGetTime();
            double elapsed_time = current_time - saved_time;
            saved_time = current_time;
            
            update_timer += elapsed_time;

            if (update_timer >= m_update_interval) {
                // Call the user's per-frame update/draw code
                if (m_user_update_func) {
                    m_user_update_func();
                }

                glfwSwapBuffers(m_window);
                glfwPollEvents();
                
                // Reset timer. Subtracting interval helps maintain a more stable framerate
                // in case of a long frame, rather than just setting to 0.
                update_timer -= m_update_interval;
            }
        }
    }

private:
    // Member variables
    int m_width;
    int m_height;
    std::string m_title;
    GLFWwindow* m_window = nullptr;
    std::unique_ptr<input::InputHandler> m_input_handler;
    double m_update_interval;
    float m_clear_color[4];

    // User-provided functions
    std::function<void()> m_user_initialize_func;
    std::function<void()> m_user_update_func;

    /**
     * @brief Handles all the boilerplate for setting up GLFW, GLAD, and the window.
     */
    void initialize_window() {
        glfwSetErrorCallback(glfw_error_callback);

        if (!glfwInit()) {
            throw std::runtime_error("Could not initialize GLFW");
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

        m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
        if (!m_window) {
            glfwTerminate();
            throw std::runtime_error("Could not create GLFW window");
        }
        glfwMakeContextCurrent(m_window);

        if (!gladLoadGL(glfwGetProcAddress)) {
            throw std::runtime_error("Failed to initialize GLAD OpenGL context");
        }

        // Apply the user-provided input handler
        m_input_handler->applyCallbacks(m_window);
    }

    /**
     * @brief Static callback for GLFW to report errors.
     */
    static void glfw_error_callback(int code, const char* msg) {
        std::cerr << "GLFW error " << code << ": " << msg << std::endl;
    }
};

} // namespace engene
#endif // ENGENE_H
