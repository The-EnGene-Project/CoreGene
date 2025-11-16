#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H


#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include "gl_includes.h"
#include "error.h"
#include "../exceptions/framebuffer_exception.h"
#include "texture.h"
#include "shader.h"

/**
 * @file framebuffer.h
 * @brief Framebuffer Object (FBO) abstraction for off-screen rendering
 * 
 * This file provides a RAII-compliant abstraction for OpenGL Framebuffer Objects,
 * enabling advanced rendering techniques like post-processing, shadow mapping,
 * reflections, and deferred shading.
 * 
 * @section usage_examples Usage Examples
 * 
 * @subsection basic_render_to_texture Basic Render-to-Texture
 * @code
 * // Create FBO with color texture and depth renderbuffer
 * auto fbo = framebuffer::Framebuffer::MakeRenderToTexture(800, 600, "scene_color");
 * 
 * // Render scene to FBO
 * framebuffer::stack()->push(fbo);
 * glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
 * scene::graph()->draw();
 * framebuffer::stack()->pop();
 * 
 * // Use FBO texture in next pass
 * auto scene_texture = fbo->getTexture("scene_color");
 * texture::stack()->push(scene_texture, 0);
 * // ... render fullscreen quad with texture ...
 * texture::stack()->pop();
 * @endcode
 * 
 * @subsection post_processing Post-Processing Pipeline
 * @code
 * // Create post-processing FBO (no depth attachment)
 * auto post_fbo = framebuffer::Framebuffer::MakePostProcessing(800, 600, "post_color");
 * 
 * // Render scene to FBO
 * framebuffer::stack()->push(post_fbo);
 * glClear(GL_COLOR_BUFFER_BIT);
 * scene::graph()->draw();
 * framebuffer::stack()->pop();
 * 
 * // Apply post-processing effect
 * auto shader = shader::Make("post_process.vert", "blur.frag");
 * post_fbo->attachToShader(shader, {{"post_color", "u_scene_texture"}});
 * 
 * shader::stack()->push(shader);
 * texture::stack()->registerSamplerUnit("u_scene_texture", 0);
 * texture::stack()->push(post_fbo->getTexture("post_color"), 0);
 * // ... render fullscreen quad ...
 * texture::stack()->pop();
 * shader::stack()->pop();
 * @endcode
 * 
 * @subsection shadow_mapping Shadow Mapping
 * @code
 * // Create shadow map FBO (depth-only)
 * auto shadow_fbo = framebuffer::Framebuffer::MakeShadowMap(1024, 1024, "shadow_depth");
 * 
 * // Render from light's perspective
 * framebuffer::stack()->push(shadow_fbo);
 * glClear(GL_DEPTH_BUFFER_BIT);
 * // ... render scene with depth-only shader ...
 * framebuffer::stack()->pop();
 * 
 * // Use shadow map in main rendering pass
 * auto shadow_texture = shadow_fbo->getTexture("shadow_depth");
 * texture::stack()->push(shadow_texture, 1);  // Bind to texture unit 1
 * // ... render scene with shadow comparison ...
 * texture::stack()->pop();
 * @endcode
 * 
 * @subsection deferred_shading Deferred Shading (G-Buffer)
 * @code
 * // Create G-Buffer with multiple render targets
 * auto gbuffer = framebuffer::Framebuffer::MakeGBuffer(800, 600, {
 *     "g_position",
 *     "g_normal",
 *     "g_albedo",
 *     "g_specular"
 * });
 * 
 * // Geometry pass: render to G-Buffer
 * framebuffer::stack()->push(gbuffer);
 * glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
 * // ... render scene with MRT shader ...
 * framebuffer::stack()->pop();
 * 
 * // Lighting pass: read from G-Buffer
 * auto lighting_shader = shader::Make("lighting.vert", "lighting.frag");
 * gbuffer->attachToShader(lighting_shader, {
 *     {"g_position", "u_position"},
 *     {"g_normal", "u_normal"},
 *     {"g_albedo", "u_albedo"},
 *     {"g_specular", "u_specular"}
 * });
 * 
 * shader::stack()->push(lighting_shader);
 * texture::stack()->registerSamplerUnit("u_position", 0);
 * texture::stack()->registerSamplerUnit("u_normal", 1);
 * texture::stack()->registerSamplerUnit("u_albedo", 2);
 * texture::stack()->registerSamplerUnit("u_specular", 3);
 * texture::stack()->push(gbuffer->getTexture("g_position"), 0);
 * texture::stack()->push(gbuffer->getTexture("g_normal"), 1);
 * texture::stack()->push(gbuffer->getTexture("g_albedo"), 2);
 * texture::stack()->push(gbuffer->getTexture("g_specular"), 3);
 * // ... render fullscreen quad with lighting calculations ...
 * texture::stack()->pop();
 * texture::stack()->pop();
 * texture::stack()->pop();
 * texture::stack()->pop();
 * shader::stack()->pop();
 * @endcode
 * 
 * @subsection component_integration Scene Graph Integration
 * @code
 * // Create FBO and attach to scene node
 * auto fbo = framebuffer::Framebuffer::MakeRenderToTexture(800, 600, "scene_color");
 * 
 * scene::graph()->addNode("offscreen_render")
 *     .with<component::FramebufferComponent>(fbo)
 *     .with<component::ShaderComponent>(shader)
 *     .addNode("cube")
 *         .with<component::GeometryComponent>(cube_geometry);
 * 
 * // Framebuffer is automatically pushed/popped during scene traversal
 * scene::graph()->draw();
 * 
 * // Retrieve texture for use in another pass
 * auto fbo_comp = scene::graph()->getNode("offscreen_render")
 *                     ->getPayload().get<component::FramebufferComponent>();
 * auto texture = fbo_comp->getTexture("scene_color");
 * @endcode
 * 
 * @subsection custom_configuration Custom Configuration
 * @code
 * // Create FBO with custom attachment configuration
 * auto custom_fbo = framebuffer::Framebuffer::Make(800, 600, {
 *     // HDR color attachment
 *     framebuffer::Framebuffer::AttachmentSpec(
 *         framebuffer::attachment::Point::Color0,
 *         framebuffer::attachment::Format::RGBA16F,
 *         "hdr_color"),
 *     // Integer ID attachment for picking
 *     framebuffer::Framebuffer::AttachmentSpec(
 *         framebuffer::attachment::Point::Color1,
 *         framebuffer::attachment::Format::R32I,
 *         "object_id"),
 *     // High-precision depth texture
 *     framebuffer::Framebuffer::AttachmentSpec(
 *         framebuffer::attachment::Point::Depth,
 *         framebuffer::attachment::Format::DepthComponent32F,
 *         "depth_map")
 * });
 * @endcode
 * 
 * @subsection mipmap_generation Mipmap Generation
 * @code
 * auto fbo = framebuffer::Framebuffer::MakeRenderToTexture(800, 600, "scene_color");
 * 
 * // Render to FBO
 * framebuffer::stack()->push(fbo);
 * scene::graph()->draw();
 * framebuffer::stack()->pop();
 * 
 * // Generate mipmaps for downsampling effects
 * fbo->generateMipmaps("scene_color");
 * 
 * // Use texture with mipmaps
 * auto texture = fbo->getTexture("scene_color");
 * texture::stack()->push(texture, 0);
 * @endcode
 */

