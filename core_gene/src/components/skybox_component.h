#ifndef SKYBOX_COMPONENT_H
#define SKYBOX_COMPONENT_H
#pragma once

#include <memory>
#include "cubemap_component.h"
#include "../gl_base/cubemap.h"
#include "../gl_base/shader.h"
#include "../gl_base/skybox_cube.h"
#include "../gl_base/texture.h"
#include "../core/scene.h"

namespace component {

class SkyboxComponent;
using SkyboxComponentPtr = std::shared_ptr<SkyboxComponent>;

/**
 * @class SkyboxComponent
 * @brief Component for rendering a skybox that always surrounds the camera.
 * 
 * This component extends CubemapComponent to add skybox-specific rendering logic.
 * It manages skybox rendering with automatic camera positioning and depth buffer control.
 * The skybox is rendered with depth writes disabled to ensure it always appears behind all scene objects.
 * 
 * Priority: 150 (between Transform at 100 and Shader at 200)
 * - Ensures skybox is positioned after transforms but before material/geometry rendering
 * - Overrides CubemapComponent's APPEARANCE priority (300)
 * 
 * @note The skybox uses a dedicated shader and cube geometry optimized for skybox rendering.
 * @note Depth buffer writing is disabled during skybox rendering to prevent occlusion of scene objects.
 * @note This component extends CubemapComponent but uses a custom priority and rendering logic.
 */
class SkyboxComponent : public CubemapComponent {
private:
    shader::ShaderPtr m_skybox_shader;
    geometry::SkyboxCubePtr m_cube_geometry;
    GLboolean m_depth_write_enabled_before;
    
protected:
    /**
     * @brief Protected constructor for RAII pattern.
     * @param cubemap The cubemap texture to use for the skybox
     */
    explicit SkyboxComponent(texture::CubemapPtr cubemap)
        : CubemapComponent(cubemap, "skybox", 0), // Initialize base with cubemap, sampler name, and unit 0
          m_depth_write_enabled_before(GL_TRUE)
    {
        // Note: Priority is set to 150 via overridden getPriority() method
        // This is different from CubemapComponent's APPEARANCE priority (300)
        
        // Create skybox cube geometry
        m_cube_geometry = geometry::SkyboxCube::Make();
        
        // Create skybox shader from shader files
        m_skybox_shader = shader::Shader::Make(
            "core_gene/shaders/skybox_vertex.glsl",
            "core_gene/shaders/skybox_fragment.glsl"
        );
        
        // Configure cubemap sampler uniform using cubemap sampler provider
        m_skybox_shader->configureDynamicUniform<uniform::detail::Sampler>("u_skybox",
            texture::getSamplerProvider("skybox"));
        
        // Silence validation warnings for uniforms set via immediate mode (Tier 4)
        m_skybox_shader->silenceUniform("u_viewProjection");
        
        // Bake the shader to ensure it's ready
        m_skybox_shader->Bake();
    }

public:
    /**
     * @brief Factory method to create a skybox component.
     * @param cubemap The cubemap texture to use for the skybox
     * @return Shared pointer to the created SkyboxComponent
     */
    static SkyboxComponentPtr Make(texture::CubemapPtr cubemap) {
        return SkyboxComponentPtr(new SkyboxComponent(cubemap));
    }
    
    /**
     * @brief Factory method to create a named skybox component.
     * @param cubemap The cubemap texture to use for the skybox
     * @param name Unique name for this component instance
     * @return Shared pointer to the created SkyboxComponent
     */
    static SkyboxComponentPtr Make(texture::CubemapPtr cubemap, const std::string& name) {
        auto comp = SkyboxComponentPtr(new SkyboxComponent(cubemap));
        comp->setName(name);
        return comp;
    }
    
