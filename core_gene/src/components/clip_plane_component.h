#ifndef CLIP_PLANE_COMPONENT_H
#define CLIP_PLANE_COMPONENT_H
#pragma once

#include "component.h"
#include "../gl_base/shader_stack.h"
#include "../gl_base/shader.h"
#include "../gl_base/transform.h"
#include "../components/camera.h"
#include "../gl_base/uniforms/uniform.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <vector>
#include <string>
#include <memory>

namespace component {

/**
 * @brief ClipPlaneComponent
    *This component manages user-defined clipping planes in a shader.
    * It allows adding multiple planes defined in local space, which are then
    * transformed to world space using the current model and view matrices.
    * The planes are passed to the shader as a uniform array, and OpenGL's
    * clip distances are enabled/disabled accordingly.
 */
class ClipPlaneComponent;
using ClipPlaneComponentPtr = std::shared_ptr<ClipPlaneComponent>;

class ClipPlaneComponent : public Component {
private:
    std::string m_uniformName; 
    std::vector<glm::vec4> m_localPlanes; 
    std::vector<glm::vec4> m_transformedPlanes;
    GLint m_location;

protected:
    explicit ClipPlaneComponent(const std::string& uniformName)
        : Component(ComponentPriority::APPEARANCE),
          m_uniformName(uniformName),
          m_location(-1)
    {}

public:

    static ClipPlaneComponentPtr Make(const std::string& name) {
        return ClipPlaneComponentPtr(new ClipPlaneComponent(name));
    }

    static ClipPlaneComponentPtr Make(float a, float b, float c, float d, const std::string& name,) {
        auto comp = ClipPlaneComponentPtr(new ClipPlaneComponent(name));
        comp->addPlane(a, b, c, d);
        return comp;
    }

    virtual const char* getTypeName() const override { return "ClipPlaneComponent"; }

    void addPlane(float a, float b, float c, float d) {
        m_localPlanes.emplace_back(a, b, c, d);
    }

    void clearPlanes() { m_localPlanes.clear(); }

    const std::vector<glm::vec4>& getPlanes() const { return m_localPlanes; }



    virtual void apply() override {
        if (m_localPlanes.empty())
            return;

        auto shader = shader::stack()->peek();
        if (!shader)
            return;

        // Obtém/atualiza a localização do uniform se necessário
        if (m_location == -1) {
            m_location = glGetUniformLocation(shader->GetShaderID(), m_uniformName.c_str());
            if (m_location == -1) {
                std::cerr << "Warning: Uniform '" << m_uniformName 
                         << "' not found in shader program." << std::endl;
                return;
            }
        }

        // Calcula as transformações necessárias
        glm::mat4 model = transform::current();
        glm::mat4 view = glm::mat4(1.0f);

        auto camera = scene::graph()->getActiveCamera();
        if (camera)
            view = camera->getViewMatrix();

        glm::mat4 modelView = view * model;
        glm::mat4 mit = glm::transpose(glm::inverse(modelView));

        // Transforma os planos
        m_transformedPlanes.clear();
        m_transformedPlanes.reserve(m_localPlanes.size());

        for (auto& p : m_localPlanes)
            m_transformedPlanes.push_back(mit * p);

        // Envia todos os planos transformados de uma vez
        glUniform4fv(m_location, static_cast<GLsizei>(m_transformedPlanes.size()),
                     glm::value_ptr(m_transformedPlanes[0]));

        // Ativa os clip planes
        for (size_t i = 0; i < m_transformedPlanes.size(); ++i)
            glEnable(GL_CLIP_DISTANCE0 + static_cast<int>(i));
    }

    virtual void unapply() override {
        for (size_t i = 0; i < m_localPlanes.size(); ++i)
            glDisable(GL_CLIP_DISTANCE0 + static_cast<int>(i));
    }
};

} // namespace component

#endif // CLIP_PLANE_COMPONENT_H
