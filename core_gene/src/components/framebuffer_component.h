#ifndef FRAMEBUFFER_COMPONENT_H
#define FRAMEBUFFER_COMPONENT_H
#pragma once

#include <memory>
#include "component.h"
#include "../gl_base/framebuffer.h"

/**
 * @file framebuffer_component.h
 * @brief Component for integrating framebuffers with the scene graph
 * 
 * The FramebufferComponent enables declarative framebuffer usage in the scene
 * hierarchy by automatically managing framebuffer stack push/pop operations
 * during scene traversal.
 */

namespace component {

class FramebufferComponent;
using FramebufferComponentPtr = std::shared_ptr<FramebufferComponent>;

/**
 * @class FramebufferComponent
 * @brief Scene graph component for framebuffer management
 * 
 * The FramebufferComponent integrates framebuffers with the scene graph component
 * system, enabling hierarchical off-screen rendering. When a node with this
 * component is visited during scene traversal, the framebuffer is automatically
 * pushed onto the framebuffer stack, and all child nodes render to the FBO.
 * When the node's traversal completes, the framebuffer is popped, restoring
 * the previous rendering target.
 * 
 * Priority: 150 (executes after Transform [100] but before Shader [200])
 * 
 * Key features:
 * - Automatic framebuffer stack management via apply()/unapply()
 * - Convenient texture retrieval methods
 * - Seamless integration with scene graph hierarchy
 * - RAII-compliant resource management
 * 
 * Usage example:
 * @code
 * auto fbo = framebuffer::Framebuffer::MakeRenderToTexture(800, 600, "scene_color");
 * 
 * scene::graph()->addNode("offscreen_render")
 *     .with<component::FramebufferComponent>(fbo)
 *     .with<component::ShaderComponent>(shader)
 *     .addNode("cube")
 *         .with<component::GeometryComponent>(cube_geometry);
 * 
 * // Later, retrieve the texture for use in another pass
 * auto fbo_comp = scene::graph()->getNode("offscreen_render")
 *                     ->getPayload().get<component::FramebufferComponent>();
 * auto texture = fbo_comp->getTexture("scene_color");
 * @endcode
 * 
 * @note The component priority (150) ensures framebuffers are bound after
 *       transforms are applied but before shaders are activated.
 */
class FramebufferComponent : virtual public Component {
private:
    framebuffer::FramebufferPtr m_fbo;
    
protected:
    /**
     * @brief Protected constructor for factory methods.
     * @param fbo The framebuffer to manage
     * 
     * Initializes the component with priority 150, positioning it between
     * Transform (100) and Shader (200) in the execution order.
     */
    FramebufferComponent(framebuffer::FramebufferPtr fbo) :
        Component(150),  // Priority 150: after Transform, before Shader
        m_fbo(fbo)
    {}

public:
    /**
     * @brief Factory method to create a FramebufferComponent.
     * @param fbo The framebuffer to manage
     * @return Shared pointer to the created component
     * 
     * Creates a FramebufferComponent that will manage the given framebuffer's
     * lifecycle within the scene graph.
     * 
     * @code
     * auto fbo = framebuffer::Framebuffer::MakeRenderToTexture(800, 600, "color");
     * auto comp = component::FramebufferComponent::Make(fbo);
     * @endcode
     */
    static FramebufferComponentPtr Make(framebuffer::FramebufferPtr fbo) {
        return FramebufferComponentPtr(new FramebufferComponent(fbo));
    }

    /**
     * @brief Factory method to create a named FramebufferComponent.
     * @param fbo The framebuffer to manage
     * @param name The name for this component instance
     * @return Shared pointer to the created component
     * 
     * Creates a named FramebufferComponent, allowing retrieval by name from
     * the component collection.
     * 
     * @code
     * auto fbo = framebuffer::Framebuffer::MakeRenderToTexture(800, 600, "color");
     * auto comp = component::FramebufferComponent::Make(fbo, "main_fbo");
     * // Later: node->getPayload().get<FramebufferComponent>("main_fbo");
     * @endcode
     */
    static FramebufferComponentPtr Make(framebuffer::FramebufferPtr fbo, const std::string& name) {
        auto comp = FramebufferComponentPtr(new FramebufferComponent(fbo));
        comp->setName(name);
        return comp;
    }
    
    /**
     * @brief Applies the component by pushing the framebuffer onto the stack.
     * 
     * Called during scene traversal when entering this node. Pushes the
     * framebuffer onto the framebuffer stack, redirecting all subsequent
     * rendering to the FBO until unapply() is called.
     * 
     * @note This method has side effects (modifies OpenGL state via stack)
     */
    virtual void apply() override {
        framebuffer::stack()->push(m_fbo);
    }

    /**
     * @brief Unapplies the component by popping the framebuffer from the stack.
     * 
     * Called during scene traversal when exiting this node. Pops the
     * framebuffer from the stack, restoring the previous rendering target
     * (either another FBO or the default framebuffer).
     * 
     * @note This method has side effects (modifies OpenGL state via stack)
     */
    virtual void unapply() override {
        framebuffer::stack()->pop();
    }

    /**
     * @brief Gets the component type name.
     * @return The string "FramebufferComponent"
     */
    virtual const char* getTypeName() const override {
        return "FramebufferComponent";
    }

    /**
     * @brief Retrieves a texture attachment from the framebuffer by name.
     * @param name The name of the texture attachment
     * @return Shared pointer to the texture
     * @throws exception::FramebufferException if texture name doesn't exist
     * 
     * Convenience method for accessing framebuffer textures without directly
     * accessing the framebuffer object. Useful for retrieving textures for
     * use in subsequent rendering passes.
     * 
     * @code
     * auto fbo_comp = node->getPayload().get<FramebufferComponent>();
     * auto scene_texture = fbo_comp->getTexture("scene_color");
     * texture::stack()->push(scene_texture, 0);
     * @endcode
     */
    texture::TexturePtr getTexture(const std::string& name) const {
        return m_fbo->getTexture(name);
    }

    /**
     * @brief Gets the managed framebuffer object.
     * @return Shared pointer to the framebuffer
     * 
     * Provides direct access to the underlying framebuffer object for
     * advanced operations like mipmap generation or shader attachment.
     * 
     * @code
     * auto fbo_comp = node->getPayload().get<FramebufferComponent>();
     * auto fbo = fbo_comp->getFramebuffer();
     * fbo->generateMipmaps("scene_color");
     * @endcode
     */
    framebuffer::FramebufferPtr getFramebuffer() const {
        return m_fbo;
    }

    /**
     * @brief Sets a new framebuffer for this component.
     * @param fbo The new framebuffer to manage
     * 
     * Replaces the currently managed framebuffer with a new one. Useful for
     * dynamically swapping framebuffers during runtime.
     * 
     * @note If the component is currently applied (framebuffer is on the stack),
     *       this will not affect the stack until the next apply() call.
     */
    void setFramebuffer(framebuffer::FramebufferPtr fbo) {
        m_fbo = fbo;
    }
};

} // namespace component

#endif // FRAMEBUFFER_COMPONENT_H