namespace framebuffer {

// Forward declarations
class Framebuffer;
class FramebufferStack;

using FramebufferPtr = std::shared_ptr<Framebuffer>;
using FramebufferStackPtr = std::shared_ptr<FramebufferStack>;

/**
 * @namespace framebuffer::attachment
 * @brief Contains enums for framebuffer attachment configuration
 * 
 * This namespace provides type-safe enums for specifying attachment points,
 * internal formats, and storage types, replacing raw OpenGL constants.
 */
namespace attachment {

/**
 * @enum Point
 * @brief Framebuffer attachment points
 * 
 * Specifies where an attachment connects to the framebuffer.
 * Supports up to 8 color attachments (MRT) plus depth/stencil.
 */
enum class Point {
    Color0,        ///< First color attachment (GL_COLOR_ATTACHMENT0)
    Color1,        ///< Second color attachment (GL_COLOR_ATTACHMENT1)
    Color2,        ///< Third color attachment (GL_COLOR_ATTACHMENT2)
    Color3,        ///< Fourth color attachment (GL_COLOR_ATTACHMENT3)
    Color4,        ///< Fifth color attachment (GL_COLOR_ATTACHMENT4)
    Color5,        ///< Sixth color attachment (GL_COLOR_ATTACHMENT5)
    Color6,        ///< Seventh color attachment (GL_COLOR_ATTACHMENT6)
    Color7,        ///< Eighth color attachment (GL_COLOR_ATTACHMENT7)
    Depth,         ///< Depth attachment (GL_DEPTH_ATTACHMENT)
    Stencil,       ///< Stencil attachment (GL_STENCIL_ATTACHMENT)
    DepthStencil   ///< Combined depth-stencil attachment (GL_DEPTH_STENCIL_ATTACHMENT)
};

/**
 * @enum Format
 * @brief Internal formats for framebuffer attachments
 * 
 * Specifies how attachment data is stored in GPU memory.
 * Includes color formats (LDR/HDR), depth formats, and combined formats.
 */
enum class Format {
    // Color formats (LDR - Low Dynamic Range)
    RGBA8,              ///< 8-bit RGBA color (GL_RGBA8)
    RGB8,               ///< 8-bit RGB color (GL_RGB8)
    
    // Color formats (HDR - High Dynamic Range)
    RGBA16F,            ///< 16-bit floating-point RGBA (GL_RGBA16F)
    RGBA32F,            ///< 32-bit floating-point RGBA (GL_RGBA32F)
    RGB16F,             ///< 16-bit floating-point RGB (GL_RGB16F)
    RGB32F,             ///< 32-bit floating-point RGB (GL_RGB32F)
    
    // Integer formats (for data storage)
    R32I,               ///< 32-bit signed integer, single channel (GL_R32I)
    R32UI,              ///< 32-bit unsigned integer, single channel (GL_R32UI)
    RG32UI,             ///< 32-bit unsigned integer, two channels (GL_RG32UI)
    
    // Depth formats
    DepthComponent16,   ///< 16-bit depth (GL_DEPTH_COMPONENT16)
    DepthComponent24,   ///< 24-bit depth (GL_DEPTH_COMPONENT24)
    DepthComponent32,   ///< 32-bit depth (GL_DEPTH_COMPONENT32)
    DepthComponent32F,  ///< 32-bit floating-point depth (GL_DEPTH_COMPONENT32F)
    
    // Stencil formats
    StencilIndex8,      ///< 8-bit stencil (GL_STENCIL_INDEX8)
    
    // Combined depth-stencil
    Depth24Stencil8     ///< 24-bit depth + 8-bit stencil (GL_DEPTH24_STENCIL8)
};

/**
 * @enum StorageType
 * @brief Storage type for framebuffer attachments
 * 
 * Specifies whether an attachment is a texture (readable by shaders)
 * or a renderbuffer (write-only, optimized for rendering).
 */
enum class StorageType {
    Texture,      ///< Texture attachment (can be read by shaders)
    Renderbuffer  ///< Renderbuffer attachment (write-only, optimized)
};

/**
 * @enum TextureFilter
 * @brief Texture filtering modes for texture attachments
 */
enum class TextureFilter {
    Nearest,      ///< Nearest-neighbor filtering (GL_NEAREST)
    Linear        ///< Linear filtering (GL_LINEAR)
};

/**
 * @enum TextureWrap
 * @brief Texture wrapping modes for texture attachments
 */
enum class TextureWrap {
    ClampToEdge,  ///< Clamp to edge (GL_CLAMP_TO_EDGE)
    ClampToBorder,///< Clamp to border (GL_CLAMP_TO_BORDER)
    Repeat        ///< Repeat (GL_REPEAT)
};

/**
 * @brief Converts attachment::Point enum to OpenGL constant
 * @param point The attachment point enum value
 * @return The corresponding OpenGL attachment point constant
 * @throws exception::FramebufferException if point is invalid
 */
inline GLenum toGLAttachmentPoint(Point point) {
    switch (point) {
        case Point::Color0: return GL_COLOR_ATTACHMENT0;
        case Point::Color1: return GL_COLOR_ATTACHMENT1;
        case Point::Color2: return GL_COLOR_ATTACHMENT2;
        case Point::Color3: return GL_COLOR_ATTACHMENT3;
        case Point::Color4: return GL_COLOR_ATTACHMENT4;
        case Point::Color5: return GL_COLOR_ATTACHMENT5;
        case Point::Color6: return GL_COLOR_ATTACHMENT6;
        case Point::Color7: return GL_COLOR_ATTACHMENT7;
        case Point::Depth: return GL_DEPTH_ATTACHMENT;
        case Point::Stencil: return GL_STENCIL_ATTACHMENT;
        case Point::DepthStencil: return GL_DEPTH_STENCIL_ATTACHMENT;
        default:
            throw exception::FramebufferException("Invalid attachment point");
    }
}

/**
 * @brief Converts attachment::Format enum to OpenGL constant
 * @param format The internal format enum value
 * @return The corresponding OpenGL internal format constant
 * @throws exception::FramebufferException if format is invalid
 */
inline GLenum toGLFormat(Format format) {
    switch (format) {
        // Color formats (LDR)
        case Format::RGBA8: return GL_RGBA8;
        case Format::RGB8: return GL_RGB8;
        
        // Color formats (HDR)
        case Format::RGBA16F: return GL_RGBA16F;
        case Format::RGBA32F: return GL_RGBA32F;
        case Format::RGB16F: return GL_RGB16F;
        case Format::RGB32F: return GL_RGB32F;
        
        // Integer formats
        case Format::R32I: return GL_R32I;
        case Format::R32UI: return GL_R32UI;
        case Format::RG32UI: return GL_RG32UI;
        
        // Depth formats
        case Format::DepthComponent16: return GL_DEPTH_COMPONENT16;
        case Format::DepthComponent24: return GL_DEPTH_COMPONENT24;
        case Format::DepthComponent32: return GL_DEPTH_COMPONENT32;
        case Format::DepthComponent32F: return GL_DEPTH_COMPONENT32F;
        
        // Stencil formats
        case Format::StencilIndex8: return GL_STENCIL_INDEX8;
        
        // Combined depth-stencil
        case Format::Depth24Stencil8: return GL_DEPTH24_STENCIL8;
        
        default:
            throw exception::FramebufferException("Invalid format");
    }
}

/**
 * @brief Converts attachment::TextureFilter enum to OpenGL constant
 * @param filter The texture filter enum value
 * @return The corresponding OpenGL filter constant
 */
inline GLenum toGLTextureFilter(TextureFilter filter) {
    switch (filter) {
        case TextureFilter::Nearest: return GL_NEAREST;
        case TextureFilter::Linear: return GL_LINEAR;
        default: return GL_LINEAR;
    }
}

/**
 * @brief Converts attachment::TextureWrap enum to OpenGL constant
 * @param wrap The texture wrap enum value
 * @return The corresponding OpenGL wrap constant
 */
inline GLenum toGLTextureWrap(TextureWrap wrap) {
    switch (wrap) {
        case TextureWrap::ClampToEdge: return GL_CLAMP_TO_EDGE;
        case TextureWrap::ClampToBorder: return GL_CLAMP_TO_BORDER;
        case TextureWrap::Repeat: return GL_REPEAT;
        default: return GL_CLAMP_TO_EDGE;
    }
}

} // namespace attachment

/**
 * @class Framebuffer
 * @brief RAII-compliant abstraction for OpenGL Framebuffer Objects
 * 
 * The Framebuffer class provides a flexible interface for off-screen rendering,
 * supporting multiple attachment configurations for advanced rendering techniques
 * including post-processing, shadow mapping, reflections, and deferred shading.
 * 
 * Key features:
 * - Named texture access for multi-pass rendering
 * - Type-safe attachment configuration via enums
 * - Automatic resource cleanup via RAII
 * - Factory methods for common configurations
 * - Integration with EnGene's stack-based state management
 * 
 * @note All texture attachments are identified by user-defined names for easy retrieval.
 * @note Framebuffer objects use move semantics; copying is disabled.
 */
class Framebuffer {
public:
    /**
     * @struct AttachmentSpec
     * @brief Specification for a framebuffer attachment
     * 
     * Defines the configuration for a single framebuffer attachment,
     * including its attachment point, storage type, internal format,
     * texture filtering/wrapping parameters, and optional name.
     * 
     * @note Only texture attachments should have names; renderbuffers are not retrievable.
     */
    struct AttachmentSpec {
        attachment::Point point;           ///< Attachment point (Color0-7, Depth, Stencil, DepthStencil)
        attachment::Format format;         ///< Internal format (RGBA8, DepthComponent24, etc.)
        attachment::StorageType storage;   ///< Storage type (Texture or Renderbuffer)
        std::string name;                  ///< Optional name for texture retrieval (empty for renderbuffers)
        attachment::TextureFilter filter;  ///< Texture filtering mode (for texture attachments)
        attachment::TextureWrap wrap;      ///< Texture wrapping mode (for texture attachments)
        bool is_shadow_texture;            ///< Enable shadow comparison mode (for depth textures)
        
