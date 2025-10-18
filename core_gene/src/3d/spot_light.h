#pragma once
#include "point_light.h"

struct SpotLightParams : PointLightParams
{
    glm::vec3 direction = glm::vec3(0, -1, 0);
    float cutOff = glm::cos(glm::radians(12.5f));
};

class SpotLight : public PointLight
{
protected:
    SpotLight(SpotLightParams params)
        : PointLight(params),
          direction(params.direction),
          cutOffAngle(params.cutOff) {}

public:
    glm::vec3 direction;
    float cutOffAngle;

    void attachToShader(shader::Shader &shader, const std::string &namePrefix) const override
    {
        PointLight::attachToShader(shader, namePrefix);
        glm::vec3 dirInShaderSpace = glm::vec3(TransformToShaderSpace(glm::vec4(direction, 0.0f)));
        shader.configureUniform<glm::vec3>(namePrefix + ".direction", [dirInShaderSpace]()
                                           { return dirInShaderSpace; });
        shader.configureUniform<float>(namePrefix + ".cutOffAngle", [this]()
                                       { return cutOffAngle; });
    }
    static LightPtr Make(const SpotLightParams &params)
    {
        return LightPtr(new SpotLight(params));
    }
};
