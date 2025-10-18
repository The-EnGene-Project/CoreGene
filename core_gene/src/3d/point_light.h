#pragma once
#include "light.h"
#include <vector>
#include "scene.h"
#include "node.h"

struct PointLightParams : LightParams
{
    glm::vec4 position = glm::vec4(0, 0, 0, 1);
    std::vector<float> attenuation = {1.0f, 0.09f, 0.032f};
};

class PointLight : public Light
{
protected:
    glm::vec4 position;
    std::vector<float> attenuation;
    PointLight(PointLightParams params)
        : Light(params),
          position(params.position),
          attenuation(params.attenuation) {}

public:
    void attachToShader(shader::Shader &shader, const std::string &namePrefix) const override
    {
        attachBase(shader, namePrefix);
        glm::vec4 posInShaderSpace = TransformToShaderSpace(position);
        shader.configureUniform<glm::vec4>(namePrefix + ".position", [posInShaderSpace]()
                                           { return posInShaderSpace; });

        shader.configureUniform<std::vector<float>>(namePrefix + ".attenuation", [this]()
                                                    { return attenuation; });
    }
    static LightPtr Make(const PointLightParams &params)
    {
        return LightPtr(new PointLight(params));
    }
};
