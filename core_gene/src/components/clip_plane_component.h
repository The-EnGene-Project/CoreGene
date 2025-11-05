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
    component::CameraPtr m_camera; 
    uniform::UniformPtr<std::vector<glm::vec4>> m_uniformPlanes;

protected:
    explicit ClipPlaneComponent(const std::string& uniformName)
        : Component(ComponentPriority::APPEARANCE),
          m_uniformName(uniformName),
          m_uniformPlanes(uniform::Uniform<std::vector<glm::vec4>>::Make(uniformName))
    {}

public:

    static ClipPlaneComponentPtr Make(const std::string& name) {
        return std::make_shared<ClipPlaneComponent>(name);
    }

    static ClipPlaneComponentPtr Make(const std::string& name, float a, float b, float c, float d) {
        auto comp = std::make_shared<ClipPlaneComponent>(name);
        comp->addPlane(a, b, c, d);
        return comp;
    }

    virtual const char* getTypeName() const override { return "ClipPlaneComponent"; }

    void addPlane(float a, float b, float c, float d) {
        m_localPlanes.emplace_back(a, b, c, d);
    }

    void clearPlanes() { m_localPlanes.clear(); }

    void setCamera(component::CameraPtr camera) { m_camera = camera; }

    const std::vector<glm::vec4>& getPlanes() const { return m_localPlanes; }



    virtual void apply() override {
        if (m_localPlanes.empty())
            return;

        auto shader = shader::stack()->peek();
        if (!shader)
            return;


        glm::mat4 model = transform::current();
        glm::mat4 view = glm::mat4(1.0f);

        if (m_camera)
            view = m_camera->getViewMatrix();

        glm::mat4 modelView = view * model;
        glm::mat4 mit = glm::transpose(glm::inverse(modelView));

        std::vector<glm::vec4> transformedPlanes;
        transformedPlanes.reserve(m_localPlanes.size());

        for (auto& p : m_localPlanes)
            transformedPlanes.push_back(mit * p);

        m_uniformPlanes->set(transformedPlanes);
        m_uniformPlanes->apply(shader);

        for (size_t i = 0; i < m_localPlanes.size(); ++i)
            glEnable(GL_CLIP_DISTANCE0 + static_cast<int>(i));
    }

    virtual void unapply() override {
        for (size_t i = 0; i < m_localPlanes.size(); ++i)
            glDisable(GL_CLIP_DISTANCE0 + static_cast<int>(i));
    }
};

} // namespace component

#endif // CLIP_PLANE_COMPONENT_H
