#ifndef ENGENE_H
#define ENGENE_H
#pragma once

#define GLAD_GL_IMPLEMENTATION // Necessary for header-only version.
#include "gl_base/gl_includes.h"
#include "gl_base/input_handler.h"
#include "gl_base/shader.h"
#include "core/EnGene_config.h"

#include <iostream>
#include <string>
#include <stdexcept>
#include <functional>
#include <memory>

namespace engene {

class EnGene {
public:
    /**
     * @brief Constructs the EnGene application engine from a configuration struct.
     *
     * This constructor initializes GLFW, creates a window, sets up an OpenGL context,
     * and prepares the main loop.
     *
     * @param config A struct containing all setup parameters for the engine.
     * @param handler A pointer to an InputHandler instance. The EnGene will take ownership of this pointer.
     * @param on_initialize A function (often a lambda) that takes a reference to the EnGene app for setup.
     * @param on_update A function (often a lambda) containing the user's per-frame drawing and update code.
     */
    EnGene(
        const EnGeneConfig& config,
        input::InputHandler* handler,
        std::function<void(EnGene&)> on_initialize, // MODIFIED: Signature now passes the app reference
        std::function<void(double)> on_update
    ) : m_width(config.width),
        m_height(config.height),
        m_title(config.title),
        m_input_handler(handler), // Takes ownership of the handler
        m_user_initialize_func(on_initialize),
        m_user_update_func(on_update)
    {
        if (!handler) {
            throw std::runtime_error("InputHandler cannot be null.");
        }
        if (config.base_vertex_shader_path.empty() || config.base_fragment_shader_path.empty()) {
            throw std::runtime_error("Base shader paths were not set in EnGeneConfig.");
        }
        
        m_update_interval = 1.0 / static_cast<double>(config.maxFramerate);
        
        initialize_window();

        // creates the shader from the paths in the config.
        m_base_shader = shader::Shader::Make();
        m_base_shader->AttachVertexShader(config.base_vertex_shader_path);
        m_base_shader->AttachFragmentShader(config.base_fragment_shader_path);
        m_base_shader->Link();

        // Copy clear color from config
        m_clear_color[0] = config.clearColor[0];
        m_clear_color[1] = config.clearColor[1];
        m_clear_color[2] = config.clearColor[2];
        m_clear_color[3] = config.clearColor[3];

        // Set up default OpenGL state
        glClearColor(m_clear_color[0], m_clear_color[1], m_clear_color[2], m_clear_color[3]);
        glEnable(GL_DEPTH_TEST);
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
     * @brief Provides access to the engine's base shader.
     * Useful for configuring uniforms during initialization.
     */
    shader::ShaderPtr getBaseShader() {
        return m_base_shader;
    }

    /**
     * @brief Starts the main application loop.
     * This function will not return until the user closes the window.
     */
    void run() {
        // Call the user's one-time setup code, passing a reference to this app.
        if (m_user_initialize_func) {
            m_user_initialize_func(*this);
        }

        double saved_time = glfwGetTime();
        double update_timer = 0.0;

        while (!glfwWindowShouldClose(m_window)) {
            double current_time = glfwGetTime();
            double elapsed_time = current_time - saved_time;
            saved_time = current_time;
            
            update_timer += elapsed_time;

            if (update_timer >= m_update_interval) {
                shader::stack()->push(m_base_shader);
                
                if (m_user_update_func) {
                    m_user_update_func(m_update_interval);
                }

                shader::stack()->pop();

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
    shader::ShaderPtr m_base_shader;
    std::unique_ptr<input::InputHandler> m_input_handler;
    double m_update_interval;
    float m_clear_color[4];

    // User-provided functions
    std::function<void(EnGene&)> m_user_initialize_func;
    std::function<void(double)> m_user_update_func;

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
