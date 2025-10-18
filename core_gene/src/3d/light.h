#pragma once
#include <glm/glm.hpp>
#include <string>
#include <memory>
#include "../gl_base/shader.h"
#include "../gl_base/transform.h"
#include "../core/scene.h"

class Light;
using LightPtr = std::shared_ptr<Light>;

struct LightParams
{
    glm::vec4 ambient = glm::vec4(0.3f);
    glm::vec4 diffuse = glm::vec4(0.7f);
    glm::vec4 specular = glm::vec4(1.0f);
    std::string space = "world";
};

class Light
{
protected:
    Light(LightParams params)
        : space(params.space),
          ambient(params.ambient),
          diffuse(params.diffuse),
          specular(params.specular) {}

    glm::vec4 TransformToShaderSpace(glm::vec4 target) const
    {
        transform::TransformPtr t = transform::Transform::Make(); // identidade

        // Aplica a transformação do espaço da luz para o espaço do shader
        if (space == "world")
        {
            t->multiply(scene::graph()->getCamera()->getMatrix());
        }
        else if (space == "camera")
        {
            t->multiply(glm::inverse(scene::graph()->getCamera()->getMatrix()));
        }
        return t->getMatrix() * target;
    }

public:
    std::string space;
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;

    virtual ~Light() = default;

    // Método para enviar a luz para o shader
    virtual void attachToShader(shader::Shader &shader, const std::string &namePrefix) const = 0;

    void attachBase(shader::Shader &shader, const std::string &namePrefix) const
    {
        shader.configureUniform<glm::vec4>(namePrefix + ".ambient", [this]()
                                           { return ambient; });
        shader.configureUniform<glm::vec4>(namePrefix + ".diffuse", [this]()
                                           { return diffuse; });
        shader.configureUniform<glm::vec4>(namePrefix + ".specular", [this]()
                                           { return specular; });
    }
};