        /**
         * @brief Generalized constructor for attachment specifications
         * @param p Attachment point
         * @param f Internal format
         * @param s Storage type (defaults to Renderbuffer)
         * @param n Texture name for later retrieval (defaults to empty string)
         * @param filt Texture filtering mode (defaults to Linear)
         * @param w Texture wrapping mode (defaults to ClampToEdge)
         * @param is_shadow Enable shadow comparison mode (defaults to false)
         * 
         * Creates an attachment specification with full control over all parameters.
         * 
         * @note For renderbuffer attachments, leave name empty and storage as Renderbuffer.
         * @note For texture attachments, set storage to Texture and provide a name.
         * @note Shadow textures should use depth formats and set is_shadow to true.
         */
        AttachmentSpec(
            attachment::Point p, 
            attachment::Format f,
            attachment::StorageType s = attachment::StorageType::Renderbuffer,
            const std::string& n = "",
            attachment::TextureFilter filt = attachment::TextureFilter::Linear,
            attachment::TextureWrap w = attachment::TextureWrap::ClampToEdge,
            bool is_shadow = false)
            : point(p), 
              format(f), 
              storage(s), 
              name(n),
              filter(filt),
              wrap(w),
              is_shadow_texture(is_shadow) {}
    };

private:
    GLuint m_fbo_id;
    int m_width;
    int m_height;
    bool m_clear_on_bind;
    bool m_has_depth;
    bool m_has_stencil;
    
    // Storage for attachments
    std::unordered_map<std::string, texture::TexturePtr> m_named_textures;
    std::vector<GLuint> m_renderbuffers;
    std::vector<GLenum> m_color_attachments;  // For glDrawBuffers
    
    // Friend declaration for FramebufferStack
    friend class FramebufferStack;
    
    /**
     * @brief Creates a texture attachment and attaches it to the framebuffer.
     * @param spec The attachment specification
     * @throws exception::FramebufferException if attachment creation fails
     */
    void createTextureAttachment(const AttachmentSpec& spec) {
        // Generate texture ID
        GLuint texture_id;
        glGenTextures(1, &texture_id);
        GL_CHECK("generate texture for FBO attachment");
        
        // Bind and configure texture
        glBindTexture(GL_TEXTURE_2D, texture_id);
        GL_CHECK("bind texture for FBO attachment");
        
        // Set texture parameters from spec
        GLenum gl_wrap = attachment::toGLTextureWrap(spec.wrap);
        GLenum gl_filter = attachment::toGLTextureFilter(spec.filter);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, gl_wrap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, gl_wrap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter);
        
        // Configure shadow comparison mode if enabled
        if (spec.is_shadow_texture) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        }
        
        GL_CHECK("set texture parameters for FBO attachment");
        
        // Determine pixel format for glTexImage2D
        GLenum internal_format = attachment::toGLFormat(spec.format);
        GLenum pixel_format;
        GLenum pixel_type;
        
