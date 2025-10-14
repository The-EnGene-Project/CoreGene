#ifndef ENGENE_H
#define ENGENE_H
#pragma once

#define GLAD_GL_IMPLEMENTATION // Necessary for header-only version.
#include "gl_base/gl_includes.h"
#include "gl_base/input_handler.h"
#include "gl_base/shader.h"
#include "gl_base/transform.h"
#include "core/EnGene_config.h"
#include "exceptions/base_exception.h"

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
     * @param on_initialize A function that runs once for setup.
     * @param on_fixed_update A function for simulation logic, called at a fixed rate. It receives the fixed delta time.
     * @param on_render A function for all drawing calls, called once per frame after the simulation catches up.
     * @param config A struct containing setup parameters for the engine.
     * @param handler A pointer to an InputHandler instance. The EnGene will take ownership of this pointer.
     */
    EnGene(
        std::function<void(EnGene&)> on_initialize,
        std::function<void(double)> on_fixed_update,
        std::function<void(double)> on_render,
        EnGeneConfig config = {},
        input::InputHandler* handler = nullptr
    ) : m_width(config.width),
        m_height(config.height),
        m_title(config.title),
        m_fixed_timestep(1.0 / static_cast<double>(config.updatesPerSecond)),
        m_max_frame_time(config.maxFrameTime),
        m_input_handler(handler ? 
            std::unique_ptr<input::InputHandler>(handler) :
            std::make_unique<input::InputHandler>()
        ),
        m_user_initialize_func(on_initialize),
        m_user_fixed_update_func(on_fixed_update),
        m_user_render_func(on_render)
    {
        if (config.base_vertex_shader_source.empty() || config.base_fragment_shader_source.empty()) {
            throw exception::EnGeneException("Base shader paths were not set in EnGeneConfig.");
        }
        
        m_fixed_timestep = 1.0 / static_cast<double>(config.updatesPerSecond);
        
        initialize_window();

        m_base_shader = shader::Shader::Make();
        m_base_shader->AttachVertexShader(config.base_vertex_shader_source);
        m_base_shader->AttachFragmentShader(config.base_fragment_shader_source);
        m_base_shader->Link();

        if (config.base_vertex_shader_source == EnGeneConfig::DEFAULT_VERTEX_SHADER) {
            m_base_shader->configureUniform<glm::mat4>("M", transform::current);
        }

        m_clear_color[0] = config.clearColor[0];
        m_clear_color[1] = config.clearColor[1];
        m_clear_color[2] = config.clearColor[2];
        m_clear_color[3] = config.clearColor[3];

        glClearColor(m_clear_color[0], m_clear_color[1], m_clear_color[2], m_clear_color[3]);
        glEnable(GL_DEPTH_TEST);
    }

    ~EnGene() {
        if (m_window) {
            glfwTerminate();
        }
    }

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
     * Implements a fixed-timestep simulation loop to decouple simulation from rendering framerate.
     */
    void run() {
        // Call the user's one-time setup code, passing a reference to this app.
        if (m_user_initialize_func) {
            m_user_initialize_func(*this);
        }

        double last_time = glfwGetTime();
        double accumulator = 0.0;

        while (!glfwWindowShouldClose(m_window)) {
            double current_time = glfwGetTime();
            double elapsed_time = current_time - last_time;
            last_time = current_time;
            
            // Prevents a "spiral of death" if updates take too long by capping elapsed time.
            if (elapsed_time > m_max_frame_time) {
                elapsed_time = m_max_frame_time;
            }

            accumulator += elapsed_time;

            // Performs fixed updates to catch the simulation up to the current time.
            while (accumulator >= m_fixed_timestep) {
                if (m_user_fixed_update_func) {
                    m_user_fixed_update_func(m_fixed_timestep);
                }
                accumulator -= m_fixed_timestep;
            }

            // This is the percentage of the way we are into the *next* simulation step.
            const double alpha = accumulator / m_fixed_timestep;


            // Render the single, most recent state.
            // All drawing code now belongs in the render callback.
            shader::stack()->push(m_base_shader);
            
            if (m_user_render_func) {
                m_user_render_func(alpha);
            }

            shader::stack()->pop();

            glfwSwapBuffers(m_window);
            glfwPollEvents();
        }
    }

private:
    int m_width;
    int m_height;
    std::string m_title;
    GLFWwindow* m_window = nullptr;
    shader::ShaderPtr m_base_shader;
    std::unique_ptr<input::InputHandler> m_input_handler;
    double m_fixed_timestep; // This is the duration of one simulation step.
    double m_max_frame_time;
    float m_clear_color[4];

    // User-provided functions
    std::function<void(EnGene&)> m_user_initialize_func;
    std::function<void(double)> m_user_fixed_update_func; // For physics/simulation
    std::function<void(double)> m_user_render_func;

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