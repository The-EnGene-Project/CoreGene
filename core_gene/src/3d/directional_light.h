#pragma once
#include "light.h"

struct DirectionalLightParams : LightParams {
    glm::vec3 direction = glm::vec3(0, -1, 0);
};

class DirectionalLight : public Light {
protected:
    DirectionalLight(DirectionalLightParams params)
        : Light(params),
          direction(params.direction) {}
public:
    glm::vec3 direction;

    void attachToShader(shader::Shader& shader, const std::string& namePrefix) const override {
        attachBase(shader, namePrefix);
        glm::vec3 dirInShaderSpace = glm::vec3(TransformToShaderSpace(glm::vec4(direction, 0.0f)));
        shader.configureUniform<glm::vec3>(namePrefix + ".direction", [dirInShaderSpace]()
                                           { return dirInShaderSpace; });
    }
    static LightPtr Make(const DirectionalLightParams &params)
    {
        return LightPtr(new DirectionalLight(params));
    }
};
