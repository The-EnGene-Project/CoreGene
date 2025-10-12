#ifndef TEXTURE_COMPONENT_H
#define TEXTURE_COMPONENT_H
#pragma once

#include "component.h"
#include "../gl_base/texture.h"
#include <memory>

namespace component {

class TextureComponent;
using TextureComponentPtr = std::shared_ptr<TextureComponent>;

/**
 * @class TextureComponent
 * @brief A component that binds a texture to a specific texture unit.
 *
 * This component wraps a texture::TexturePtr. When applied, it pushes the
 * texture onto the global texture::TextureStack, making it the active texture
 * for the given texture unit. When unapplied, it pops the stack, restoring
 * the previous texture state. This is designed to work within a scene graph
 * traversal system.
 */
class TextureComponent : public Component {
private:
    texture::TexturePtr m_texture;
    GLuint m_texture_unit;
    std::string m_sampler_name;

protected:
    /**
     * @brief Constructs a TextureComponent.
     * @param texture The texture resource to manage.
     * @param unit The OpenGL texture unit to bind to (e.g., GL_TEXTURE0 is unit 0).
     */
    explicit TextureComponent(texture::TexturePtr texture, std::string samplerName, GLuint unit = 0)
        : Component(ComponentPriority::APPEARANCE), // Textures are part of appearance.
          m_texture(texture),
          m_texture_unit(unit),
          m_sampler_name(std::move(samplerName))
    {
    }

public:
    // Delete copy constructor and assignment operator to prevent accidental copying.
    TextureComponent(const TextureComponent&) = delete;
    TextureComponent& operator=(const TextureComponent&) = delete;

    /**
     * @brief Factory function to create a new TextureComponent.
     * @param texture The texture resource to manage.
     * @param unit The OpenGL texture unit to bind to.
     * @return A shared pointer to the newly created TextureComponent.
     */
    static TextureComponentPtr Make(texture::TexturePtr texture, std::string samplerName, GLuint unit = 0) {
        return TextureComponentPtr(new TextureComponent(texture, std::move(samplerName), unit));
    }

    /**
     * @brief Pushes the component's texture onto the texture stack, making it active.
     * This method is intended to be called during a "pre-order" scene graph traversal.
     */
    void apply() override {
        if (m_texture) {
            // 1. Push texture to the stack to bind it (existing logic)
            texture::stack()->push(m_texture, m_texture_unit);
            // 2. Register this component's sampler-to-unit mapping
            texture::stack()->registerSamplerUnit(m_sampler_name, m_texture_unit);
        }
    }

    /**
     * @brief Pops from the texture stack, restoring the previous texture state.
     * This method is intended to be called during a "post-order" scene graph traversal.
     */
    void unapply() override {
        if (m_texture) {
            // 1. Un-register the mapping
            texture::stack()->unregisterSamplerUnit(m_sampler_name);
            // 2. Pop the stack to restore previous texture state
            texture::stack()->pop();
        }
    }

    virtual const char* getTypeName() const override {
        return "TextureComponent";
    }
};

} // namespace component

#endif // TEXTURE_COMPONENT_H