#ifndef SHADER_H
#define SHADER_H
#pragma once

#include <cstdlib>
#include <memory>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <any>
#include <variant>

#include "gl_includes.h"
#include "i_shader.h"
#include "error.h"
#include "uniforms/uniform.h"
#include "uniforms/pending_uniform_command.h"
#include "uniforms/global_resource_manager.h"
#include "../exceptions/shader_exception.h"


namespace shader {

class Shader;
using ShaderPtr = std::shared_ptr<Shader>;

class ShaderStack;
using ShaderStackPtr = std::shared_ptr<ShaderStack>;

using UniformProviderMap = std::unordered_map<std::string, std::any>;

static const char* GLenumToString(GLenum type);

/**
 * @class Shader
 * @brief Manages GLSL shader programs, including a 4-tier uniform system.
 *
 * Tier 1: Global Resources (UBOs) - Bound once at link time.
 * Tier 2: Static Uniforms (Per-Use) - Applied when shader is activated.
 * Tier 3: Dynamic Uniforms (Per-Draw) - Applied every draw call via ShaderStack.
 * Tier 4: Immediate Uniforms (Manual) - Set directly via setUniform for one-off values.
 */
class Shader : public IShader, public std::enable_shared_from_this<Shader> {
    friend class ShaderStack; // ShaderStack needs to manage m_is_currently_active_in_GL

private:
    unsigned int m_pid;
    bool m_is_dirty = true;
    mutable bool m_uniforms_validated = false;

    // --- 4-Tier Uniform System ---
    // Tier 1: Global Resources (UBOs) to bind at link time.
    std::vector<std::string> m_resource_blocks_to_bind;

    // Tier 2: Static uniforms, applied when the shader is made active.
    std::unordered_map<std::string, uniform::UniformInterfacePtr> m_static_uniforms;

    // Tier 3: Dynamic uniforms, applied per-draw.
    std::unordered_map<std::string, uniform::UniformInterfacePtr> m_dynamic_uniforms;

    // Tier 4: State and queue for immediate-mode uniforms.
    bool m_is_currently_active_in_GL = false;
    std::vector<uniform::PendingUniformCommand> m_pending_uniform_queue;

    /**
     * @brief Helper to check if a string is likely a file path.
     * @details A simple heuristic: if it lacks newlines and GLSL keywords, it's treated as a path.
     */
    static bool isLikelyFilePath(const std::string& source) {
        // If it contains a newline or a GLSL keyword, it's almost certainly source code.
        if (source.find('\n') != std::string::npos ||
            source.find("#version") != std::string::npos ||
            source.find("void main") != std::string::npos) {
            return false;
        }
        // Otherwise, assume it's a path. A more robust check might look for extensions.
        return true;
    }