        // Determine pixel format and type based on internal format
        switch (spec.format) {
            case attachment::Format::RGBA8:
                pixel_format = GL_RGBA;
                pixel_type = GL_UNSIGNED_BYTE;
                break;
            case attachment::Format::RGB8:
                pixel_format = GL_RGB;
                pixel_type = GL_UNSIGNED_BYTE;
                break;
            case attachment::Format::RGBA16F:
            case attachment::Format::RGBA32F:
                pixel_format = GL_RGBA;
                pixel_type = GL_FLOAT;
                break;
            case attachment::Format::RGB16F:
            case attachment::Format::RGB32F:
                pixel_format = GL_RGB;
                pixel_type = GL_FLOAT;
                break;
            case attachment::Format::R32I:
                pixel_format = GL_RED_INTEGER;
                pixel_type = GL_INT;
                break;
            case attachment::Format::R32UI:
                pixel_format = GL_RED_INTEGER;
                pixel_type = GL_UNSIGNED_INT;
                break;
            case attachment::Format::RG32UI:
                pixel_format = GL_RG_INTEGER;
                pixel_type = GL_UNSIGNED_INT;
                break;
            case attachment::Format::DepthComponent16:
            case attachment::Format::DepthComponent24:
            case attachment::Format::DepthComponent32:
            case attachment::Format::DepthComponent32F:
                pixel_format = GL_DEPTH_COMPONENT;
                pixel_type = GL_FLOAT;
                break;
            case attachment::Format::StencilIndex8:
                pixel_format = GL_STENCIL_INDEX;
                pixel_type = GL_UNSIGNED_BYTE;
                break;
            case attachment::Format::Depth24Stencil8:
                pixel_format = GL_DEPTH_STENCIL;
                pixel_type = GL_UNSIGNED_INT_24_8;
                break;
            default:
                throw exception::FramebufferException("Unsupported texture format");
        }
        
        // Allocate texture storage
        glTexImage2D(GL_TEXTURE_2D, 0, internal_format, m_width, m_height, 0, 
                     pixel_format, pixel_type, nullptr);
        GL_CHECK("allocate texture storage for FBO attachment");
        
        // Attach texture to framebuffer
        GLenum attachment_point = attachment::toGLAttachmentPoint(spec.point);
        glFramebufferTexture2D(GL_FRAMEBUFFER, attachment_point, GL_TEXTURE_2D, texture_id, 0);
        GL_CHECK("attach texture to framebuffer");
        
        // Unbind texture
        glBindTexture(GL_TEXTURE_2D, 0);
        
        // Create Texture object and store in map
        texture::TexturePtr texture_ptr(new texture::Texture(texture_id, m_width, m_height));
        m_named_textures[spec.name] = texture_ptr;
        
        // Track color attachments for glDrawBuffers
        if (spec.point >= attachment::Point::Color0 && spec.point <= attachment::Point::Color7) {
            m_color_attachments.push_back(attachment_point);
        }
    }
    
    /**
     * @brief Creates a renderbuffer attachment and attaches it to the framebuffer.
     * @param spec The attachment specification
     * @throws exception::FramebufferException if attachment creation fails
     */
    void createRenderbufferAttachment(const AttachmentSpec& spec) {
        // Generate renderbuffer ID
        GLuint renderbuffer_id;
        glGenRenderbuffers(1, &renderbuffer_id);
        GL_CHECK("generate renderbuffer for FBO attachment");
        
        // Bind and configure renderbuffer
        glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer_id);
        GL_CHECK("bind renderbuffer for FBO attachment");
        
        // Allocate renderbuffer storage
        GLenum internal_format = attachment::toGLFormat(spec.format);
        glRenderbufferStorage(GL_RENDERBUFFER, internal_format, m_width, m_height);
        GL_CHECK("allocate renderbuffer storage for FBO attachment");
        
        // Attach renderbuffer to framebuffer
        GLenum attachment_point = attachment::toGLAttachmentPoint(spec.point);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment_point, GL_RENDERBUFFER, renderbuffer_id);
        GL_CHECK("attach renderbuffer to framebuffer");
        
        // Unbind renderbuffer
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        
        // Store renderbuffer ID for cleanup
        m_renderbuffers.push_back(renderbuffer_id);
    }
    
    /**
     * @brief Validates framebuffer completeness.
     * @throws exception::FramebufferException if framebuffer is incomplete
     */
    void validateCompleteness() const {
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            std::string error_message;
            
            switch (status) {
                case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                    error_message = "Framebuffer incomplete: One or more attachment points are incomplete";
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                    error_message = "Framebuffer incomplete: No attachments were specified";
                    break;
                case GL_FRAMEBUFFER_UNSUPPORTED:
                    error_message = "Framebuffer incomplete: The format combination is not supported by this implementation";
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
                    error_message = "Framebuffer incomplete: Multisample configuration mismatch";
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
                    error_message = "Framebuffer incomplete: Layer targets mismatch";
                    break;
                default:
                    error_message = "Framebuffer incomplete: Unknown error (status code: " + std::to_string(status) + ")";
                    break;
            }
            
            throw exception::FramebufferException(error_message);
        }
    }

protected:
    /**
     * @brief Protected constructor for factory methods.
     * @param width Framebuffer width in pixels
     * @param height Framebuffer height in pixels
     * @param specs Vector of attachment specifications
     * @throws exception::FramebufferException if framebuffer creation or validation fails
     */
    Framebuffer(int width, int height, const std::vector<AttachmentSpec>& specs)
        : m_fbo_id(0), m_width(width), m_height(height), m_clear_on_bind(true), 
          m_has_depth(false), m_has_stencil(false) {
        
        // Analyze attachment specs to determine what buffers we have
        for (const auto& spec : specs) {
            if (spec.point == attachment::Point::Depth) {
                m_has_depth = true;
            } else if (spec.point == attachment::Point::Stencil) {
                m_has_stencil = true;
            } else if (spec.point == attachment::Point::DepthStencil) {
                m_has_depth = true;
                m_has_stencil = true;
            }
        }
        
        // Generate framebuffer object
        glGenFramebuffers(1, &m_fbo_id);
        GL_CHECK("generate framebuffer");
        
        if (m_fbo_id == 0) {
            throw exception::FramebufferException("Failed to generate framebuffer object");
        }
        
        // Bind framebuffer for configuration
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_id);
        GL_CHECK("bind framebuffer for configuration");
        
        // Create attachments
        for (const auto& spec : specs) {
            if (spec.storage == attachment::StorageType::Texture) {
                createTextureAttachment(spec);
            } else {
                createRenderbufferAttachment(spec);
            }
        }
        
        // Validate framebuffer completeness
        validateCompleteness();
        
        // Unbind framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

