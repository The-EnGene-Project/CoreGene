#ifndef CUBEMAP_COMPONENT_H
#define CUBEMAP_COMPONENT_H
#pragma once

#include "component.h"
#include "../gl_base/cubemap.h"
#include "../gl_base/texture.h"
#include <memory>

namespace component {

class CubemapComponent;
using CubemapComponentPtr = std::shared_ptr<CubemapComponent>;

/**
 * @class CubemapComponent
 * @brief A component that binds a cubemap texture to a specific texture unit.
 *
 * This component wraps a texture::CubemapPtr. When applied, it pushes the
 * cubemap onto the global texture::TextureStack, making it the active cubemap
 * for the given texture unit. When unapplied, it pops the stack, restoring
 * the previous texture state. This is designed to work within a scene graph
 * traversal system.
 * 
 * Priority: 300 (APPEARANCE) - Same as TextureComponent
 * 
 * @note This component is similar to TextureComponent but specifically for cubemaps.
 * @note SkyboxComponent extends this class to add skybox-specific rendering logic.
 */
class CubemapComponent : public Component {
protected:
    texture::CubemapPtr m_cubemap;
    GLuint m_texture_unit;
    std::string m_sampler_name;

    /**
     * @brief Constructs a CubemapComponent.
     * @param cubemap The cubemap resource to manage.
     * @param samplerName The name of the sampler uniform in the shader.
     * @param unit The OpenGL texture unit to bind to (e.g., GL_TEXTURE0 is unit 0).
     */
    explicit CubemapComponent(texture::CubemapPtr cubemap, std::string samplerName, GLuint unit = 0)
        : Component(ComponentPriority::APPEARANCE), // Cubemaps are part of appearance.
          m_cubemap(cubemap),
          m_texture_unit(unit),
          m_sampler_name(std::move(samplerName))
    {
    }

public:
    // Delete copy constructor and assignment operator to prevent accidental copying.
    CubemapComponent(const CubemapComponent&) = delete;
    CubemapComponent& operator=(const CubemapComponent&) = delete;

    /**
     * @brief Factory function to create a new CubemapComponent.
     * @param cubemap The cubemap resource to manage.
     * @param samplerName The name of the sampler uniform in the shader.
     * @param unit The OpenGL texture unit to bind to.
     * @return A shared pointer to the newly created CubemapComponent.
     */
    static CubemapComponentPtr Make(texture::CubemapPtr cubemap, std::string samplerName, GLuint unit = 0) {
        return CubemapComponentPtr(new CubemapComponent(cubemap, std::move(samplerName), unit));
    }

    /**
     * @brief Factory function to create a named CubemapComponent.
     * @param cubemap The cubemap resource to manage.
     * @param samplerName The name of the sampler uniform in the shader.
     * @param unit The OpenGL texture unit to bind to.
     * @param name Unique name for this component instance.
     * @return A shared pointer to the newly created CubemapComponent.
     */
    static CubemapComponentPtr Make(texture::CubemapPtr cubemap, std::string samplerName, GLuint unit, const std::string& name) {
        auto comp = CubemapComponentPtr(new CubemapComponent(cubemap, std::move(samplerName), unit));
        comp->setName(name);
        return comp;
    }

    /**
     * @brief Pushes the component's cubemap onto the texture stack, making it active.
     * This method is intended to be called during a "pre-order" scene graph traversal.
     */
    void apply() override {
        if (m_cubemap) {
            // 1. Push cubemap to the stack to bind it
            texture::stack()->push(m_cubemap, m_texture_unit);
            // 2. Register this component's sampler-to-unit mapping
            texture::stack()->registerSamplerUnit(m_sampler_name, m_texture_unit);
        }
    }

    /**
     * @brief Pops from the texture stack, restoring the previous texture state.
     * This method is intended to be called during a "post-order" scene graph traversal.
     */
    void unapply() override {
        if (m_cubemap) {
            // 1. Un-register the mapping
            texture::stack()->unregisterSamplerUnit(m_sampler_name);
            // 2. Pop the stack to restore previous texture state
            texture::stack()->pop();
        }
    }

    /**
     * @brief Get the type name of this component.
     * @return "CubemapComponent"
     */
    virtual const char* getTypeName() const override {
        return "CubemapComponent";
    }

    /**
     * @brief Get the current cubemap texture.
     * @return Shared pointer to the cubemap texture
     */
    texture::CubemapPtr getCubemap() const {
        return m_cubemap;
    }

    /**
     * @brief Set a new cubemap texture.
     * @param cubemap The new cubemap texture
     */
    void setCubemap(texture::CubemapPtr cubemap) {
        m_cubemap = cubemap;
    }

    /**
     * @brief Get the texture unit this cubemap is bound to.
     * @return Texture unit (0-31)
     */
    GLuint getTextureUnit() const {
        return m_texture_unit;
    }

    /**
     * @brief Get the sampler name used in shaders.
     * @return Sampler uniform name
     */
    const std::string& getSamplerName() const {
        return m_sampler_name;
    }
};

} // namespace component

#endif // CUBEMAP_COMPONENT_H
