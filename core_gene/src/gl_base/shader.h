#ifndef SHADER_H
#define SHADER_H
#pragma once

#include <memory>
#include <fstream>
#include <iostream>
#include <sstream> 
#include <cstdlib>
#include <vector>
#include <unordered_map>
#include <string>
#include <functional>
#include "gl_includes.h"
#include "error.h"
#include "uniform.h"

namespace shader {

class Shader;
using ShaderPtr = std::shared_ptr<Shader>;

class ShaderStack;
using ShaderStackPtr = std::shared_ptr<ShaderStack>;

static GLuint MakeShader(GLenum shadertype, const std::string& filename) {
    
    GLuint id = glCreateShader(shadertype);
    Error::Check("create shader");

    // errorcheck
    if (id == 0) {
        std::cerr << "Could not create shader object";
        exit(1);
    }

    // open the shader file
    std::ifstream fp;
    fp.open(filename); 


    // errorcheck
    if (!fp.is_open()) {
        std::cerr << "Could not open file: " << filename << std::endl;
        exit(1);
    } 

    // read the shader file content
    std::stringstream strStream;
    strStream << fp.rdbuf();

    // pass the source string to OpenGL
    std::string source = strStream.str();
    const char* csource = source.c_str();
    glShaderSource(id, 1, &csource, 0);
    Error::Check("set shader source");

    // tell OpenGL to compile the shader
    GLint status;
    glCompileShader(id);
    glGetShaderiv(id, GL_COMPILE_STATUS, &status);
    Error::Check("compile shader");

    // errorcheck
    if (!status) {
        GLint len;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &len);
        char* message = new char[len];
        glGetShaderInfoLog(id, len, 0, message);
        std::cerr << filename << ":" << std::endl << message << std::endl;
        delete [] message;
        exit(1);
    }

    return id;
}

class Shader {
private:
    unsigned int m_pid;
    // Mapa para armazenar os uniformes configurados
    std::unordered_map<std::string, UniformInterfacePtr> m_uniforms;

protected:
    Shader() {
        m_pid = glCreateProgram();
        if (m_pid == 0) {
            std::cerr << "Could not create program object";
            exit(1);
        }
    }

public:
    static ShaderPtr Make() {
        return ShaderPtr(new Shader());
    }
    
    virtual ~Shader(){
        glDeleteProgram(m_pid);
    }

    unsigned int GetShaderID() const {
        return m_pid;
    }

    void AttachVertexShader(const std::string& filename) {
        GLuint sid = MakeShader(GL_VERTEX_SHADER, filename);
        glAttachShader(m_pid, sid);
    }

    void AttachFragmentShader(const std::string& filename) {
        GLuint sid = MakeShader(GL_FRAGMENT_SHADER, filename);
        glAttachShader(m_pid, sid);
    }

    void Link() {
        glLinkProgram(m_pid);
        GLint status;
        glGetProgramiv(m_pid, GL_LINK_STATUS, &status);
        if (status == GL_FALSE) {
            GLint len;
            glGetProgramiv(m_pid, GL_INFO_LOG_LENGTH, &len);
            char* message = new char[len];
            glGetProgramInfoLog(m_pid, len, 0, message);
            std::cerr << "Shader linking failed: " << message << std::endl;
            delete[] message;
            exit(1);
        }
    }

    // Configura um uniforme dinâmico
    template<typename T>
    void configureUniform(const std::string& name, std::function<T()> value_provider) {
        // 1. Cria o objeto Uniform.
        auto uniform_obj = Uniform<T>::Make(name, std::move(value_provider));
        
        // 2. Pede ao objeto para encontrar sua própria localização neste shader.
        uniform_obj->findLocation(m_pid);
        
        // 3. Armazena o uniforme configurado no mapa.
        m_uniforms[name] = std::move(uniform_obj);
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
        applyUniforms();
    }
};

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
        // only push if it's different from the current top
        if (shader != stack.back()) {
            stack.push_back(shader);
        }
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
        // A lógica de otimização de usar o shader só quando muda é boa.
        // O UseProgram agora também aplica os uniformes.
        if (current_shader != last_used_shader) {
            current_shader->UseProgram();
            last_used_shader = current_shader;
        }
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