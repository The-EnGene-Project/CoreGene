#ifndef CLIP_PLANE_COMPONENT_H
#define CLIP_PLANE_COMPONENT_H
#pragma once

#include "component.h"
#include "../gl_base/shader.h"
#include "../gl_base/transform.h"
#include "../3d/camera/camera.h"
#include "../gl_base/uniforms/uniform.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>
#include <memory>
#include <iostream>

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
    std::string m_countUniformName;
    std::vector<glm::vec4> m_localPlanes; 
    std::vector<glm::vec4> m_transformedPlanes;
    GLint m_location;
    GLint m_countLocation;

protected:
    explicit ClipPlaneComponent(const std::string& uniformName, const std::string& countUniformName)
        : Component(ComponentPriority::APPEARANCE),
          m_uniformName(uniformName),
          m_countUniformName(countUniformName),
          m_location(-1),
          m_countLocation(-1)
    {}

public:

    static ClipPlaneComponentPtr Make(const std::string& planesUniformName, const std::string& countUniformName = "num_clip_planes") {
        return ClipPlaneComponentPtr(new ClipPlaneComponent(planesUniformName, countUniformName));
    }

    static ClipPlaneComponentPtr Make(float a, float b, float c, float d, const std::string& planesUniformName, const std::string& countUniformName = "num_clip_planes") {
        auto comp = ClipPlaneComponentPtr(new ClipPlaneComponent(planesUniformName, countUniformName));
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
        // Ensure shader is active by calling top() instead of peek()
        auto shader = shader::stack()->top();
        if (!shader)
            return;

        // Get/update uniform locations if necessary
        if (m_location == -1) {
            m_location = glGetUniformLocation(shader->GetShaderID(), m_uniformName.c_str());
            if (m_location == -1) {
                std::cerr << "Warning: Uniform '" << m_uniformName 
                         << "' not found in shader program." << std::endl;
            }
        }
        
        if (m_countLocation == -1) {
            m_countLocation = glGetUniformLocation(shader->GetShaderID(), m_countUniformName.c_str());
            if (m_countLocation == -1) {
                std::cerr << "Warning: Uniform '" << m_countUniformName 
                         << "' not found in shader program." << std::endl;
            }
        }

        // Set the number of clip planes
        int numPlanes = static_cast<int>(m_localPlanes.size());
        if (m_countLocation >= 0) {
            glUniform1i(m_countLocation, numPlanes);
        }

        // If no planes, just disable all clip distances and return
        if (m_localPlanes.empty()) {
            for (int i = 0; i < 6; ++i) {  // MAX_CLIP_PLANES = 6
                glDisable(GL_CLIP_DISTANCE0 + i);
            }
            return;
        }

        // Calculate necessary transformations
        glm::mat4 model = transform::current();
        glm::mat4 view = glm::mat4(1.0f);

        auto camera = scene::graph()->getActiveCamera();
        if (camera)
            view = camera->getViewMatrix();

        glm::mat4 modelView = view * model;
        glm::mat4 mit = glm::transpose(glm::inverse(modelView));

        // Transform the planes
        m_transformedPlanes.clear();
        m_transformedPlanes.reserve(m_localPlanes.size());

        for (auto& p : m_localPlanes)
            m_transformedPlanes.push_back(mit * p);

        // Send all transformed planes at once
        if (m_location >= 0) {
            glUniform4fv(m_location, static_cast<GLsizei>(m_transformedPlanes.size()),
                         glm::value_ptr(m_transformedPlanes[0]));
        }

        // Enable the clip planes
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