    /**
     * @brief Helper to load shader source code from a file.
     * @throws exception::ShaderException if the file cannot be opened.
     */
    static std::string loadSourceFromFile(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            throw exception::ShaderException("Could not open shader file: " + filepath);
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    /**
     * @brief Core shader compilation function.
     * @param shadertype GL_VERTEX_SHADER or GL_FRAGMENT_SHADER.
     * @param source The actual GLSL source code.
     * @param identifier A name for the shader (e.g., file path) for use in error messages.
     * @return The OpenGL ID of the compiled shader.
     * @throws exception::ShaderException on compilation failure.
     */
    static GLuint compileShaderFromSource(GLenum shadertype, const std::string& source, const std::string& identifier) {
        GLuint id = glCreateShader(shadertype);
        if (id == 0) {
            throw exception::ShaderException("Could not create shader object.");
        }

        const char* c_source = source.c_str();
        glShaderSource(id, 1, &c_source, nullptr);
        glCompileShader(id);

        GLint status;
        glGetShaderiv(id, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE) {
            GLint len;
            glGetShaderiv(id, GL_INFO_LOG_LENGTH, &len);
            std::vector<char> message(len);
            glGetShaderInfoLog(id, len, nullptr, message.data());
            glDeleteShader(id); // Clean up the failed shader
            throw exception::ShaderException("Failed to compile shader '" + identifier + "':\n" + message.data());
        }

        return id;
    }

    /**
     * @brief [Tier 4] Sends all queued immediate-mode uniforms to OpenGL.
     * This is called automatically when the shader is activated.
     */
    void flushPendingUniforms() {
        if (m_pending_uniform_queue.empty()) {
            return;
        }
        for (const auto& command : m_pending_uniform_queue) {
            command.Execute(m_pid);
        }
        m_pending_uniform_queue.clear();
    }
    
    /**
     * @brief [Tier 4] Internal helper to immediately set a uniform value.
     * This is the fast-path for when the shader is already active.
     */
    template<typename T>
    void _setUniform(GLint location, const T& value) {
        if constexpr (std::is_same_v<T, int>) {
            glUniform1i(location, value);
        } else if constexpr (std::is_same_v<T, float>) {
            glUniform1f(location, value);
        } else if constexpr (std::is_same_v<T, glm::vec2>) {
            glUniform2fv(location, 1, glm::value_ptr(value));
        } else if constexpr (std::is_same_v<T, glm::vec3>) {
            glUniform3fv(location, 1, glm::value_ptr(value));
        } else if constexpr (std::is_same_v<T, glm::vec4>) {
            glUniform4fv(location, 1, glm::value_ptr(value));
        } else if constexpr (std::is_same_v<T, glm::mat3>) {
            glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
        } else if constexpr (std::is_same_v<T, glm::mat4>) {
            glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
        }
    }


protected:
    Shader() : m_pid(-1) {}

    void initialize() {
        if (m_pid != static_cast<unsigned int>(-1)) {
            return;
        }
        m_pid = glCreateProgram();
        if (m_pid == 0) {
            throw exception::ShaderException("Could not create shader program object.");
        }
    }

    void validateUniforms() const {
        if (m_uniforms_validated) return;
        GLint active_uniforms = 0;
        glGetProgramiv(m_pid, GL_ACTIVE_UNIFORMS, &active_uniforms);
        
        // Buffer for uniform names
        const GLsizei bufSize = 256; 
        GLchar name[bufSize]; 
        GLsizei length; 
        GLint size; 
        GLenum glsl_type;

        for (GLint i = 0; i < active_uniforms; ++i) {
            glGetActiveUniform(m_pid, (GLuint)i, bufSize, &length, &size, &glsl_type, name);

            std::string uniform_name(name, length);

            // Ignore built-in OpenGL uniforms which start with "gl_"
            if (uniform_name.rfind("gl_", 0) == 0) {
                continue;
            }
            
            // Check against all configured uniform maps
            uniform::UniformInterface* configured_uniform = nullptr;
            // Try to find in static uniforms first
            auto static_it = m_static_uniforms.find(uniform_name);
            if (static_it != m_static_uniforms.end()) {
                configured_uniform = static_it->second.get(); // Get the value (pointer)
            } else {
                // If not found, try to find in dynamic uniforms
                auto dynamic_it = m_dynamic_uniforms.find(uniform_name);
                if (dynamic_it != m_dynamic_uniforms.end()) {
                    configured_uniform = dynamic_it->second.get(); // Get raw ptr from unique_ptr
                }
            }

            if (configured_uniform) {
                // This uniform IS configured in C++.
                // Your "Dead Uniform" check in `findLocation` already caught any with location == -1.
                // Now, we just check for type mismatches.
                GLenum cpp_type = configured_uniform->getCppType();

                // Note: GL_NONE means our C++ type trait didn't recognize the type.
                if (cpp_type != GL_NONE && cpp_type != glsl_type) {
                    // TYPE MISMATCH!
                    std::cerr << "Warning: Uniform type mismatch for '" << uniform_name << "'. "
                            << "GLSL expects type [" << GLenumToString(glsl_type) << "] but "
                            << "C++ is configured as [" << GLenumToString(cpp_type) << "]."
                            << std::endl;
                }
            } else {
                 std::cout << "Info: Active uniform '" << uniform_name  
                           << "' (type: " << GLenumToString(glsl_type) << ")"
                           << "' is in the shader but not configured as a static or dynamic uniform." 
                           << " (This may be intentional for immediate-mode uniforms)."
                           << std::endl;
            }
        }
        m_uniforms_validated = true;
    }

public:
    static ShaderPtr Make() {
        return ShaderPtr(new Shader());
    }

    // This factory function works with both paths and source code.
    // Creates, attaches, links and configures uniforms all at once.
    static ShaderPtr Make(
        const std::string& vertex_source,
        const std::string& fragment_source,
        const UniformProviderMap& uniforms = {})
    {
        ShaderPtr shader = ShaderPtr(new Shader());
        shader->initialize();
        shader->AttachVertexShader(vertex_source);
        shader->AttachFragmentShader(fragment_source);
        shader->Bake();

        for (const auto& [name, any_provider] : uniforms) {
            if (const auto* provider = std::any_cast<std::function<float()>>(&any_provider)) {
                shader->configureDynamicUniform<float>(name, *provider);
            } 
            else if (const auto* provider = std::any_cast<std::function<int()>>(&any_provider)) {
                shader->configureDynamicUniform<int>(name, *provider);
            }
            else if (const auto* provider = std::any_cast<std::function<glm::vec2()>>(&any_provider)) {
                shader->configureDynamicUniform<glm::vec2>(name, *provider);
            }
            else if (const auto* provider = std::any_cast<std::function<glm::vec3()>>(&any_provider)) {
                shader->configureDynamicUniform<glm::vec3>(name, *provider);
            }
            else if (const auto* provider = std::any_cast<std::function<glm::vec4()>>(&any_provider)) {
                shader->configureDynamicUniform<glm::vec4>(name, *provider);
            }
            else if (const auto* provider = std::any_cast<std::function<glm::mat3()>>(&any_provider)) {
                shader->configureDynamicUniform<glm::mat3>(name, *provider);
            }
            else if (const auto* provider = std::any_cast<std::function<glm::mat4()>>(&any_provider)) {
                shader->configureDynamicUniform<glm::mat4>(name, *provider);
            }
            else {
                std::cerr << "Warning: Uniform '" << name << "' has an unsupported type in Make function." << std::endl;
            }
        }

        return shader;
    }

    virtual ~Shader() {
        if (m_pid > 0) {
            glDeleteProgram(m_pid);
        }
    }

    virtual GLuint GetShaderID() const override {
        return m_pid;
    }

    /**
     * @brief Attaches a vertex shader from a file path or a string literal.
     */
    void AttachVertexShader(const std::string& source_or_path) {
        initialize();
        std::string source_code;
        std::string identifier;

        if (isLikelyFilePath(source_or_path)) {
            identifier = source_or_path;
            source_code = loadSourceFromFile(source_or_path);
        } else {
            identifier = "Vertex Shader (from string)"; // TODO: BACALHAU make this not repeat for multiple string shaders
            source_code = source_or_path;
        }

        GLuint sid = compileShaderFromSource(GL_VERTEX_SHADER, source_code, identifier);
        glAttachShader(m_pid, sid);
        glDeleteShader(sid); // The shader object is no longer needed after attachment
        m_is_dirty = true; // [Suggestion 1] Mark as dirty
    }

    /**
     * @brief Attaches a fragment shader from a file path or a string literal.
     */
    void AttachFragmentShader(const std::string& source_or_path) {
        initialize();
        std::string source_code;
        std::string identifier;

        if (isLikelyFilePath(source_or_path)) {
            identifier = source_or_path;
            source_code = loadSourceFromFile(source_or_path);
        } else {
            identifier = "Fragment Shader (from string)"; // TODO: BACALHAU make this not repeat for multiple string shaders
            source_code = source_or_path;
        }
        
        GLuint sid = compileShaderFromSource(GL_FRAGMENT_SHADER, source_code, identifier);
        glAttachShader(m_pid, sid);
        glDeleteShader(sid); // The shader object is no longer needed after attachment
        m_is_dirty = true; // [Suggestion 1] Mark as dirty
    }

    /**
     * @brief Internal "Just-in-Time" linking and configuration.
     * This is the single authoritative source for linking and (re)configuring uniforms.
     * @throws exception::ShaderException on linking failure.
     */
    void Bake() {
        // 1. Check if we're already baked
        if (!m_is_dirty) {
            return;
        }

        // 2. Link the program
        glLinkProgram(m_pid);
        Error::Check("link program");

        GLint status;
        glGetProgramiv(m_pid, GL_LINK_STATUS, &status);
        if (status == GL_FALSE) {
            GLint len;
            glGetProgramiv(m_pid, GL_INFO_LOG_LENGTH, &len);
            std::vector<char> message(len);
            glGetProgramInfoLog(m_pid, len, 0, message.data());
            throw exception::ShaderException("Shader linking failed: " + std::string(message.data()));
        }

        // 3. Bind Tier 1 (Global Resources)
        for (const auto& block_name : m_resource_blocks_to_bind) {
            uniform::manager().bindResourceToShader(m_pid, block_name);
        }

        // 4. CRITICAL: Re-find all Tier 2 & 3 uniform locations
        // This fixes the stale location bug
        for (auto& [name, uniform_ptr] : m_static_uniforms) {
            uniform_ptr->findLocation(m_pid);
        }
        for (auto& [name, uniform_ptr] : m_dynamic_uniforms) {
            uniform_ptr->findLocation(m_pid);
        }

        // 5. Mark as clean
        m_is_dirty = false;
        m_uniforms_validated = false; // Force re-validation on next use
    }

    // --- Tier 1: Global Resource Configuration ---
    void addResourceBlockToBind(const std::string& block_name) {
        m_resource_blocks_to_bind.push_back(block_name);
        m_is_dirty = true;
    }

    // --- Tier 2: Static Uniform Configuration & Application ---
    template<typename T>
    ShaderPtr configureStaticUniform(const std::string& name, std::function<T()> value_provider) {
        // 1. Creates the Uniform object
        auto uniform_obj = uniform::Uniform<T>::Make(name, std::move(value_provider));
        // 2. Asks the object to find it's location in the shader
        uniform_obj->findLocation(m_pid);
        // 3. Stores the configured Uniform in the static uniform map
        m_static_uniforms[name] = std::move(uniform_obj);
        return shared_from_this();
    }

    void applyStaticUniforms() const {
        for (const auto& [name, uniform_ptr] : m_static_uniforms) {
            uniform_ptr->apply();
        }
    }

    // --- Tier 3: Dynamic Uniform Configuration & Application ---
    template<typename T>
    ShaderPtr configureDynamicUniform(const std::string& name, std::function<T()> value_provider) {
        // 1. Creates the Uniform object
        auto uniform_obj = uniform::Uniform<T>::Make(name, std::move(value_provider));
        // 2. Asks the object to find it's location in the shader
        uniform_obj->findLocation(m_pid);
        // 3. Stores the configured Uniform in the dynamic uniform map
        m_dynamic_uniforms[name] = std::move(uniform_obj);
        return shared_from_this();
    }

    void applyDynamicUniforms() const {
        for (const auto& [name, uniform_ptr] : m_dynamic_uniforms) {
            uniform_ptr->apply();
        }
    }
    
    // --- Tier 4: Immediate-Mode Uniforms ---
    template<typename T>
    void setUniform(const std::string& name, const T& value) {

        if (m_is_currently_active_in_GL) {
            GLint location = glGetUniformLocation(m_pid, name.c_str());
            if (location == -1) {
                // This is not necessarily an error, the uniform might be optimized out.
                // For production, this might be silenced.
                std::cerr << "Warning: Uniform '" << name << "' not found in shader." << std::endl;
                return;
            }
            // Fast path: Shader is active, set uniform directly.
            _setUniform(location, value);
        } else {
            // Slow path: Shader is inactive, queue the command.
            m_pending_uniform_queue.emplace_back(uniform::PendingUniformCommand{name, value});
        }
    }

    // --- Shader Activation ---
    void UseProgram() {
        if (m_is_dirty) {
            Bake();
        }
        glUseProgram(m_pid);
        m_is_currently_active_in_GL = true; // Set active flag
        validateUniforms();
        applyStaticUniforms();  // Apply Tier 2 uniforms
        flushPendingUniforms(); // Apply any queued Tier 4 uniforms
    }
};

static const char* GLenumToString(GLenum type) {
    switch (type) {
        case GL_FLOAT:      return "float";
        case GL_FLOAT_VEC2: return "vec2";
        case GL_FLOAT_VEC3: return "vec3";
        case GL_FLOAT_VEC4: return "vec4";
        case GL_INT:        return "int";
        case GL_INT_VEC2:   return "ivec2";
        case GL_INT_VEC3:   return "ivec3";
        case GL_INT_VEC4:   return "ivec4";
        case GL_BOOL:       return "bool";
        case GL_BOOL_VEC2:  return "bvec2";
        case GL_BOOL_VEC3:  return "bvec3";
        case GL_BOOL_VEC4:  return "bvec4";
        case GL_FLOAT_MAT2: return "mat2";
        case GL_FLOAT_MAT3: return "mat3";
        case GL_FLOAT_MAT4: return "mat4";
        case GL_SAMPLER_2D: return "sampler2D";
        case GL_SAMPLER_CUBE: return "samplerCube";
        default:            return "Unknown Type";
    }
}

class ShaderStack {
    // TODO: BACALHAU still needs to be adapted for shader switch batching
private:
    std::vector<ShaderPtr> stack;
    ShaderPtr last_used_shader;

