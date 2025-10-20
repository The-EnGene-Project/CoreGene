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

#include "gl_includes.h"
#include "error.h"
#include "uniforms/uniform.h"
#include "../exceptions/shader_exception.h"

namespace shader {

class Shader;
using ShaderPtr = std::shared_ptr<Shader>;

class ShaderStack;
using ShaderStackPtr = std::shared_ptr<ShaderStack>;

using UniformProviderMap = std::unordered_map<std::string, std::any>;

static const char* GLenumToString(GLenum type);

class Shader : public std::enable_shared_from_this<Shader> {
private:
    unsigned int m_pid;
    // Mapa para armazenar os uniformes configurados
    std::unordered_map<std::string, uniform::UniformInterfacePtr> m_uniforms;
    mutable bool m_uniforms_validated = false;

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
        GLenum type;

        for (GLint i = 0; i < active_uniforms; ++i) {
            glGetActiveUniform(m_pid, (GLuint)i, bufSize, &length, &size, &type, name);

            std::string uniform_name(name, length);

            // Ignore built-in OpenGL uniforms which start with "gl_"
            if (uniform_name.rfind("gl_", 0) == 0) {
                continue;
            }

            // Check if the active uniform exists in our configured map
            if (m_uniforms.find(uniform_name) == m_uniforms.end()) {
                std::cerr << "Warning: Active uniform '" << uniform_name  
                          << "' (type: " << GLenumToString(type) << ")"
                          << "' is declared in the shader but not configured in the C++ code." 
                          << std::endl;
            }
        }
        m_uniforms_validated = true;
    }

public:
    static ShaderPtr Make() {
        return ShaderPtr(new Shader());
    }

    // This factory function now works with both paths and source code.
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
        shader->Link();

        for (const auto& [name, any_provider] : uniforms) {
            // Verifica e configura cada tipo de uniforme suportado
            if (const auto* provider = std::any_cast<std::function<float()>>(&any_provider)) {
                shader->configureUniform<float>(name, *provider);
            } 
            else if (const auto* provider = std::any_cast<std::function<int()>>(&any_provider)) {
                shader->configureUniform<int>(name, *provider);
            }
            else if (const auto* provider = std::any_cast<std::function<glm::vec2()>>(&any_provider)) {
                shader->configureUniform<glm::vec2>(name, *provider);
            }
            else if (const auto* provider = std::any_cast<std::function<glm::vec3()>>(&any_provider)) {
                shader->configureUniform<glm::vec3>(name, *provider);
            }
            else if (const auto* provider = std::any_cast<std::function<glm::vec4()>>(&any_provider)) {
                shader->configureUniform<glm::vec4>(name, *provider);
            }
            else if (const auto* provider = std::any_cast<std::function<glm::mat3()>>(&any_provider)) {
                shader->configureUniform<glm::mat3>(name, *provider);
            }
            else if (const auto* provider = std::any_cast<std::function<glm::mat4()>>(&any_provider)) {
                shader->configureUniform<glm::mat4>(name, *provider);
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

    unsigned int GetShaderID() const {
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
            identifier = "Vertex Shader (from string)";
            source_code = source_or_path;
        }

        GLuint sid = compileShaderFromSource(GL_VERTEX_SHADER, source_code, identifier);
        glAttachShader(m_pid, sid);
        glDeleteShader(sid); // The shader object is no longer needed after attachment
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
            identifier = "Fragment Shader (from string)";
            source_code = source_or_path;
        }
        
        GLuint sid = compileShaderFromSource(GL_FRAGMENT_SHADER, source_code, identifier);
        glAttachShader(m_pid, sid);
        glDeleteShader(sid); // The shader object is no longer needed after attachment
    }

    /**
     * @brief Links the shader program.
     * @throws exception::ShaderException on linking failure.
     */
    void Link() {
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
    }

    // Configura um uniforme dinâmico
    template<typename T>
    ShaderPtr configureUniform(const std::string& name, std::function<T()> value_provider) {
        // 1. Cria o objeto Uniform.
        auto uniform_obj = uniform::Uniform<T>::Make(name, std::move(value_provider));
        
        // 2. Pede ao objeto para encontrar sua própria localização neste shader.
        uniform_obj->findLocation(m_pid);
        
        // 3. Armazena o uniforme configurado no mapa.
        m_uniforms[name] = std::move(uniform_obj);

        return shared_from_this();
    }

    // Aplica todos os uniformes configurados para este shader.
    void applyUniforms() const {
        for (const auto& pair : m_uniforms) {
            pair.second->apply();
        }
    }

    // Ativa o shader e aplica seus uniformes.
    void UseProgram() const {
        // É crucial usar o programa ANTES de enviar os valores dos uniformes.
        glUseProgram(m_pid);
        validateUniforms();
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

class ShaderStack { // singleton
    // BACALHAU falta adaptar para ter o batching dos comandos de draw pelo shader
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
    ShaderPtr top() {
        ShaderPtr current_shader = stack.back();
        if (current_shader != last_used_shader) {
            current_shader->UseProgram();
            last_used_shader = current_shader;
        }
        current_shader->applyUniforms();
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

// static GLuint educationalMakeShader(GLenum shadertype, const std::string& filename) {

//     GLuint id = glCreateShader(shadertype);

//     // open the shader file
//     std::ifstream fp;
//     fp.open(filename); 

//     // read the shader file content
//     std::stringstream strStream;
//     strStream << fp.rdbuf();

//     // pass the source string to OpenGL
//     std::string source = strStream.str();
//     const char* csource = source.c_str();
//     glShaderSource(id, 1, &csource, 0);

//     // tell OpenGL to compile the shader
//     GLint status;
//     glCompileShader(id);
//     glGetShaderiv(id, GL_COMPILE_STATUS, &status);

//     return id;
// }

} // namespace shader

#endif