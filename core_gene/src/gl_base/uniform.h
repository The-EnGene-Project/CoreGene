#ifndef UNIFORM_H
#define UNIFORM_H
#pragma once

#include "gl_includes.h" // Supondo que isso inclua GLAD/GLEW
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <memory>
#include <functional>

namespace shader {

// Forward declaration
class UniformInterface;

// Usando um alias de ponteiro para consistência com o resto do projeto
using UniformInterfacePtr = std::unique_ptr<UniformInterface>;

// Interface abstrata para um Uniform.
// A classe Shader irá armazenar ponteiros para esta interface.
class UniformInterface {
public:
    GLint location = -1;
    virtual ~UniformInterface() = default;
    // O método virtual puro que será chamado para aplicar o uniforme.
    virtual void apply() const = 0;
};

// Classe de template que implementa a interface para um tipo específico.
template<typename T>
class Uniform : public UniformInterface {
private:
    // Armazena uma função que não recebe argumentos e retorna um valor do tipo T.
    std::function<T()> value_provider;

    // Construtor privado para forçar o uso do método de fábrica Make().
    Uniform(std::function<T()> provider) : value_provider(std::move(provider)) {}

public:
    // Método de fábrica (Factory Method) estático para criar instâncias.
    // Exemplo de uso: auto my_uniform = shader::Uniform<float>::Make(...);
    static UniformInterfacePtr Make(std::function<T()> provider) {
        // Usamos std::unique_ptr com um construtor privado.
        // O std::make_unique não funcionaria aqui porque o construtor é privado.
        return std::unique_ptr<Uniform<T>>(new Uniform<T>(std::move(provider)));
    }

    // A implementação de apply() é especializada para cada tipo.
    void apply() const override;
};

// --- Especializações do método apply() para cada tipo de uniforme ---

template<>
inline void Uniform<float>::apply() const {
    glUniform1f(location, value_provider());
}

template<>
inline void Uniform<int>::apply() const {
    glUniform1i(location, value_provider());
}

template<>
inline void Uniform<glm::vec2>::apply() const {
    glUniform2fv(location, 1, glm::value_ptr(value_provider()));
}

template<>
inline void Uniform<glm::vec3>::apply() const {
    glUniform3fv(location, 1, glm::value_ptr(value_provider()));
}

template<>
inline void Uniform<glm::vec4>::apply() const {
    glUniform4fv(location, 1, glm::value_ptr(value_provider()));
}

template<>
inline void Uniform<glm::mat3>::apply() const {
    glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value_provider()));
}

template<>
inline void Uniform<glm::mat4>::apply() const {
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value_provider()));
}

} // namespace shader

#endif