public:
    /**
     * @brief Destructor - automatically cleans up OpenGL resources.
     */
    ~Framebuffer() {
        // Delete renderbuffers
        if (!m_renderbuffers.empty()) {
            glDeleteRenderbuffers(static_cast<GLsizei>(m_renderbuffers.size()), m_renderbuffers.data());
        }
        
        // Textures are managed by shared_ptr, will be deleted automatically
        
        // Delete framebuffer
        if (m_fbo_id != 0) {
            glDeleteFramebuffers(1, &m_fbo_id);
        }
    }
    
    /**
     * @brief Move constructor.
     */
    Framebuffer(Framebuffer&& other) noexcept
        : m_fbo_id(other.m_fbo_id),
          m_width(other.m_width),
          m_height(other.m_height),
          m_named_textures(std::move(other.m_named_textures)),
          m_renderbuffers(std::move(other.m_renderbuffers)),
          m_color_attachments(std::move(other.m_color_attachments)) {
        
        // Invalidate the moved-from object
        other.m_fbo_id = 0;
        other.m_width = 0;
        other.m_height = 0;
    }
    
    /**
     * @brief Move assignment operator.
     */
    Framebuffer& operator=(Framebuffer&& other) noexcept {
        if (this != &other) {
            // Clean up existing resources
            if (!m_renderbuffers.empty()) {
                glDeleteRenderbuffers(static_cast<GLsizei>(m_renderbuffers.size()), m_renderbuffers.data());
            }
            if (m_fbo_id != 0) {
                glDeleteFramebuffers(1, &m_fbo_id);
            }
            
            // Move resources from other
            m_fbo_id = other.m_fbo_id;
            m_width = other.m_width;
            m_height = other.m_height;
            m_named_textures = std::move(other.m_named_textures);
            m_renderbuffers = std::move(other.m_renderbuffers);
            m_color_attachments = std::move(other.m_color_attachments);
            
            // Invalidate the moved-from object
            other.m_fbo_id = 0;
            other.m_width = 0;
            other.m_height = 0;
        }
        return *this;
    }
    
    // Delete copy constructor and copy assignment operator
    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;
    
    /**
     * @brief Factory method for custom framebuffer configuration.
     * @param width Framebuffer width in pixels
     * @param height Framebuffer height in pixels
     * @param specs Vector of attachment specifications
     * @return Shared pointer to the created framebuffer
     * @throws exception::FramebufferException if framebuffer creation fails
     * 
     * This is the most flexible factory method, allowing complete control
     * over attachment configuration. Use this when the preset factories
     * don't match your requirements.
     * 
     * @code
     * auto fbo = framebuffer::Framebuffer::Make(800, 600, {
     *     framebuffer::Framebuffer::AttachmentSpec(
     *         framebuffer::attachment::Point::Color0,
     *         framebuffer::attachment::Format::RGBA16F,
     *         "hdr_color"),
     *     framebuffer::Framebuffer::AttachmentSpec(
     *         framebuffer::attachment::Point::Depth,
     *         framebuffer::attachment::Format::DepthComponent24)
     * });
     * @endcode
     */
    static FramebufferPtr Make(int width, int height, 
                                const std::vector<AttachmentSpec>& specs) {
        return FramebufferPtr(new Framebuffer(width, height, specs));
    }
    
    /**
     * @brief Factory method for render-to-texture configuration.
     * @param width Framebuffer width in pixels
     * @param height Framebuffer height in pixels
     * @param colorTextureName Name for the color texture attachment
     * @param colorFormat Internal format for the color attachment (default: RGBA8)
     * @param depthFormat Internal format for the depth attachment (default: DepthComponent24)
     * @return Shared pointer to the created framebuffer
     * @throws exception::FramebufferException if framebuffer creation fails
     * 
     * Creates a framebuffer with one color texture attachment and one depth
     * renderbuffer attachment. This is the most common configuration for
     * off-screen rendering, reflections, and shadow receivers.
     * 
     * The color texture can be retrieved by name and used in subsequent
     * rendering passes. The depth attachment is a renderbuffer (not readable).
     * 
     * @code
     * auto fbo = framebuffer::Framebuffer::MakeRenderToTexture(800, 600, "scene_color");
     * // Later: auto texture = fbo->getTexture("scene_color");
     * @endcode
     */
    static FramebufferPtr MakeRenderToTexture(
        int width, int height, 
        const std::string& colorTextureName,
        attachment::Format colorFormat = attachment::Format::RGBA8,
        attachment::Format depthFormat = attachment::Format::DepthComponent24) {
        
        std::vector<AttachmentSpec> specs = {
            AttachmentSpec(attachment::Point::Color0, colorFormat, 
                          attachment::StorageType::Texture, colorTextureName),
            AttachmentSpec(attachment::Point::Depth, depthFormat)  // Renderbuffer (default)
        };
        return Make(width, height, specs);
    }
    
    /**
     * @brief Factory method for post-processing configuration.
     * @param width Framebuffer width in pixels
     * @param height Framebuffer height in pixels
     * @param colorTextureName Name for the color texture attachment
     * @param colorFormat Internal format for the color attachment (default: RGBA8)
     * @param depthFormat Internal format for the depth attachment (default: DepthComponent24)
     * @return Shared pointer to the created framebuffer
     * @throws exception::FramebufferException if framebuffer creation fails
     * 
     * Creates a framebuffer with one color texture attachment and one depth
     * renderbuffer attachment. This configuration is optimized for post-processing
     * effects while maintaining proper depth testing during the initial render pass.
     * 
     * @code
     * auto fbo = framebuffer::Framebuffer::MakePostProcessing(800, 600, "post_color");
     * // Render scene to fbo with depth testing, then apply post-processing using the texture
     * @endcode
     * 
     * @note The depth attachment is a renderbuffer (not readable), suitable for
     *       standard depth testing during rendering. If you need to read depth values
     *       in shaders, use the custom Make() factory with a depth texture attachment.
     */
    static FramebufferPtr MakePostProcessing(
        int width, int height, 
        const std::string& colorTextureName,
        attachment::Format colorFormat = attachment::Format::RGBA8,
        attachment::Format depthFormat = attachment::Format::DepthComponent24) {
        
        std::vector<AttachmentSpec> specs = {
            AttachmentSpec(attachment::Point::Color0, colorFormat, 
                          attachment::StorageType::Texture, colorTextureName),
            AttachmentSpec(attachment::Point::Depth, depthFormat)  // Renderbuffer (default)
        };
        return Make(width, height, specs);
    }
    
    /**
     * @brief Factory method for shadow map configuration.
     * @param width Shadow map width in pixels
     * @param height Shadow map height in pixels
     * @param depthTextureName Name for the depth texture attachment
     * @param depthFormat Internal format for the depth attachment (default: DepthComponent24)
     * @return Shared pointer to the created framebuffer
     * @throws exception::FramebufferException if framebuffer creation fails
     * 
     * Creates a framebuffer with one depth texture attachment and no color
     * attachments. This configuration is used for shadow mapping, where only
     * depth information is needed.
     * 
     * The framebuffer is automatically configured with GL_NONE for draw buffers,
     * preventing fragment shader color output errors.
     * 
     * @code
     * auto shadow_fbo = framebuffer::Framebuffer::MakeShadowMap(1024, 1024, "shadow_depth");
     * // Render from light's perspective, then use shadow_depth texture for shadow testing
     * @endcode
     * 
     * @note The depth texture can be sampled in shaders for shadow comparison.
     * @note Consider using DepthComponent32F for higher precision shadow maps.
     */
    static FramebufferPtr MakeShadowMap(
        int width, int height, 
        const std::string& depthTextureName,
        attachment::Format depthFormat = attachment::Format::DepthComponent24) {
        
        std::vector<AttachmentSpec> specs = {
            AttachmentSpec(attachment::Point::Depth, depthFormat, 
                          attachment::StorageType::Texture, depthTextureName,
                          attachment::TextureFilter::Linear,
                          attachment::TextureWrap::ClampToBorder,
                          true)  // Enable shadow comparison mode
        };
        return Make(width, height, specs);
    }
    
    /**
     * @brief Factory method for G-Buffer configuration (deferred shading).
     * @param width G-Buffer width in pixels
     * @param height G-Buffer height in pixels
     * @param colorTextureNames Vector of names for color texture attachments
     * @param colorFormat Internal format for all color attachments (default: RGBA16F)
     * @param depthFormat Internal format for the depth attachment (default: DepthComponent24)
     * @return Shared pointer to the created framebuffer
     * @throws exception::FramebufferException if framebuffer creation fails
     * 
     * Creates a framebuffer with multiple color texture attachments (up to 8)
     * and one depth renderbuffer attachment. This configuration is used for
     * deferred shading, where geometry data is written to multiple render
     * targets in a single pass.
     * 
     * Each color texture is assigned to sequential attachment points starting
     * from Color0. All textures can be retrieved by name for use in the
     * lighting pass.
     * 
     * @code
     * auto gbuffer = framebuffer::Framebuffer::MakeGBuffer(800, 600, {
     *     "g_position",
     *     "g_normal",
     *     "g_albedo",
     *     "g_specular"
     * });
     * // Geometry pass writes to all 4 textures
     * // Lighting pass reads from all 4 textures
     * @endcode
     * 
     * @note Maximum of 8 color attachments (OpenGL 4.3 minimum guarantee).
     * @note HDR format (RGBA16F) is recommended for accurate lighting calculations.
     */
    static FramebufferPtr MakeGBuffer(
        int width, int height, 
        const std::vector<std::string>& colorTextureNames,
        attachment::Format colorFormat = attachment::Format::RGBA16F,
        attachment::Format depthFormat = attachment::Format::DepthComponent24) {
        
        std::vector<AttachmentSpec> specs;
        
        // Add color attachments (up to 8)
        for (size_t i = 0; i < colorTextureNames.size() && i < 8; ++i) {
            attachment::Point point = static_cast<attachment::Point>(
                static_cast<int>(attachment::Point::Color0) + i);
            specs.push_back(AttachmentSpec(point, colorFormat, 
                                          attachment::StorageType::Texture, 
                                          colorTextureNames[i]));
        }
        
        // Add depth renderbuffer
        specs.push_back(AttachmentSpec(attachment::Point::Depth, depthFormat));
        
        return Make(width, height, specs);
    }
    
    /**
     * @brief Binds the framebuffer (internal use by FramebufferStack).
     * 
     * Binds this framebuffer as the current render target via glBindFramebuffer.
     * If clear-on-bind is enabled, automatically clears the appropriate buffers.
     * This is a low-level method used internally by FramebufferStack.
     * 
     * @note Application code should use framebuffer::stack()->push() instead
     *       of calling this method directly. The stack manages viewport and
     *       draw buffer configuration automatically.
     * @note This method does not set viewport or draw buffers; those are
     *       managed by FramebufferStack.
     */
    void bind() {
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_id);
        GL_CHECK("bind framebuffer");
        
        // Automatically clear framebuffer if enabled
        if (m_clear_on_bind) {
            GLbitfield clear_mask = 0;
            
            // Determine what to clear based on tracked attachments
            if (!m_color_attachments.empty()) {
                clear_mask |= GL_COLOR_BUFFER_BIT;
            }
            
            if (m_has_depth) {
                clear_mask |= GL_DEPTH_BUFFER_BIT;
            }
            
            if (m_has_stencil) {
                clear_mask |= GL_STENCIL_BUFFER_BIT;
            }
            
            if (clear_mask != 0) {
                glClear(clear_mask);
                GL_CHECK("auto-clear framebuffer on bind");
            }
        }
    }
    
    /**
     * @brief Unbinds the framebuffer (internal use by FramebufferStack).
     * 
     * Binds the default framebuffer (ID 0) as the current render target.
     * This is a low-level method used internally by FramebufferStack.
     * 
     * @note Application code should use framebuffer::stack()->pop() instead
     *       of calling this method directly. The stack manages state restoration
     *       automatically.
     */
    void unbind() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        GL_CHECK("unbind framebuffer");
    }
    
    /**
     * @brief Retrieves a texture attachment by name.
     * @param name The name of the texture attachment
     * @return Shared pointer to the texture
     * @throws exception::FramebufferException if texture name doesn't exist or refers to a renderbuffer
     * 
     * Retrieves a texture attachment by its user-defined name for use in
     * subsequent rendering passes. The texture can be bound to texture units
     * and sampled in shaders.
     * 
     * @code
     * auto fbo = framebuffer::Framebuffer::MakeRenderToTexture(800, 600, "scene_color");
     * // ... render to FBO ...
     * auto texture = fbo->getTexture("scene_color");
     * texture::stack()->push(texture, 0);
     * @endcode
     * 
     * @note Only texture attachments can be retrieved; renderbuffer attachments
     *       are not accessible (they are write-only).
     * @note The returned texture is managed by shared_ptr and can be safely
     *       stored and used after the framebuffer is destroyed.
     */
    texture::TexturePtr getTexture(const std::string& name) const {
        auto it = m_named_textures.find(name);
        if (it == m_named_textures.end()) {
            throw exception::FramebufferException("Texture '" + name + "' not found in framebuffer");
        }
        return it->second;
    }
    
    /**
     * @brief Checks if a named texture exists in the framebuffer.
     * @param name The name of the texture attachment
     * @return True if the texture exists, false otherwise
     * 
     * Use this method to check if a texture attachment exists before
     * attempting to retrieve it, avoiding exceptions.
     * 
     * @code
     * if (fbo->hasTexture("scene_color")) {
     *     auto texture = fbo->getTexture("scene_color");
     *     // ... use texture ...
     * }
     * @endcode
     */
    bool hasTexture(const std::string& name) const {
        return m_named_textures.find(name) != m_named_textures.end();
    }
    
    /**
     * @brief Generates mipmaps for a named texture attachment.
     * @param textureName The name of the texture attachment
     * @throws exception::FramebufferException if texture doesn't exist
     * 
     * Generates a mipmap chain for the specified texture attachment and
     * configures texture parameters to enable mipmap filtering. This is
     * useful for downsampling effects, bloom, and improved texture filtering
     * when the texture is sampled at different scales.
     * 
     * The method automatically:
     * - Updates texture parameters to GL_LINEAR_MIPMAP_LINEAR for minification
     * - Generates the complete mipmap chain via glGenerateMipmap
     * 
     * @code
     * auto fbo = framebuffer::Framebuffer::MakeRenderToTexture(800, 600, "scene_color");
     * // ... render to FBO ...
     * fbo->generateMipmaps("scene_color");
     * // Texture now has mipmaps for improved filtering
     * @endcode
     * 
     * @note Mipmap generation is only valid for texture attachments, not renderbuffers.
     * @note The texture must have power-of-two dimensions for optimal mipmap generation.
     */
    void generateMipmaps(const std::string& textureName) {
        // Validate texture exists
        if (!hasTexture(textureName)) {
            throw exception::FramebufferException("Texture '" + textureName + "' not found in framebuffer");
        }
        
        auto texture = m_named_textures.at(textureName);
        
        // Update texture parameters to support mipmaps
        texture->setTextureParameters(
            GL_CLAMP_TO_EDGE, 
            GL_CLAMP_TO_EDGE,
            GL_LINEAR_MIPMAP_LINEAR,  // Enable mipmap filtering
            GL_LINEAR);
        
        // Generate mipmaps
        texture->generateMipmaps();
    }
    
    /**
     * @brief Attaches framebuffer textures to shader samplers.
     * @param shader The shader to attach textures to
     * @param textureToSamplerMap Map of texture names to sampler uniform names
     * @throws exception::FramebufferException if a texture name doesn't exist
     * 
     * Utility method that automatically configures dynamic uniforms for shader
     * samplers, mapping framebuffer texture attachments to sampler uniform names.
     * This eliminates the need to manually configure each sampler uniform.
     * 
     * The method:
     * - Validates that all texture names exist in the framebuffer
     * - Configures dynamic uniforms using texture::getSamplerProvider()
     * - Enables automatic texture unit resolution via TextureStack
     * 
     * @code
     * auto fbo = framebuffer::Framebuffer::MakeRenderToTexture(800, 600, "scene_color");
     * auto shader = shader::Make("post_process.vert", "blur.frag");
     * 
     * // Attach FBO texture to shader's u_scene_texture sampler
     * fbo->attachToShader(shader, {{"scene_color", "u_scene_texture"}});
     * 
     * // Later in render loop
     * shader::stack()->push(shader);
     * texture::stack()->registerSamplerUnit("u_scene_texture", 0);
     * texture::stack()->push(fbo->getTexture("scene_color"), 0);
     * // Shader automatically receives correct texture unit
     * @endcode
     * 
     * @note This method only configures the shader uniforms; you still need to
     *       push textures onto the TextureStack during rendering.
     * @note Multiple textures can be attached in a single call by providing
     *       multiple entries in the map.
     */
    void attachToShader(shader::ShaderPtr shader, 
                        const std::unordered_map<std::string, std::string>& textureToSamplerMap) {
        for (const auto& pair : textureToSamplerMap) {
            const std::string& textureName = pair.first;
            const std::string& samplerName = pair.second;
            
            // Validate texture exists
            if (!hasTexture(textureName)) {
                throw exception::FramebufferException(
                    "Texture '" + textureName + "' not found in framebuffer");
            }
            
            // Configure dynamic uniform for the sampler
            shader->configureDynamicUniform<uniform::detail::Sampler>(
                samplerName, 
                texture::getSamplerProvider(samplerName));
        }
    }
    
    /**
     * @brief Gets the framebuffer width.
     * @return Width in pixels
     * 
     * Returns the width specified during framebuffer creation. This matches
     * the dimensions of all attachments.
     * 
     * @code
     * auto fbo = framebuffer::Framebuffer::MakeRenderToTexture(800, 600, "color");
     * int width = fbo->getWidth();  // Returns 800
     * @endcode
     */
    int getWidth() const { return m_width; }
    
    /**
     * @brief Gets the framebuffer height.
     * @return Height in pixels
     * 
     * Returns the height specified during framebuffer creation. This matches
     * the dimensions of all attachments.
     * 
     * @code
     * auto fbo = framebuffer::Framebuffer::MakeRenderToTexture(800, 600, "color");
     * int height = fbo->getHeight();  // Returns 600
     * @endcode
     */
    int getHeight() const { return m_height; }
    
    /**
     * @brief Gets the OpenGL framebuffer ID.
     * @return The framebuffer object ID
     * 
     * Returns the underlying OpenGL framebuffer object ID. This is primarily
     * for internal use by FramebufferStack and debugging purposes.
     * 
     * @note Direct manipulation of the framebuffer via this ID is discouraged;
     *       use the provided methods instead.
     */
    GLuint getID() const { return m_fbo_id; }
    
    /**
     * @brief Sets whether the framebuffer should automatically clear on bind.
     * @param clear_on_bind If true, framebuffer will clear when bound (default: true)
     * 
     * Controls whether the framebuffer automatically clears its attachments
     * when it is bound via the FramebufferStack. This is useful for:
     * - Disabling automatic clearing when accumulating multiple render passes
     * - Enabling automatic clearing for single-pass rendering (default behavior)
     * 
     * @code
     * auto fbo = framebuffer::Framebuffer::MakeRenderToTexture(800, 600, "scene");
     * fbo->setClearOnBind(false);  // Disable automatic clearing
     * 
     * framebuffer::stack()->push(fbo);
     * // Manual clearing if needed
     * glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
     * // ... render ...
     * framebuffer::stack()->pop();
     * @endcode
     * 
     * @note The clearing behavior is determined by the attachment types:
     *       - Color attachments: GL_COLOR_BUFFER_BIT
     *       - Depth attachments: GL_DEPTH_BUFFER_BIT
     *       - Stencil attachments: GL_STENCIL_BUFFER_BIT
     */
    void setClearOnBind(bool clear_on_bind) {
        m_clear_on_bind = clear_on_bind;
    }
    
    /**
     * @brief Gets whether the framebuffer automatically clears on bind.
     * @return True if framebuffer clears on bind, false otherwise
     * 
     * Returns the current clear-on-bind setting for this framebuffer.
     * 
     * @code
     * auto fbo = framebuffer::Framebuffer::MakeRenderToTexture(800, 600, "scene");
     * bool clears = fbo->getClearOnBind();  // Returns true (default)
     * @endcode
     */
    bool getClearOnBind() const {
        return m_clear_on_bind;
    }
};