    ShaderStack() {
        stack.push_back(Shader::Make());
    }
    
    friend ShaderStackPtr stack();
public:
    ShaderStack(const ShaderStack&) = delete;
    ShaderStack& operator=(const ShaderStack&) = delete;
    ~ShaderStack() = default;

    void push(ShaderPtr shader) {
        stack.push_back(shader);
    }
    void pop() {
        if (stack.size() > 1) {
            stack.pop_back();
        } else {
            std::cerr << "Warning: Attempt to pop the base shader from the shader stack." << std::endl;
        }
    }
    
    /**
     * @brief Gets the currently active shader, managing state transitions.
     * @return A shared pointer to the active shader.
     */
    ShaderPtr top() {
        ShaderPtr current_shader = stack.back();

        // State transition logic
        if (current_shader != last_used_shader) {
            // Deactivate the previous shader if it exists
            if (last_used_shader) {
                last_used_shader->m_is_currently_active_in_GL = false;
            }
            // Activate the new shader (this also Bakes if dirty, and applies Tier 2 and Tier 4 uniforms)
            current_shader->UseProgram();
            last_used_shader = current_shader;
        }

        // Apply per-draw (Tier 3) uniforms
        current_shader->applyDynamicUniforms();
        
        return current_shader;
    }

    unsigned int topId() {
        return top()->GetShaderID();
    }
    ShaderPtr getLastUsedShader() const {
        return last_used_shader;
    }
};

inline ShaderStackPtr stack() {
    static ShaderStackPtr instance = ShaderStackPtr(new ShaderStack());
    return instance;
}

} // namespace shader

#endif // SHADER_H