    /**
     * @brief Apply method called during scene traversal.
     * 
     * This method overrides CubemapComponent::apply() to add skybox-specific rendering:
     * 1. Calls base class apply() to bind cubemap to texture stack
     * 2. Saves current depth write state
     * 3. Disables depth buffer writing
     * 4. Gets camera matrices from active camera
     * 5. Removes translation from view matrix
     * 6. Calculates view-projection matrix on CPU
     * 7. Pushes skybox shader onto ShaderStack
     * 8. Sets uniforms using setUniform() (Tier 4)
     * 9. Renders cube geometry
     * 10. Pops shader from ShaderStack
     * 11. Restores depth buffer writing state
     * 
     * @note The cubemap is bound by the base class and will be unbound by base class unapply()
     */
    void apply() override {
        // Validate cubemap exists
        if (!m_cubemap) {
            std::cerr << "Warning: SkyboxComponent has no cubemap texture. Skipping render." << std::endl;
            return;
        }
        
        // Get active camera
        auto camera = scene::graph()->getActiveCamera();
        if (!camera) {
            std::cerr << "Warning: No active camera for skybox positioning. Skipping render." << std::endl;
            return;
        }
        
        // Call base class to bind cubemap to texture stack
        CubemapComponent::apply();
        
        // Save current depth write state
        glGetBooleanv(GL_DEPTH_WRITEMASK, &m_depth_write_enabled_before);
        
        // Disable depth buffer writing (skybox should not write to depth buffer)
        glDepthMask(GL_FALSE);
        
        // Set depth function to LEQUAL to allow skybox at maximum depth
        glDepthFunc(GL_LEQUAL);
        
        // Get camera matrices
        glm::mat4 view = camera->getViewMatrix();
        glm::mat4 projection = camera->getProjectionMatrix();
        
        // Remove translation from view matrix (keep only rotation)
        // This ensures the skybox always surrounds the camera
        glm::mat4 view_no_translation = glm::mat4(glm::mat3(view));
        
        // Calculate combined view-projection matrix on CPU
        glm::mat4 view_projection = projection * view_no_translation;
        
        // Push skybox shader onto ShaderStack
        shader::stack()->push(m_skybox_shader);
        
        // Set view-projection uniform using Tier 4 (immediate mode)
        m_skybox_shader->setUniform("u_viewProjection", view_projection);
        
        // Render cube geometry (cubemap already bound by base class)
        shader::stack()->top();
        m_cube_geometry->Draw();
        
        // Pop shader from ShaderStack
        shader::stack()->pop();
        
        // Restore depth buffer writing state
        glDepthMask(m_depth_write_enabled_before);
        
        // Restore default depth function
        glDepthFunc(GL_LESS);
    }
    
    /**
     * @brief Unapply method called after scene traversal.
     * 
     * Calls base class unapply() to unbind cubemap from texture stack.
     */
    void unapply() override {
        // Call base class to unbind cubemap from texture stack
        CubemapComponent::unapply();
    }
    
    /**
     * @brief Get the type name of this component.
     * @return "SkyboxComponent"
     */
    const char* getTypeName() const override {
        return "SkyboxComponent";
    }
    
    /**
     * @brief Override priority to 150 (between Transform and Shader).
     * This ensures skybox renders before other appearance components.
     * @return Priority value of 150
     */
    unsigned int getPriority() override {
        return 150;
    }
    
    /**
     * @brief Set a custom shader for the skybox.
     * @param shader The custom shader to use
     * 
     * @note The custom shader must have the same uniforms as the default skybox shader:
     *       - u_viewProjection (mat4)
     *       - u_skybox (samplerCube)
     */
    void setCustomShader(shader::ShaderPtr shader) {
        m_skybox_shader = shader;
    }
    
    /**
     * @brief Get the current skybox shader.
     * @return Shared pointer to the skybox shader
     */
    shader::ShaderPtr getShader() const {
        return m_skybox_shader;
    }
    
    // Note: getCubemap() and setCubemap() are inherited from CubemapComponent
};

} // namespace component

#endif // SKYBOX_COMPONENT_H
