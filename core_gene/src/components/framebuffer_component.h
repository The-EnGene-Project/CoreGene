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
    framebuffer::RenderStatePtr m_render_state;  ///< Optional pre-configured render state
    
protected:
    /**
     * @brief Protected constructor for factory methods (inherit mode).
     * @param fbo The framebuffer to manage
     * 
     * Initializes the component with priority 150, positioning it between
     * Transform (100) and Shader (200) in the execution order.
     * Sets m_render_state to nullptr, enabling inherit mode push.
     */
    FramebufferComponent(framebuffer::FramebufferPtr fbo) :
        Component(150),  // Priority 150: after Transform, before Shader
        m_fbo(fbo),
        m_render_state(nullptr)
    {}
    
    /**
     * @brief Protected constructor for factory methods (apply mode).
     * @param fbo The framebuffer to manage
     * @param state Pre-configured render state to apply atomically
     * 
     * Initializes the component with priority 150, positioning it between
     * Transform (100) and Shader (200) in the execution order.
     * Sets m_render_state, enabling apply mode push.
     */
    FramebufferComponent(framebuffer::FramebufferPtr fbo, framebuffer::RenderStatePtr state) :
        Component(150),  // Priority 150: after Transform, before Shader
        m_fbo(fbo),
        m_render_state(state)
    {}

public:
    /**
     * @brief Factory method to create a FramebufferComponent (inherit mode).
     * @param fbo The framebuffer to manage
     * @return Shared pointer to the created component
     * 
     * Creates a FramebufferComponent that will manage the given framebuffer's
     * lifecycle within the scene graph. Uses inherit mode: stencil and blend
     * state are inherited from the parent framebuffer level.
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
     * @brief Factory method to create a named FramebufferComponent (inherit mode).
     * @param fbo The framebuffer to manage
     * @param name The name for this component instance
     * @return Shared pointer to the created component
     * 
     * Creates a named FramebufferComponent, allowing retrieval by name from
     * the component collection. Uses inherit mode: stencil and blend state
     * are inherited from the parent framebuffer level.
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
     * @brief Factory method to create a FramebufferComponent with RenderState (apply mode).
     * @param fbo The framebuffer to manage
     * @param state Pre-configured render state to apply atomically
     * @return Shared pointer to the created component
     * 
     * Creates a FramebufferComponent that will manage the given framebuffer's
     * lifecycle within the scene graph. Uses apply mode: the provided RenderState
     * is applied atomically when the framebuffer is pushed, overriding parent state.
     * 
     * @code
     * auto fbo = framebuffer::Framebuffer::MakeRenderToTexture(800, 600, "color");
     * auto state = std::make_shared<framebuffer::RenderState>();
     * state->stencil().setTest(true);
     * state->blend().setEnabled(true);
     * auto comp = component::FramebufferComponent::Make(fbo, state);
     * @endcode
     */
    static FramebufferComponentPtr Make(framebuffer::FramebufferPtr fbo, framebuffer::RenderStatePtr state) {
        return FramebufferComponentPtr(new FramebufferComponent(fbo, state));
    }
    
    /**
     * @brief Factory method to create a named FramebufferComponent with RenderState (apply mode).
     * @param fbo The framebuffer to manage
     * @param state Pre-configured render state to apply atomically
     * @param name The name for this component instance
     * @return Shared pointer to the created component
     * 
     * Creates a named FramebufferComponent with pre-configured render state.
     * Uses apply mode: the provided RenderState is applied atomically when
     * the framebuffer is pushed, overriding parent state.
     * 
     * @code
     * auto fbo = framebuffer::Framebuffer::MakeRenderToTexture(800, 600, "color");
     * auto state = std::make_shared<framebuffer::RenderState>();
     * state->stencil().setTest(true);
     * auto comp = component::FramebufferComponent::Make(fbo, state, "stencil_fbo");
     * @endcode
     */
    static FramebufferComponentPtr Make(framebuffer::FramebufferPtr fbo, framebuffer::RenderStatePtr state, const std::string& name) {
        auto comp = FramebufferComponentPtr(new FramebufferComponent(fbo, state));
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
     * If m_render_state is null (inherit mode), the framebuffer inherits
     * stencil and blend state from the parent level without issuing OpenGL calls.
     * 
     * If m_render_state is not null (apply mode), the pre-configured state
     * is applied atomically, issuing OpenGL calls only for state fields that
     * differ from the cached GPU state.
     * 
     * @note This method has side effects (modifies OpenGL state via stack)
     */
    virtual void apply() override {
        if (m_render_state == nullptr) {
            // Inherit mode: inherit parent state, no GL calls
            framebuffer::stack()->push(m_fbo);
        } else {
            // Apply mode: apply pre-configured state atomically
            framebuffer::stack()->push(m_fbo, m_render_state);
        }
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
    
    /**
     * @brief Gets the current render state (may be null).
     * @return Shared pointer to the render state, or nullptr if using inherit mode
     * 
     * Returns the pre-configured render state if this component is using apply mode,
     * or nullptr if using inherit mode.
     * 
     * @code
     * auto fbo_comp = node->getPayload().get<FramebufferComponent>();
     * auto state = fbo_comp->getRenderState();
     * if (state) {
     *     // Component is using apply mode
     * } else {
     *     // Component is using inherit mode
     * }
     * @endcode
     */
    framebuffer::RenderStatePtr getRenderState() const {
        return m_render_state;
    }
    
    /**
     * @brief Sets a new render state for this component.
     * @param state The new render state to apply, or nullptr for inherit mode
     * 
     * Replaces the currently configured render state. Set to nullptr to switch
     * to inherit mode, or provide a RenderState to switch to apply mode.
     * 
     * @note If the component is currently applied (framebuffer is on the stack),
     *       this will not affect the stack until the next apply() call.
     * 
     * @code
     * auto fbo_comp = node->getPayload().get<FramebufferComponent>();
     * 
     * // Switch to apply mode with custom state
     * auto state = std::make_shared<framebuffer::RenderState>();
     * state->blend().setEnabled(true);
     * fbo_comp->setRenderState(state);
     * 
     * // Switch back to inherit mode
     * fbo_comp->setRenderState(nullptr);
     * @endcode
     */
    void setRenderState(framebuffer::RenderStatePtr state) {
        m_render_state = state;
    }
};

} // namespace component

#endif // FRAMEBUFFER_COMPONENT_H