/**
 * @class FramebufferStack
 * @brief Manages hierarchical framebuffer state with automatic restoration
 * 
 * The FramebufferStack provides stack-based framebuffer management, similar to
 * ShaderStack, TransformStack, and MaterialStack. It tracks the currently bound
 * framebuffer, viewport dimensions, and draw buffer configuration, enabling
 * efficient state restoration and minimizing redundant OpenGL calls.
 * 
 * Key features:
 * - Hierarchical state management (push/pop operations)
 * - GPU state tracking to prevent redundant glBindFramebuffer calls
 * - Viewport optimization (only updates when dimensions change)
 * - Automatic window dimension query when restoring default framebuffer
 * - Protected base state (cannot pop below default framebuffer)
 * 
 * @note Access via singleton: framebuffer::stack()
 * @note The base level represents the default framebuffer (window)
 */
class FramebufferStack {
private:
    /**
     * @struct FramebufferState
     * @brief Complete framebuffer state at a single stack level
     * 
     * Stores all state necessary to restore a framebuffer configuration,
     * including the framebuffer pointer, viewport dimensions, and draw
     * buffer configuration.
     */
    struct FramebufferState {
        FramebufferPtr fbo;          ///< Framebuffer pointer (nullptr = default framebuffer)
        int viewport_width;          ///< Viewport width in pixels
        int viewport_height;         ///< Viewport height in pixels
        std::vector<GLenum> draw_buffers; ///< Draw buffer configuration for glDrawBuffers
    };
    
