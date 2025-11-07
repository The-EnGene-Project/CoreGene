#ifndef UNIFORM_H
#define UNIFORM_H
#pragma once

#include "../gl_includes.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <memory>
#include <functional>
#include <iostream>

namespace uniform {

namespace detail {
    /**
     * @brief Helper trait to map C++ types to their corresponding GLenum.
     */
    template<typename T> struct GLTypeFor { static const GLenum value = GL_NONE; }; // Default
    
    // Specializations for supported types
    template<> struct GLTypeFor<float>     { static const GLenum value = GL_FLOAT; };
    template<> struct GLTypeFor<int>       { static const GLenum value = GL_INT; };
    template<> struct GLTypeFor<bool>      { static const GLenum value = GL_BOOL; };
    template<> struct GLTypeFor<glm::vec2> { static const GLenum value = GL_FLOAT_VEC2; };
    template<> struct GLTypeFor<glm::vec3> { static const GLenum value = GL_FLOAT_VEC3; };
    template<> struct GLTypeFor<glm::vec4> { static const GLenum value = GL_FLOAT_VEC4; };
    template<> struct GLTypeFor<glm::mat3> { static const GLenum value = GL_FLOAT_MAT3; };
    template<> struct GLTypeFor<glm::mat4> { static const GLenum value = GL_FLOAT_MAT4; };
    
    // Sampler types (represented as int in C++ for texture unit binding)
    // Note: We use custom structs to distinguish samplers from regular int
    
    // Generic sampler type that works for all sampler types (sampler2D, samplerCube, etc.)
    // The shader will determine the actual sampler type based on the GLSL uniform declaration
    struct Sampler { int unit; };
    
    // Note: Sampler uses GL_SAMPLER_2D as a placeholder, but works for all sampler types
    // The actual type checking is done by comparing GLSL type with C++ configuration
    template<> struct GLTypeFor<Sampler> { static const GLenum value = GL_SAMPLER_2D; };
}

// Forward declaration
class UniformInterface;

// Usando um alias de ponteiro para consistência com o resto do projeto
using UniformInterfacePtr = std::unique_ptr<UniformInterface>;

// Interface abstrata para um Uniform.
class UniformInterface {
protected:
    GLint m_location = -2;
    std::string m_name;
    GLenum m_cpp_type = GL_NONE;

public:
    virtual ~UniformInterface() = default;
    virtual void apply() const = 0;

    // Novo método para buscar a localização do uniforme "preguiçosamente".
    // Será chamado pelo Shader no momento da configuração.
    void findLocation(GLuint program_id) {
        // Só busca a localização se ainda não a tivermos encontrado.
        if (m_location == -2) {
            m_location = glGetUniformLocation(program_id, m_name.c_str());
            if (m_location == -1) {
                std::cerr << "Warning: Uniform '" << m_name 
                        << "' was configured in C++ but is not an active uniform in the shader program."
                        << " It might be unused or misspelled." << std::endl;
            }
        }
    }

    bool isValid() const {
        return m_location >= 0;
    }
    
    const std::string& getName() const { 
        return m_name; 
    }

    GLenum getCppType() const {
        return m_cpp_type;
    }
};

// Classe de template que implementa a interface para um tipo específico.
template<typename T>
class Uniform : public UniformInterface {
private:
    // Armazena uma função que não recebe argumentos e retorna um valor do tipo T.
    std::function<T()> value_provider;

    // Construtor privado para forçar o uso do método de fábrica Make().
    Uniform(const std::string& uniform_name, std::function<T()> provider)
        : value_provider(std::move(provider))
    {
        m_name = uniform_name;
        m_cpp_type = detail::GLTypeFor<T>::value;
    }

public:
    // Método de fábrica (Factory Method) estático para criar instâncias.
    // Exemplo de uso: auto my_uniform = uniform::Uniform<float>::Make(...);
    static UniformInterfacePtr Make(const std::string& name, std::function<T()> provider) {
        return std::unique_ptr<Uniform<T>>(new Uniform<T>(name, std::move(provider)));
    }

    // A implementação de apply() é especializada para cada tipo.
    void apply() const override;
};

// --- Especializações do método apply() para cada tipo de uniforme ---

template<>
inline void Uniform<float>::apply() const {
    if (isValid()) {
        glUniform1f(m_location, value_provider());
    }
}

template<>
inline void Uniform<int>::apply() const {
    if (isValid()) {
        glUniform1i(m_location, value_provider());
    }
}

template<>
inline void Uniform<bool>::apply() const {
    if (isValid()) {
        // In GLSL, booleans are represented as integers (0 = false, non-zero = true)
        glUniform1i(m_location, value_provider() ? 1 : 0);
    }
}

template<>
inline void Uniform<glm::vec2>::apply() const {
    if (isValid()) {
        glUniform2fv(m_location, 1, glm::value_ptr(value_provider()));
    }
}

template<>
inline void Uniform<glm::vec3>::apply() const {
    if (isValid()) {
        glUniform3fv(m_location, 1, glm::value_ptr(value_provider()));
    }
}

template<>
inline void Uniform<glm::vec4>::apply() const {
    if (isValid()) {
        glUniform4fv(m_location, 1, glm::value_ptr(value_provider()));
    }
}

template<>
inline void Uniform<glm::mat3>::apply() const {
    if (isValid()) {
        glUniformMatrix3fv(m_location, 1, GL_FALSE, glm::value_ptr(value_provider()));
    }
}

template<>
inline void Uniform<glm::mat4>::apply() const {
    if (isValid()) {
        glUniformMatrix4fv(m_location, 1, GL_FALSE, glm::value_ptr(value_provider()));
    }
}

template<>
inline void Uniform<detail::Sampler>::apply() const {
    if (isValid()) {
        glUniform1i(m_location, value_provider().unit);
    }
}

} // namespace uniform

#endif