    std::vector<FramebufferState> m_stack;
    
    // State tracking for optimization (prevents redundant OpenGL calls)
    GLuint m_currently_bound_fbo;
    int m_current_viewport_width;
    int m_current_viewport_height;
    
    /**
     * @brief Private constructor for singleton pattern.
     * 
     * Initializes the stack with a base state representing the default
     * framebuffer (window). The base state cannot be popped.
     */
    FramebufferStack() 
        : m_currently_bound_fbo(0),
          m_current_viewport_width(0),
          m_current_viewport_height(0) {
        
        // Initialize with base state (default framebuffer)
        FramebufferState base_state;
        base_state.fbo = nullptr;  // nullptr represents default framebuffer
        base_state.viewport_width = 0;  // Will be queried from GLFW when needed
        base_state.viewport_height = 0;
        base_state.draw_buffers = { GL_BACK };  // Default draw buffer
        
        m_stack.push_back(base_state);
    }
    
    friend FramebufferStackPtr stack();
    
public:
    // Delete copy constructor and assignment operator (singleton)
    FramebufferStack(const FramebufferStack&) = delete;
    FramebufferStack& operator=(const FramebufferStack&) = delete;
    
    /**
     * @brief Pushes a framebuffer onto the stack and activates it.
     * @param fbo The framebuffer to push (must not be nullptr)
     * 
     * Stores the complete framebuffer state and activates the framebuffer
     * by binding it, setting the viewport, and configuring draw buffers.
     * 
     * Optimizations:
     * - Only calls glBindFramebuffer if FBO ID changed
     * - Only calls glViewport if dimensions changed
     * - Handles depth-only FBOs (GL_NONE for draw buffers)
     * 
     * @note This method has side effects (modifies OpenGL state)
     */
    void push(FramebufferPtr fbo);
    
    /**
     * @brief Pops the current framebuffer and restores the previous state.
     * 
     * Restores the previous framebuffer, viewport, and draw buffer configuration.
     * If restoring the default framebuffer, queries GLFW for current window
     * dimensions to handle window resizing.
     * 
     * @note Cannot pop the base state (default framebuffer)
     * @note Logs a warning if attempting to pop base state
     */
    void pop();
    
    /**
     * @brief Returns the framebuffer at the top of the stack.
     * @return Shared pointer to the current framebuffer (nullptr if default framebuffer)
     */
    FramebufferPtr top() const {
        return m_stack.back().fbo;
    }
    
    /**
     * @brief Checks if the default framebuffer is currently active.
     * @return True if the default framebuffer (window) is active, false otherwise
     */
    bool isDefaultFramebuffer() const {
        return m_stack.back().fbo == nullptr;
    }
    
    /**
     * @brief Gets the current viewport width.
     * @return Current viewport width in pixels
     */
    int getCurrentWidth() const {
        return m_current_viewport_width;
    }
    
    /**
     * @brief Gets the current viewport height.
     * @return Current viewport height in pixels
     */
    int getCurrentHeight() const {
        return m_current_viewport_height;
    }
};

// FramebufferStack method implementations

inline void FramebufferStack::push(FramebufferPtr fbo) {
    if (!fbo) {
        std::cerr << "Warning: Attempting to push null framebuffer to stack." << std::endl;
        return;
    }
    
    // Store current state
    FramebufferState state;
    state.fbo = fbo;
    state.viewport_width = fbo->getWidth();
    state.viewport_height = fbo->getHeight();
    state.draw_buffers = fbo->m_color_attachments;  // Access via friend
    
    m_stack.push_back(state);
    
    // Bind FBO (with optimization - only bind if changed)
    if (m_currently_bound_fbo != fbo->getID()) {
        fbo->bind();
        m_currently_bound_fbo = fbo->getID();
    }
    
    // Set viewport (with optimization - only update if dimensions changed)
    if (m_current_viewport_width != state.viewport_width || 
        m_current_viewport_height != state.viewport_height) {
        glViewport(0, 0, state.viewport_width, state.viewport_height);
        GL_CHECK("set framebuffer viewport");
        m_current_viewport_width = state.viewport_width;
        m_current_viewport_height = state.viewport_height;
    }
    
    // Configure draw buffers
    if (state.draw_buffers.empty()) {
        // Depth-only FBO (e.g., shadow map)
        glDrawBuffer(GL_NONE);
        GL_CHECK("set draw buffer to none");
    } else {
        // Color attachments present
        glDrawBuffers(static_cast<GLsizei>(state.draw_buffers.size()), state.draw_buffers.data());
        GL_CHECK("set draw buffers");
    }
}

inline void FramebufferStack::pop() {
    // Prevent popping base state
    if (m_stack.size() <= 1) {
        std::cerr << "Warning: Attempt to pop base framebuffer state." << std::endl;
        return;
    }
    
    m_stack.pop_back();
    const FramebufferState& state_to_restore = m_stack.back();
    
    // Restore framebuffer
    if (state_to_restore.fbo == nullptr) {
        // Restore default framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        GL_CHECK("restore default framebuffer");
        m_currently_bound_fbo = 0;
        
        // Query window size from GLFW
        GLFWwindow* window = glfwGetCurrentContext();
        int window_width, window_height;
        glfwGetFramebufferSize(window, &window_width, &window_height);
        
        glViewport(0, 0, window_width, window_height);
        GL_CHECK("restore window viewport");
        m_current_viewport_width = window_width;
        m_current_viewport_height = window_height;
        
        // Restore default draw buffer
        // For default framebuffer, use glDrawBuffer (singular) with GL_BACK
        glDrawBuffer(GL_BACK);
        GL_CHECK("restore default draw buffer");
    } else {
        // Restore previous FBO
        if (m_currently_bound_fbo != state_to_restore.fbo->getID()) {
            state_to_restore.fbo->bind();
            m_currently_bound_fbo = state_to_restore.fbo->getID();
        }
        
        if (m_current_viewport_width != state_to_restore.viewport_width ||
            m_current_viewport_height != state_to_restore.viewport_height) {
            glViewport(0, 0, state_to_restore.viewport_width, state_to_restore.viewport_height);
            GL_CHECK("restore framebuffer viewport");
            m_current_viewport_width = state_to_restore.viewport_width;
            m_current_viewport_height = state_to_restore.viewport_height;
        }
        
        if (state_to_restore.draw_buffers.empty()) {
            glDrawBuffer(GL_NONE);
            GL_CHECK("restore draw buffer to none");
        } else {
            glDrawBuffers(static_cast<GLsizei>(state_to_restore.draw_buffers.size()), 
                         state_to_restore.draw_buffers.data());
            GL_CHECK("restore draw buffers");
        }
    }
}

/**
 * @brief Singleton accessor for the FramebufferStack.
 * @return Shared pointer to the global FramebufferStack instance
 * 
 * Uses Meyers singleton pattern for thread-safe initialization.
 * The stack is initialized with a base state representing the default
 * framebuffer (window).
 * 
 * @code
 * auto fbo = framebuffer::Framebuffer::MakeRenderToTexture(800, 600, "scene");
 * framebuffer::stack()->push(fbo);
 * // ... render to FBO ...
 * framebuffer::stack()->pop();
 * @endcode
 */
inline FramebufferStackPtr stack() {
    static FramebufferStackPtr instance = FramebufferStackPtr(new FramebufferStack());
    return instance;
}

} // namespace framebuffer

#endif // FRAMEBUFFER_H
