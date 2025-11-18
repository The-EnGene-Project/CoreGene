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
 * Provides RAII-compliant abstraction for OpenGL Framebuffer Objects,
 * enabling post-processing, shadow mapping, reflections, and deferred shading.
 * 
 * See README.md for detailed usage examples and integration patterns.
 */

namespace framebuffer {

// Forward declarations
class Framebuffer;
class FramebufferStack;

using FramebufferPtr = std::shared_ptr<Framebuffer>;
using FramebufferStackPtr = std::shared_ptr<FramebufferStack>;

/**
 * @namespace framebuffer::attachment
 * @brief Type-safe enums for framebuffer attachment configuration
 */
namespace attachment {

/**
 * @enum Point
 * @brief Framebuffer attachment points (up to 8 color + depth/stencil)
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
 * @brief Internal formats for framebuffer attachments (color, depth, stencil)
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
 * @brief Texture (readable) or Renderbuffer (write-only, optimized)
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
 * @enum StencilFunc
 * @brief Stencil test comparison functions
 */
enum class StencilFunc : GLenum {
    Never = GL_NEVER,           ///< Test never passes
    Less = GL_LESS,             ///< Test passes if (ref & mask) < (stencil & mask)
    LEqual = GL_LEQUAL,         ///< Test passes if (ref & mask) <= (stencil & mask)
    Greater = GL_GREATER,       ///< Test passes if (ref & mask) > (stencil & mask)
    GEqual = GL_GEQUAL,         ///< Test passes if (ref & mask) >= (stencil & mask)
    Equal = GL_EQUAL,           ///< Test passes if (ref & mask) == (stencil & mask)
    NotEqual = GL_NOTEQUAL,     ///< Test passes if (ref & mask) != (stencil & mask)
    Always = GL_ALWAYS          ///< Test always passes
};

/**
 * @enum DepthFunc
 * @brief Depth test comparison functions
 */
enum class DepthFunc : GLenum {
    Never = GL_NEVER,           ///< Test never passes
    Less = GL_LESS,             ///< Test passes if incoming < stored (default)
    LEqual = GL_LEQUAL,         ///< Test passes if incoming <= stored
    Greater = GL_GREATER,       ///< Test passes if incoming > stored
    GEqual = GL_GEQUAL,         ///< Test passes if incoming >= stored
    Equal = GL_EQUAL,           ///< Test passes if incoming == stored
    NotEqual = GL_NOTEQUAL,     ///< Test passes if incoming != stored
    Always = GL_ALWAYS          ///< Test always passes
};

/**
 * @enum StencilOp
 * @brief Stencil buffer update operations
 */
enum class StencilOp : GLenum {
    Keep = GL_KEEP,                 ///< Keep current stencil value
    Zero = GL_ZERO,                 ///< Set stencil value to 0
    Replace = GL_REPLACE,           ///< Replace stencil value with reference value
    Increment = GL_INCR,            ///< Increment stencil value (clamp to max)
    IncrementWrap = GL_INCR_WRAP,   ///< Increment stencil value (wrap to 0)
    Decrement = GL_DECR,            ///< Decrement stencil value (clamp to 0)
    DecrementWrap = GL_DECR_WRAP,   ///< Decrement stencil value (wrap to max)
    Invert = GL_INVERT              ///< Bitwise invert stencil value
};

/**
 * @enum BlendFactor
 * @brief Blend factor values for source and destination
 */
enum class BlendFactor : GLenum {
    Zero = GL_ZERO,                             ///< Factor is (0, 0, 0, 0)
    One = GL_ONE,                               ///< Factor is (1, 1, 1, 1)
    SrcColor = GL_SRC_COLOR,                    ///< Factor is (Rs, Gs, Bs, As)
    OneMinusSrcColor = GL_ONE_MINUS_SRC_COLOR,  ///< Factor is (1-Rs, 1-Gs, 1-Bs, 1-As)
    DstColor = GL_DST_COLOR,                    ///< Factor is (Rd, Gd, Bd, Ad)
    OneMinusDstColor = GL_ONE_MINUS_DST_COLOR,  ///< Factor is (1-Rd, 1-Gd, 1-Bd, 1-Ad)
    SrcAlpha = GL_SRC_ALPHA,                    ///< Factor is (As, As, As, As)
    OneMinusSrcAlpha = GL_ONE_MINUS_SRC_ALPHA,  ///< Factor is (1-As, 1-As, 1-As, 1-As)
    DstAlpha = GL_DST_ALPHA,                    ///< Factor is (Ad, Ad, Ad, Ad)
    OneMinusDstAlpha = GL_ONE_MINUS_DST_ALPHA,  ///< Factor is (1-Ad, 1-Ad, 1-Ad, 1-Ad)
    ConstantColor = GL_CONSTANT_COLOR,          ///< Factor is (Rc, Gc, Bc, Ac)
    OneMinusConstantColor = GL_ONE_MINUS_CONSTANT_COLOR, ///< Factor is (1-Rc, 1-Gc, 1-Bc, 1-Ac)
    ConstantAlpha = GL_CONSTANT_ALPHA,          ///< Factor is (Ac, Ac, Ac, Ac)
    OneMinusConstantAlpha = GL_ONE_MINUS_CONSTANT_ALPHA, ///< Factor is (1-Ac, 1-Ac, 1-Ac, 1-Ac)
    SrcAlphaSaturate = GL_SRC_ALPHA_SATURATE    ///< Factor is (f, f, f, 1) where f = min(As, 1-Ad)
};

/**
 * @enum BlendEquation
 * @brief Blend equation modes for combining source and destination
 */
enum class BlendEquation : GLenum {
    Add = GL_FUNC_ADD,                      ///< Result = src * srcFactor + dst * dstFactor
    Subtract = GL_FUNC_SUBTRACT,            ///< Result = src * srcFactor - dst * dstFactor
    ReverseSubtract = GL_FUNC_REVERSE_SUBTRACT, ///< Result = dst * dstFactor - src * srcFactor
    Min = GL_MIN,                           ///< Result = min(src, dst)
    Max = GL_MAX                            ///< Result = max(src, dst)
};

/**
 * @struct StencilState
 * @brief Complete stencil test state with OpenGL default values
 * 
 * Stores all stencil test parameters including enable flag, comparison function,
 * reference value, masks, and stencil operations. All fields are initialized to
 * OpenGL default values to ensure consistency with GPU state on startup.
 */
struct StencilState {
    bool enabled = false;                       ///< Stencil test enabled (default: false)
    StencilFunc func = StencilFunc::Always;     ///< Stencil comparison function (default: Always)
    GLint ref = 0;                              ///< Reference value for stencil test (default: 0)
    GLuint func_mask = 0xFFFFFFFF;              ///< Mask for stencil comparison (default: all bits)
    GLuint write_mask = 0xFFFFFFFF;             ///< Mask for stencil buffer writes (default: all bits)
    StencilOp sfail = StencilOp::Keep;          ///< Action when stencil test fails (default: Keep)
    StencilOp dpfail = StencilOp::Keep;         ///< Action when stencil passes but depth fails (default: Keep)
    StencilOp dppass = StencilOp::Keep;         ///< Action when both stencil and depth pass (default: Keep)
};

/**
 * @struct BlendState
 * @brief Complete blend state with OpenGL default values
 * 
 * Stores all blending parameters including enable flag, blend equations for RGB and Alpha,
 * blend factors for source and destination, and constant blend color. All fields are
 * initialized to OpenGL default values to ensure consistency with GPU state on startup.
 */
struct BlendState {
    bool enabled = false;                           ///< Blending enabled (default: false)
    BlendEquation equationRGB = BlendEquation::Add; ///< RGB blend equation (default: Add)
    BlendEquation equationAlpha = BlendEquation::Add; ///< Alpha blend equation (default: Add)
    BlendFactor sfactorRGB = BlendFactor::One;      ///< Source RGB blend factor (default: One)
    BlendFactor dfactorRGB = BlendFactor::Zero;     ///< Destination RGB blend factor (default: Zero)
    BlendFactor sfactorAlpha = BlendFactor::One;    ///< Source Alpha blend factor (default: One)
    BlendFactor dfactorAlpha = BlendFactor::Zero;   ///< Destination Alpha blend factor (default: Zero)
    float constant_color_r = 0.0f;                  ///< Constant blend color red component (default: 0.0)
    float constant_color_g = 0.0f;                  ///< Constant blend color green component (default: 0.0)
    float constant_color_b = 0.0f;                  ///< Constant blend color blue component (default: 0.0)
    float constant_color_a = 0.0f;                  ///< Constant blend color alpha component (default: 0.0)
};

/**
 * @struct DepthState
 * @brief Complete depth test state with OpenGL default values
 * 
 * Stores all depth testing parameters including enable flag, write mask, comparison function,
 * depth clamping, and depth range. All fields are initialized to OpenGL default values to
 * ensure consistency with GPU state on startup.
 */
struct DepthState {
    bool test_enabled = false;          ///< Depth test enabled (default: false)
    bool write_enabled = true;          ///< Depth buffer writes enabled (default: true)
    DepthFunc func = DepthFunc::Less;   ///< Depth comparison function (default: Less)
    bool clamp_enabled = false;         ///< Depth clamping enabled (default: false)
    double range_near = 0.0;            ///< Near depth range (default: 0.0)
    double range_far = 1.0;             ///< Far depth range (default: 1.0)
};

/**
 * @class Framebuffer
 * @brief RAII-compliant OpenGL Framebuffer Object abstraction
 * 
 * Supports off-screen rendering with named texture access, type-safe configuration,
 * and factory methods for common use cases. See README.md for usage examples.
 * 
 * @note Texture attachments use user-defined names for retrieval.
 * @note Move-only (copying disabled).
 */
class Framebuffer {
public:
    /**
     * @struct AttachmentSpec
     * @brief Configuration for a single framebuffer attachment
     * 
     * @note Only texture attachments need names (renderbuffers not retrievable).
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
         * @brief Constructor for attachment specifications
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
     * @brief Factory method for custom framebuffer configuration
     * @throws exception::FramebufferException if creation fails
     */
    static FramebufferPtr Make(int width, int height, 
                                const std::vector<AttachmentSpec>& specs) {
        return FramebufferPtr(new Framebuffer(width, height, specs));
    }
    
    /**
     * @brief Factory for render-to-texture (color texture + depth renderbuffer)
     * @throws exception::FramebufferException if creation fails
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
     * @brief Factory for post-processing (color texture + depth renderbuffer)
     * @throws exception::FramebufferException if creation fails
     * @note Use custom Make() if you need readable depth texture
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
     * @brief Factory for shadow mapping (depth texture only, no color)
     * @throws exception::FramebufferException if creation fails
     * @note Depth texture has shadow comparison mode enabled
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
     * @brief Factory for G-Buffer (multiple color textures + depth renderbuffer)
     * @throws exception::FramebufferException if creation fails
     * @note Maximum 8 color attachments (OpenGL 4.3 guarantee)
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
     * @brief Binds framebuffer (internal use by FramebufferStack)
     * @note Use framebuffer::stack()->push() instead
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
     * @brief Unbinds framebuffer (internal use by FramebufferStack)
     * @note Use framebuffer::stack()->pop() instead
     */
    void unbind() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        GL_CHECK("unbind framebuffer");
    }
    
    /**
     * @brief Retrieves texture attachment by name
     * @throws exception::FramebufferException if not found
     * @note Only texture attachments retrievable (not renderbuffers)
     */
    texture::TexturePtr getTexture(const std::string& name) const {
        auto it = m_named_textures.find(name);
        if (it == m_named_textures.end()) {
            throw exception::FramebufferException("Texture '" + name + "' not found in framebuffer");
        }
        return it->second;
    }
    
    /**
     * @brief Checks if named texture exists
     */
    bool hasTexture(const std::string& name) const {
        return m_named_textures.find(name) != m_named_textures.end();
    }
    
    /**
     * @brief Generates mipmaps for texture attachment
     * @throws exception::FramebufferException if texture doesn't exist
     * @note Only for texture attachments (not renderbuffers)
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
     * @brief Configures shader samplers for FBO textures
     * @throws exception::FramebufferException if texture name doesn't exist
     * @note Still need to push textures to TextureStack during rendering
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
    
    /** @brief Gets framebuffer width in pixels */
    int getWidth() const { return m_width; }
    
    /** @brief Gets framebuffer height in pixels */
    int getHeight() const { return m_height; }
    
    /** @brief Gets OpenGL framebuffer ID (for internal use) */
    GLuint getID() const { return m_fbo_id; }
    
    /**
     * @brief Sets automatic clear on bind (default: true)
     * @note Clears color/depth/stencil based on attachments
     */
    void setClearOnBind(bool clear_on_bind) {
        m_clear_on_bind = clear_on_bind;
    }
    
    /** @brief Gets clear-on-bind setting */
    bool getClearOnBind() const {
        return m_clear_on_bind;
    }
};

/**
 * @class RenderState
 * @brief Offline configuration for stencil, blend, and depth state
 * 
 * Provides a way to pre-configure stencil, blend, and depth state without issuing OpenGL calls.
 * State can be built offline and then applied atomically when pushing a framebuffer.
 * 
 * @note Access state members via friend declaration (FramebufferStack)
 * @note Use stencil(), blend(), and depth() methods to configure state offline
 */
class RenderState {
private:
    StencilState m_stencil_state;  ///< Stencil test state (CPU-side only)
    BlendState m_blend_state;      ///< Blend state (CPU-side only)
    DepthState m_depth_state;      ///< Depth test state (CPU-side only)
    
    // Friend declaration for FramebufferStack to access private state
    friend class FramebufferStack;
    
public:
    /**
     * @class StencilManager
     * @brief Offline stencil state configuration (no OpenGL calls)
     * 
     * Provides methods to configure stencil testing without issuing OpenGL calls.
     * All methods modify CPU-side state only. State is applied when RenderState
     * is used with FramebufferStack::push().
     */
    class StencilManager {
    private:
        RenderState* m_owner;  ///< Pointer to owning RenderState
        
    public:
        /**
         * @brief Constructs StencilManager with owner pointer
         * @param owner Pointer to the RenderState instance
         */
        explicit StencilManager(RenderState* owner) : m_owner(owner) {}
        
        /**
         * @brief Enables or disables stencil testing (offline)
         * @param enabled True to enable stencil test, false to disable
         * @note No OpenGL calls - modifies CPU-side state only
         */
        void setTest(bool enabled) {
            m_owner->m_stencil_state.enabled = enabled;
        }
        
        /**
         * @brief Sets stencil write mask (offline)
         * @param mask Bit mask controlling which bits are written to stencil buffer
         * @note No OpenGL calls - modifies CPU-side state only
         */
        void setWriteMask(GLuint mask) {
            m_owner->m_stencil_state.write_mask = mask;
        }
        
        /**
         * @brief Sets stencil test function and parameters (offline)
         * @param func Stencil comparison function
         * @param ref Reference value for stencil test
         * @param mask Bit mask for stencil comparison
         * @note No OpenGL calls - modifies CPU-side state only
         */
        void setFunction(StencilFunc func, GLint ref, GLuint mask) {
            m_owner->m_stencil_state.func = func;
            m_owner->m_stencil_state.ref = ref;
            m_owner->m_stencil_state.func_mask = mask;
        }
        
        /**
         * @brief Sets stencil buffer update operations (offline)
         * @param sfail Action when stencil test fails
         * @param dpfail Action when stencil passes but depth test fails
         * @param dppass Action when both stencil and depth tests pass
         * @note No OpenGL calls - modifies CPU-side state only
         */
        void setOperation(StencilOp sfail, StencilOp dpfail, StencilOp dppass) {
            m_owner->m_stencil_state.sfail = sfail;
            m_owner->m_stencil_state.dpfail = dpfail;
            m_owner->m_stencil_state.dppass = dppass;
        }
        
        // Note: setClearValue and clearBuffer are NOT provided
        // (they are immediate operations that don't make sense offline)
    };
    
    /**
     * @class BlendManager
     * @brief Offline blend state configuration (no OpenGL calls)
     * 
     * Provides methods to configure blending without issuing OpenGL calls.
     * All methods modify CPU-side state only. State is applied when RenderState
     * is used with FramebufferStack::push().
     */
    class BlendManager {
    private:
        RenderState* m_owner;  ///< Pointer to owning RenderState
        
    public:
        /**
         * @brief Constructs BlendManager with owner pointer
         * @param owner Pointer to the RenderState instance
         */
        explicit BlendManager(RenderState* owner) : m_owner(owner) {}
        
        /**
         * @brief Enables or disables blending (offline)
         * @param enabled True to enable blending, false to disable
         * @note No OpenGL calls - modifies CPU-side state only
         */
        void setEnabled(bool enabled) {
            m_owner->m_blend_state.enabled = enabled;
        }
        
        /**
         * @brief Sets blend equation for both RGB and Alpha (offline)
         * @param mode Blend equation to use for both RGB and Alpha channels
         * @note No OpenGL calls - modifies CPU-side state only
         */
        void setEquation(BlendEquation mode) {
            m_owner->m_blend_state.equationRGB = mode;
            m_owner->m_blend_state.equationAlpha = mode;
        }
        
        /**
         * @brief Sets blend equation separately for RGB and Alpha (offline)
         * @param modeRGB Blend equation for RGB channels
         * @param modeAlpha Blend equation for Alpha channel
         * @note No OpenGL calls - modifies CPU-side state only
         */
        void setEquationSeparate(BlendEquation modeRGB, BlendEquation modeAlpha) {
            m_owner->m_blend_state.equationRGB = modeRGB;
            m_owner->m_blend_state.equationAlpha = modeAlpha;
        }
        
        /**
         * @brief Sets blend function for both RGB and Alpha (offline)
         * @param sfactor Source blend factor for both RGB and Alpha
         * @param dfactor Destination blend factor for both RGB and Alpha
         * @note No OpenGL calls - modifies CPU-side state only
         */
        void setFunction(BlendFactor sfactor, BlendFactor dfactor) {
            m_owner->m_blend_state.sfactorRGB = sfactor;
            m_owner->m_blend_state.dfactorRGB = dfactor;
            m_owner->m_blend_state.sfactorAlpha = sfactor;
            m_owner->m_blend_state.dfactorAlpha = dfactor;
        }
        
        /**
         * @brief Sets blend function separately for RGB and Alpha (offline)
         * @param srcRGB Source blend factor for RGB channels
         * @param dstRGB Destination blend factor for RGB channels
         * @param srcAlpha Source blend factor for Alpha channel
         * @param dstAlpha Destination blend factor for Alpha channel
         * @note No OpenGL calls - modifies CPU-side state only
         */
        void setFunctionSeparate(BlendFactor srcRGB, BlendFactor dstRGB,
                                 BlendFactor srcAlpha, BlendFactor dstAlpha) {
            m_owner->m_blend_state.sfactorRGB = srcRGB;
            m_owner->m_blend_state.dfactorRGB = dstRGB;
            m_owner->m_blend_state.sfactorAlpha = srcAlpha;
            m_owner->m_blend_state.dfactorAlpha = dstAlpha;
        }
        
        /**
         * @brief Sets blend constant color (offline)
         * @param r Red component (0.0 to 1.0)
         * @param g Green component (0.0 to 1.0)
         * @param b Blue component (0.0 to 1.0)
         * @param a Alpha component (0.0 to 1.0)
         * @note No OpenGL calls - modifies CPU-side state only
         */
        void setConstantColor(float r, float g, float b, float a) {
            m_owner->m_blend_state.constant_color_r = r;
            m_owner->m_blend_state.constant_color_g = g;
            m_owner->m_blend_state.constant_color_b = b;
            m_owner->m_blend_state.constant_color_a = a;
        }
    };
    
    /**
     * @class DepthManager
     * @brief Offline depth state configuration (no OpenGL calls)
     * 
     * Provides methods to configure depth testing without issuing OpenGL calls.
     * All methods modify CPU-side state only. State is applied when RenderState
     * is used with FramebufferStack::push().
     */
    class DepthManager {
    private:
        RenderState* m_owner;  ///< Pointer to owning RenderState
        
    public:
        /**
         * @brief Constructs DepthManager with owner pointer
         * @param owner Pointer to the RenderState instance
         */
        explicit DepthManager(RenderState* owner) : m_owner(owner) {}
        
        /**
         * @brief Enables or disables depth testing (offline)
         * @param enabled True to enable depth test, false to disable
         * @note No OpenGL calls - modifies CPU-side state only
         */
        void setTest(bool enabled) {
            m_owner->m_depth_state.test_enabled = enabled;
        }
        
        /**
         * @brief Enables or disables depth buffer writes (offline)
         * @param enabled True to enable depth writes, false to disable
         * @note No OpenGL calls - modifies CPU-side state only
         */
        void setWrite(bool enabled) {
            m_owner->m_depth_state.write_enabled = enabled;
        }
        
        /**
         * @brief Sets depth test comparison function (offline)
         * @param func Depth comparison function
         * @note No OpenGL calls - modifies CPU-side state only
         */
        void setFunction(DepthFunc func) {
            m_owner->m_depth_state.func = func;
        }
        
        /**
         * @brief Enables or disables depth clamping (offline)
         * @param enabled True to enable depth clamping, false to disable
         * @note No OpenGL calls - modifies CPU-side state only
         */
        void setClamp(bool enabled) {
            m_owner->m_depth_state.clamp_enabled = enabled;
        }
        
        /**
         * @brief Sets depth range (offline)
         * @param near_val Near depth range value (default: 0.0)
         * @param far_val Far depth range value (default: 1.0)
         * @note No OpenGL calls - modifies CPU-side state only
         */
        void setRange(double near_val, double far_val) {
            m_owner->m_depth_state.range_near = near_val;
            m_owner->m_depth_state.range_far = far_val;
        }
    };
    
    /**
     * @brief Default constructor - initializes state to OpenGL defaults
     */
    RenderState() = default;
    
    /**
     * @brief Returns StencilManager instance for configuring stencil state offline
     * @return StencilManager proxy object
     */
    StencilManager stencil() {
        return StencilManager(this);
    }
    
    /**
     * @brief Returns BlendManager instance for configuring blend state offline
     * @return BlendManager proxy object
     */
    BlendManager blend() {
        return BlendManager(this);
    }
    
    /**
     * @brief Returns DepthManager instance for configuring depth state offline
     * @return DepthManager proxy object
     */
    DepthManager depth() {
        return DepthManager(this);
    }
};

/**
 * @brief Type alias for shared pointer to RenderState
 */
using RenderStatePtr = std::shared_ptr<RenderState>;

/**
 * @class FramebufferStack
 * @brief Stack-based framebuffer management with GPU state tracking
 * 
 * Manages framebuffer binding, viewport, and draw buffers hierarchically.
 * Optimizes by tracking GPU state to prevent redundant calls.
 * 
 * @note Access via framebuffer::stack()
 * @note Base level is default framebuffer (cannot pop)
 */
class FramebufferStack {
private:
    /**
     * @struct FramebufferState
     * @brief Complete framebuffer state (FBO, viewport, draw buffers, stencil, blend, depth)
     */
    struct FramebufferState {
        FramebufferPtr fbo;          ///< Framebuffer pointer (nullptr = default framebuffer)
        int viewport_width;          ///< Viewport width in pixels
        int viewport_height;         ///< Viewport height in pixels
        std::vector<GLenum> draw_buffers; ///< Draw buffer configuration for glDrawBuffers
        
        // State management (NEW)
        StencilState stencil_state;  ///< Stencil test state
        BlendState blend_state;      ///< Blend state
        DepthState depth_state;      ///< Depth test state
    };
    
    std::vector<FramebufferState> m_stack;
    
    // State tracking for optimization (prevents redundant OpenGL calls)
    GLuint m_currently_bound_fbo;
    int m_current_viewport_width;
    int m_current_viewport_height;
    
    // GPU state caching for stencil, blend, and depth (NEW)
    StencilState m_gpu_stencil_state;  ///< Cached GPU stencil state
    BlendState m_gpu_blend_state;      ///< Cached GPU blend state
    DepthState m_gpu_depth_state;      ///< Cached GPU depth state
    
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
        
        // Initialize stencil, blend, and depth state to OpenGL defaults
        base_state.stencil_state = StencilState();
        base_state.blend_state = BlendState();
        base_state.depth_state = DepthState();
        
        m_stack.push_back(base_state);
        
        // Initialize GPU state cache to OpenGL defaults
        m_gpu_stencil_state = StencilState();
        m_gpu_blend_state = BlendState();
        m_gpu_depth_state = DepthState();
    }
    
    /**
     * @brief Synchronizes GPU state to match target framebuffer state
     * @param target_state The desired state to synchronize to
     * 
     * Compares all fields in target_state with cached GPU state and issues
     * OpenGL calls only for fields that differ. Updates GPU state cache after
     * each successful state change.
     */
    void syncGpuToState(const FramebufferState& target_state);
    
    friend FramebufferStackPtr stack();
    
public:
    /**
     * @class StencilManager
     * @brief Proxy class for configuring stencil test state
     * 
     * Provides methods to configure stencil testing with automatic GPU state caching
     * to prevent redundant OpenGL calls. All methods update the logical state at the
     * top of the stack and compare with cached GPU state before issuing OpenGL calls.
     */
    class StencilManager {
    private:
        FramebufferStack* m_owner;  ///< Pointer to owning FramebufferStack
        
    public:
        /**
         * @brief Constructs StencilManager with owner pointer
         * @param owner Pointer to the FramebufferStack instance
         */
        explicit StencilManager(FramebufferStack* owner) : m_owner(owner) {}
        
        /**
         * @brief Enables or disables stencil testing
         * @param enabled True to enable stencil test, false to disable
         */
        void setTest(bool enabled) {
            // 1. Get logical state (top of stack)
            StencilState& logical_state = m_owner->m_stack.back().stencil_state;
            
            // 2. Update logical state
            logical_state.enabled = enabled;
            
            // 3. Get GPU state cache
            StencilState& gpu_state = m_owner->m_gpu_stencil_state;
            
            // 4. Compare and update if different
            if (logical_state.enabled != gpu_state.enabled) {
                if (enabled) {
                    glEnable(GL_STENCIL_TEST);
                } else {
                    glDisable(GL_STENCIL_TEST);
                }
                GL_CHECK("set stencil test");
                gpu_state.enabled = logical_state.enabled;
            }
        }
        
        /**
         * @brief Sets stencil write mask
         * @param mask Bit mask controlling which bits are written to stencil buffer
         */
        void setWriteMask(GLuint mask) {
            // 1. Get logical state (top of stack)
            StencilState& logical_state = m_owner->m_stack.back().stencil_state;
            
            // 2. Update logical state
            logical_state.write_mask = mask;
            
            // 3. Get GPU state cache
            StencilState& gpu_state = m_owner->m_gpu_stencil_state;
            
            // 4. Compare and update if different
            if (logical_state.write_mask != gpu_state.write_mask) {
                glStencilMask(mask);
                GL_CHECK("set stencil write mask");
                gpu_state.write_mask = logical_state.write_mask;
            }
        }
        
        /**
         * @brief Sets stencil test function and parameters
         * @param func Stencil comparison function
         * @param ref Reference value for stencil test
         * @param mask Bit mask for stencil comparison
         */
        void setFunction(StencilFunc func, GLint ref, GLuint mask) {
            // 1. Get logical state (top of stack)
            StencilState& logical_state = m_owner->m_stack.back().stencil_state;
            
            // 2. Update logical state
            logical_state.func = func;
            logical_state.ref = ref;
            logical_state.func_mask = mask;
            
            // 3. Get GPU state cache
            StencilState& gpu_state = m_owner->m_gpu_stencil_state;
            
            // 4. Compare and update if any parameter differs
            if (logical_state.func != gpu_state.func ||
                logical_state.ref != gpu_state.ref ||
                logical_state.func_mask != gpu_state.func_mask) {
                
                glStencilFunc(
                    static_cast<GLenum>(func),
                    ref,
                    mask);
                GL_CHECK("set stencil function");
                
                gpu_state.func = logical_state.func;
                gpu_state.ref = logical_state.ref;
                gpu_state.func_mask = logical_state.func_mask;
            }
        }
        
        /**
         * @brief Sets stencil buffer update operations
         * @param sfail Action when stencil test fails
         * @param dpfail Action when stencil passes but depth test fails
         * @param dppass Action when both stencil and depth tests pass
         */
        void setOperation(StencilOp sfail, StencilOp dpfail, StencilOp dppass) {
            // 1. Get logical state (top of stack)
            StencilState& logical_state = m_owner->m_stack.back().stencil_state;
            
            // 2. Update logical state
            logical_state.sfail = sfail;
            logical_state.dpfail = dpfail;
            logical_state.dppass = dppass;
            
            // 3. Get GPU state cache
            StencilState& gpu_state = m_owner->m_gpu_stencil_state;
            
            // 4. Compare and update if any parameter differs
            if (logical_state.sfail != gpu_state.sfail ||
                logical_state.dpfail != gpu_state.dpfail ||
                logical_state.dppass != gpu_state.dppass) {
                
                glStencilOp(
                    static_cast<GLenum>(sfail),
                    static_cast<GLenum>(dpfail),
                    static_cast<GLenum>(dppass));
                GL_CHECK("set stencil operation");
                
                gpu_state.sfail = logical_state.sfail;
                gpu_state.dpfail = logical_state.dpfail;
                gpu_state.dppass = logical_state.dppass;
            }
        }
        
        /**
         * @brief Sets stencil clear value (immediate operation)
         * @param value Value to clear stencil buffer to
         * @note This is an immediate operation that does not modify logical or GPU state cache
         */
        void setClearValue(GLint value) {
            glClearStencil(value);
            GL_CHECK("set stencil clear value");
        }
        
        /**
         * @brief Clears stencil buffer (immediate operation)
         * @note This is an immediate operation that does not modify logical or GPU state cache
         */
        void clearBuffer() {
            glClear(GL_STENCIL_BUFFER_BIT);
            GL_CHECK("clear stencil buffer");
        }
    };
    
    /**
     * @brief Returns StencilManager instance for configuring stencil state
     * @return StencilManager proxy object
     */
    StencilManager stencil() {
        return StencilManager(this);
    }
    
    /**
     * @class BlendManager
     * @brief Proxy class for configuring blend state
     * 
     * Provides methods to configure blending with automatic GPU state caching
     * to prevent redundant OpenGL calls. All methods update the logical state at the
     * top of the stack and compare with cached GPU state before issuing OpenGL calls.
     */
    class BlendManager {
    private:
        FramebufferStack* m_owner;  ///< Pointer to owning FramebufferStack
        
    public:
        /**
         * @brief Constructs BlendManager with owner pointer
         * @param owner Pointer to the FramebufferStack instance
         */
        explicit BlendManager(FramebufferStack* owner) : m_owner(owner) {}
        
        /**
         * @brief Enables or disables blending
         * @param enabled True to enable blending, false to disable
         */
        void setEnabled(bool enabled) {
            // 1. Get logical state (top of stack)
            BlendState& logical_state = m_owner->m_stack.back().blend_state;
            
            // 2. Update logical state
            logical_state.enabled = enabled;
            
            // 3. Get GPU state cache
            BlendState& gpu_state = m_owner->m_gpu_blend_state;
            
            // 4. Compare and update if different
            if (logical_state.enabled != gpu_state.enabled) {
                if (enabled) {
                    glEnable(GL_BLEND);
                } else {
                    glDisable(GL_BLEND);
                }
                GL_CHECK("set blend enabled");
                gpu_state.enabled = logical_state.enabled;
            }
        }
        
        /**
         * @brief Sets blend equation for both RGB and Alpha
         * @param mode Blend equation to use for both RGB and Alpha channels
         */
        void setEquation(BlendEquation mode) {
            // 1. Get logical state (top of stack)
            BlendState& logical_state = m_owner->m_stack.back().blend_state;
            
            // 2. Update logical state (both RGB and Alpha to same value)
            logical_state.equationRGB = mode;
            logical_state.equationAlpha = mode;
            
            // 3. Get GPU state cache
            BlendState& gpu_state = m_owner->m_gpu_blend_state;
            
            // 4. Compare and update if different
            if (logical_state.equationRGB != gpu_state.equationRGB ||
                logical_state.equationAlpha != gpu_state.equationAlpha) {
                
                glBlendEquation(static_cast<GLenum>(mode));
                GL_CHECK("set blend equation");
                
                gpu_state.equationRGB = logical_state.equationRGB;
                gpu_state.equationAlpha = logical_state.equationAlpha;
            }
        }
        
        /**
         * @brief Sets blend equation separately for RGB and Alpha
         * @param modeRGB Blend equation for RGB channels
         * @param modeAlpha Blend equation for Alpha channel
         */
        void setEquationSeparate(BlendEquation modeRGB, BlendEquation modeAlpha) {
            // 1. Get logical state (top of stack)
            BlendState& logical_state = m_owner->m_stack.back().blend_state;
            
            // 2. Update logical state
            logical_state.equationRGB = modeRGB;
            logical_state.equationAlpha = modeAlpha;
            
            // 3. Get GPU state cache
            BlendState& gpu_state = m_owner->m_gpu_blend_state;
            
            // 4. Compare and update if different
            if (logical_state.equationRGB != gpu_state.equationRGB ||
                logical_state.equationAlpha != gpu_state.equationAlpha) {
                
                glBlendEquationSeparate(
                    static_cast<GLenum>(modeRGB),
                    static_cast<GLenum>(modeAlpha));
                GL_CHECK("set blend equation separate");
                
                gpu_state.equationRGB = logical_state.equationRGB;
                gpu_state.equationAlpha = logical_state.equationAlpha;
            }
        }
        
        /**
         * @brief Sets blend function for both RGB and Alpha
         * @param sfactor Source blend factor for both RGB and Alpha
         * @param dfactor Destination blend factor for both RGB and Alpha
         */
        void setFunction(BlendFactor sfactor, BlendFactor dfactor) {
            // 1. Get logical state (top of stack)
            BlendState& logical_state = m_owner->m_stack.back().blend_state;
            
            // 2. Update logical state (all four factors to same RGB/Alpha values)
            logical_state.sfactorRGB = sfactor;
            logical_state.dfactorRGB = dfactor;
            logical_state.sfactorAlpha = sfactor;
            logical_state.dfactorAlpha = dfactor;
            
            // 3. Get GPU state cache
            BlendState& gpu_state = m_owner->m_gpu_blend_state;
            
            // 4. Compare and update if different
            if (logical_state.sfactorRGB != gpu_state.sfactorRGB ||
                logical_state.dfactorRGB != gpu_state.dfactorRGB ||
                logical_state.sfactorAlpha != gpu_state.sfactorAlpha ||
                logical_state.dfactorAlpha != gpu_state.dfactorAlpha) {
                
                glBlendFunc(
                    static_cast<GLenum>(sfactor),
                    static_cast<GLenum>(dfactor));
                GL_CHECK("set blend function");
                
                gpu_state.sfactorRGB = logical_state.sfactorRGB;
                gpu_state.dfactorRGB = logical_state.dfactorRGB;
                gpu_state.sfactorAlpha = logical_state.sfactorAlpha;
                gpu_state.dfactorAlpha = logical_state.dfactorAlpha;
            }
        }
        
        /**
         * @brief Sets blend function separately for RGB and Alpha
         * @param srcRGB Source blend factor for RGB channels
         * @param dstRGB Destination blend factor for RGB channels
         * @param srcAlpha Source blend factor for Alpha channel
         * @param dstAlpha Destination blend factor for Alpha channel
         */
        void setFunctionSeparate(BlendFactor srcRGB, BlendFactor dstRGB,
                                 BlendFactor srcAlpha, BlendFactor dstAlpha) {
            // 1. Get logical state (top of stack)
            BlendState& logical_state = m_owner->m_stack.back().blend_state;
            
            // 2. Update logical state
            logical_state.sfactorRGB = srcRGB;
            logical_state.dfactorRGB = dstRGB;
            logical_state.sfactorAlpha = srcAlpha;
            logical_state.dfactorAlpha = dstAlpha;
            
            // 3. Get GPU state cache
            BlendState& gpu_state = m_owner->m_gpu_blend_state;
            
            // 4. Compare and update if any factor differs
            if (logical_state.sfactorRGB != gpu_state.sfactorRGB ||
                logical_state.dfactorRGB != gpu_state.dfactorRGB ||
                logical_state.sfactorAlpha != gpu_state.sfactorAlpha ||
                logical_state.dfactorAlpha != gpu_state.dfactorAlpha) {
                
                glBlendFuncSeparate(
                    static_cast<GLenum>(srcRGB),
                    static_cast<GLenum>(dstRGB),
                    static_cast<GLenum>(srcAlpha),
                    static_cast<GLenum>(dstAlpha));
                GL_CHECK("set blend function separate");
                
                gpu_state.sfactorRGB = logical_state.sfactorRGB;
                gpu_state.dfactorRGB = logical_state.dfactorRGB;
                gpu_state.sfactorAlpha = logical_state.sfactorAlpha;
                gpu_state.dfactorAlpha = logical_state.dfactorAlpha;
            }
        }
        
        /**
         * @brief Sets blend constant color
         * @param r Red component (0.0 to 1.0)
         * @param g Green component (0.0 to 1.0)
         * @param b Blue component (0.0 to 1.0)
         * @param a Alpha component (0.0 to 1.0)
         */
        void setConstantColor(float r, float g, float b, float a) {
            // 1. Get logical state (top of stack)
            BlendState& logical_state = m_owner->m_stack.back().blend_state;
            
            // 2. Update logical state
            logical_state.constant_color_r = r;
            logical_state.constant_color_g = g;
            logical_state.constant_color_b = b;
            logical_state.constant_color_a = a;
            
            // 3. Get GPU state cache
            BlendState& gpu_state = m_owner->m_gpu_blend_state;
            
            // 4. Compare and update if any component differs
            if (logical_state.constant_color_r != gpu_state.constant_color_r ||
                logical_state.constant_color_g != gpu_state.constant_color_g ||
                logical_state.constant_color_b != gpu_state.constant_color_b ||
                logical_state.constant_color_a != gpu_state.constant_color_a) {
                
                glBlendColor(r, g, b, a);
                GL_CHECK("set blend constant color");
                
                gpu_state.constant_color_r = logical_state.constant_color_r;
                gpu_state.constant_color_g = logical_state.constant_color_g;
                gpu_state.constant_color_b = logical_state.constant_color_b;
                gpu_state.constant_color_a = logical_state.constant_color_a;
            }
        }
    };
    
    /**
     * @brief Returns BlendManager instance for configuring blend state
     * @return BlendManager proxy object
     */
    BlendManager blend() {
        return BlendManager(this);
    }
    
    /**
     * @class DepthManager
     * @brief Proxy class for configuring depth test state
     * 
     * Provides methods to configure depth testing with automatic GPU state caching
     * to prevent redundant OpenGL calls. All methods update the logical state at the
     * top of the stack and compare with cached GPU state before issuing OpenGL calls.
     */
    class DepthManager {
    private:
        FramebufferStack* m_owner;  ///< Pointer to owning FramebufferStack
        
    public:
        /**
         * @brief Constructs DepthManager with owner pointer
         * @param owner Pointer to the FramebufferStack instance
         */
        explicit DepthManager(FramebufferStack* owner) : m_owner(owner) {}
        
        /**
         * @brief Enables or disables depth testing
         * @param enabled True to enable depth test, false to disable
         */
        void setTest(bool enabled) {
            // 1. Get logical state (top of stack)
            DepthState& logical_state = m_owner->m_stack.back().depth_state;
            
            // 2. Update logical state
            logical_state.test_enabled = enabled;
            
            // 3. Get GPU state cache
            DepthState& gpu_state = m_owner->m_gpu_depth_state;
            
            // 4. Compare and update if different
            if (logical_state.test_enabled != gpu_state.test_enabled) {
                if (enabled) {
                    glEnable(GL_DEPTH_TEST);
                } else {
                    glDisable(GL_DEPTH_TEST);
                }
                GL_CHECK("set depth test");
                gpu_state.test_enabled = logical_state.test_enabled;
            }
        }
        
        /**
         * @brief Enables or disables depth buffer writes
         * @param enabled True to enable depth writes, false to disable
         */
        void setWrite(bool enabled) {
            // 1. Get logical state (top of stack)
            DepthState& logical_state = m_owner->m_stack.back().depth_state;
            
            // 2. Update logical state
            logical_state.write_enabled = enabled;
            
            // 3. Get GPU state cache
            DepthState& gpu_state = m_owner->m_gpu_depth_state;
            
            // 4. Compare and update if different
            if (logical_state.write_enabled != gpu_state.write_enabled) {
                glDepthMask(enabled ? GL_TRUE : GL_FALSE);
                GL_CHECK("set depth write mask");
                gpu_state.write_enabled = logical_state.write_enabled;
            }
        }
        
        /**
         * @brief Sets depth test comparison function
         * @param func Depth comparison function
         */
        void setFunction(DepthFunc func) {
            // 1. Get logical state (top of stack)
            DepthState& logical_state = m_owner->m_stack.back().depth_state;
            
            // 2. Update logical state
            logical_state.func = func;
            
            // 3. Get GPU state cache
            DepthState& gpu_state = m_owner->m_gpu_depth_state;
            
            // 4. Compare and update if different
            if (logical_state.func != gpu_state.func) {
                glDepthFunc(static_cast<GLenum>(func));
                GL_CHECK("set depth function");
                gpu_state.func = logical_state.func;
            }
        }
        
        /**
         * @brief Enables or disables depth clamping
         * @param enabled True to enable depth clamping, false to disable
         */
        void setClamp(bool enabled) {
            // 1. Get logical state (top of stack)
            DepthState& logical_state = m_owner->m_stack.back().depth_state;
            
            // 2. Update logical state
            logical_state.clamp_enabled = enabled;
            
            // 3. Get GPU state cache
            DepthState& gpu_state = m_owner->m_gpu_depth_state;
            
            // 4. Compare and update if different
            if (logical_state.clamp_enabled != gpu_state.clamp_enabled) {
                if (enabled) {
                    glEnable(GL_DEPTH_CLAMP);
                } else {
                    glDisable(GL_DEPTH_CLAMP);
                }
                GL_CHECK("set depth clamp");
                gpu_state.clamp_enabled = logical_state.clamp_enabled;
            }
        }
        
        /**
         * @brief Sets depth range
         * @param near_val Near depth range value (default: 0.0)
         * @param far_val Far depth range value (default: 1.0)
         */
        void setRange(double near_val, double far_val) {
            // 1. Get logical state (top of stack)
            DepthState& logical_state = m_owner->m_stack.back().depth_state;
            
            // 2. Update logical state
            logical_state.range_near = near_val;
            logical_state.range_far = far_val;
            
            // 3. Get GPU state cache
            DepthState& gpu_state = m_owner->m_gpu_depth_state;
            
            // 4. Compare and update if different
            if (logical_state.range_near != gpu_state.range_near ||
                logical_state.range_far != gpu_state.range_far) {
                
                glDepthRange(near_val, far_val);
                GL_CHECK("set depth range");
                
                gpu_state.range_near = logical_state.range_near;
                gpu_state.range_far = logical_state.range_far;
            }
        }
    };
    
    /**
     * @brief Returns DepthManager instance for configuring depth state
     * @return DepthManager proxy object
     */
    DepthManager depth() {
        return DepthManager(this);
    }
    
    // Delete copy constructor and assignment operator (singleton)
    FramebufferStack(const FramebufferStack&) = delete;
    FramebufferStack& operator=(const FramebufferStack&) = delete;
    
    /**
     * @brief Pushes framebuffer with state inheritance (inherit mode)
     * @param fbo Framebuffer to push onto stack
     * @note Inherits stencil/blend state from parent, no GL calls for state
     * @note Optimized: only updates changed FBO/viewport/draw buffers
     */
    void push(FramebufferPtr fbo);
    
    /**
     * @brief Pushes framebuffer with pre-configured state (apply mode)
     * @param fbo Framebuffer to push onto stack
     * @param state_to_apply Pre-configured RenderState to apply atomically
     * @note Applies RenderState atomically, issues GL calls via syncGpuToState
     * @note Optimized: only updates state fields that differ from GPU cache
     */
    void push(FramebufferPtr fbo, RenderStatePtr state_to_apply);
    
    /**
     * @brief Pops and restores previous framebuffer state
     * @note Cannot pop base state (default framebuffer)
     */
    void pop();
    
    /** @brief Returns current framebuffer (nullptr if default) */
    FramebufferPtr top() const {
        return m_stack.back().fbo;
    }
    
    /** @brief Checks if default framebuffer is active */
    bool isDefaultFramebuffer() const {
        return m_stack.back().fbo == nullptr;
    }
    
    /** @brief Gets current viewport width */
    int getCurrentWidth() const {
        return m_current_viewport_width;
    }
    
    /** @brief Gets current viewport height */
    int getCurrentHeight() const {
        return m_current_viewport_height;
    }
};

// FramebufferStack method implementations

inline void FramebufferStack::push(FramebufferPtr fbo) {
    // Get current state for inheritance
    const FramebufferState& previous_state = m_stack.back();
    
    // Check if pushing default framebuffer
    bool is_default_fbo = (fbo == nullptr);
    
    // Create new state
    FramebufferState state;
    state.fbo = fbo;
    
    // Set viewport and draw buffers based on FBO type
    if (is_default_fbo) {
        // Query window size from GLFW for default framebuffer
        GLFWwindow* window = glfwGetCurrentContext();
        int window_width, window_height;
        glfwGetFramebufferSize(window, &window_width, &window_height);
        
        state.viewport_width = window_width;
        state.viewport_height = window_height;
        state.draw_buffers = { GL_BACK };
    } else {
        // Use FBO dimensions and color attachments
        state.viewport_width = fbo->getWidth();
        state.viewport_height = fbo->getHeight();
        state.draw_buffers = fbo->m_color_attachments;  // Access via friend
    }
    
    // Inherit stencil, blend, and depth state from parent (no GL calls)
    state.stencil_state = previous_state.stencil_state;
    state.blend_state = previous_state.blend_state;
    state.depth_state = previous_state.depth_state;
    
    m_stack.push_back(state);
    
    // Determine target FBO ID
    GLuint target_fbo_id = is_default_fbo ? 0 : fbo->getID();
    
    // Bind FBO (with optimization - only bind if changed)
    if (m_currently_bound_fbo != target_fbo_id) {
        if (is_default_fbo) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            GL_CHECK("bind default framebuffer");
        } else {
            fbo->bind();
        }
        m_currently_bound_fbo = target_fbo_id;
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
    if (is_default_fbo) {
        glDrawBuffer(GL_BACK);
        GL_CHECK("set default draw buffer");
    } else if (state.draw_buffers.empty()) {
        // Depth-only FBO (e.g., shadow map)
        glDrawBuffer(GL_NONE);
        GL_CHECK("set draw buffer to none");
    } else {
        // Color attachments present
        glDrawBuffers(static_cast<GLsizei>(state.draw_buffers.size()), state.draw_buffers.data());
        GL_CHECK("set draw buffers");
    }
    
    // CRITICAL: Do NOT call syncGpuToState() in inherit mode
    // State is logically identical to previous level, no GPU state changes needed
}

inline void FramebufferStack::push(FramebufferPtr fbo, RenderStatePtr state_to_apply) {
    if (!state_to_apply) {
        throw exception::FramebufferException("Attempted to push a null RenderStatePtr to the FramebufferStack. Use the push(fbo) overload for state inheritance.");
    }
    
    // Check if pushing default framebuffer
    bool is_default_fbo = (fbo == nullptr);
    
    // Create new state
    FramebufferState state;
    state.fbo = fbo;
    
    // Set viewport and draw buffers based on FBO type
    if (is_default_fbo) {
        // Query window size from GLFW for default framebuffer
        GLFWwindow* window = glfwGetCurrentContext();
        int window_width, window_height;
        glfwGetFramebufferSize(window, &window_width, &window_height);
        
        state.viewport_width = window_width;
        state.viewport_height = window_height;
        state.draw_buffers = { GL_BACK };
    } else {
        // Use FBO dimensions and color attachments
        state.viewport_width = fbo->getWidth();
        state.viewport_height = fbo->getHeight();
        state.draw_buffers = fbo->m_color_attachments;  // Access via friend
    }
    
    // Apply pre-configured state (access via friend)
    state.stencil_state = state_to_apply->m_stencil_state;
    state.blend_state = state_to_apply->m_blend_state;
    state.depth_state = state_to_apply->m_depth_state;
    
    m_stack.push_back(state);
    
    // Determine target FBO ID
    GLuint target_fbo_id = is_default_fbo ? 0 : fbo->getID();
    
    // Bind FBO (with optimization - only bind if changed)
    if (m_currently_bound_fbo != target_fbo_id) {
        if (is_default_fbo) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            GL_CHECK("bind default framebuffer");
        } else {
            fbo->bind();
        }
        m_currently_bound_fbo = target_fbo_id;
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
    if (is_default_fbo) {
        glDrawBuffer(GL_BACK);
        GL_CHECK("set default draw buffer");
    } else if (state.draw_buffers.empty()) {
        // Depth-only FBO (e.g., shadow map)
        glDrawBuffer(GL_NONE);
        GL_CHECK("set draw buffer to none");
    } else {
        // Color attachments present
        glDrawBuffers(static_cast<GLsizei>(state.draw_buffers.size()), state.draw_buffers.data());
        GL_CHECK("set draw buffers");
    }
    
    // CRITICAL: Call syncGpuToState() to apply the new state atomically
    syncGpuToState(m_stack.back());
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
    
    // Restore stencil and blend state
    syncGpuToState(state_to_restore);
}

inline void FramebufferStack::syncGpuToState(const FramebufferState& target_state) {
    // ========== Sync Stencil State ==========
    const StencilState& target_stencil = target_state.stencil_state;
    
    // Compare and sync stencil test enabled/disabled
    if (target_stencil.enabled != m_gpu_stencil_state.enabled) {
        if (target_stencil.enabled) {
            glEnable(GL_STENCIL_TEST);
        } else {
            glDisable(GL_STENCIL_TEST);
        }
        GL_CHECK("sync stencil test");
        m_gpu_stencil_state.enabled = target_stencil.enabled;
    }
    
    // Compare and sync stencil write mask
    if (target_stencil.write_mask != m_gpu_stencil_state.write_mask) {
        glStencilMask(target_stencil.write_mask);
        GL_CHECK("sync stencil write mask");
        m_gpu_stencil_state.write_mask = target_stencil.write_mask;
    }
    
    // Compare and sync stencil function (func, ref, func_mask)
    if (target_stencil.func != m_gpu_stencil_state.func ||
        target_stencil.ref != m_gpu_stencil_state.ref ||
        target_stencil.func_mask != m_gpu_stencil_state.func_mask) {
        
        glStencilFunc(
            static_cast<GLenum>(target_stencil.func),
            target_stencil.ref,
            target_stencil.func_mask);
        GL_CHECK("sync stencil function");
        
        m_gpu_stencil_state.func = target_stencil.func;
        m_gpu_stencil_state.ref = target_stencil.ref;
        m_gpu_stencil_state.func_mask = target_stencil.func_mask;
    }
    
    // Compare and sync stencil operations (sfail, dpfail, dppass)
    if (target_stencil.sfail != m_gpu_stencil_state.sfail ||
        target_stencil.dpfail != m_gpu_stencil_state.dpfail ||
        target_stencil.dppass != m_gpu_stencil_state.dppass) {
        
        glStencilOp(
            static_cast<GLenum>(target_stencil.sfail),
            static_cast<GLenum>(target_stencil.dpfail),
            static_cast<GLenum>(target_stencil.dppass));
        GL_CHECK("sync stencil operation");
        
        m_gpu_stencil_state.sfail = target_stencil.sfail;
        m_gpu_stencil_state.dpfail = target_stencil.dpfail;
        m_gpu_stencil_state.dppass = target_stencil.dppass;
    }
    
    // ========== Sync Blend State ==========
    const BlendState& target_blend = target_state.blend_state;
    
    // Compare and sync blend enabled/disabled
    if (target_blend.enabled != m_gpu_blend_state.enabled) {
        if (target_blend.enabled) {
            glEnable(GL_BLEND);
        } else {
            glDisable(GL_BLEND);
        }
        GL_CHECK("sync blend");
        m_gpu_blend_state.enabled = target_blend.enabled;
    }
    
    // Compare and sync blend equations (RGB and Alpha)
    if (target_blend.equationRGB != m_gpu_blend_state.equationRGB ||
        target_blend.equationAlpha != m_gpu_blend_state.equationAlpha) {
        
        glBlendEquationSeparate(
            static_cast<GLenum>(target_blend.equationRGB),
            static_cast<GLenum>(target_blend.equationAlpha));
        GL_CHECK("sync blend equation");
        
        m_gpu_blend_state.equationRGB = target_blend.equationRGB;
        m_gpu_blend_state.equationAlpha = target_blend.equationAlpha;
    }
    
    // Compare and sync blend factors (source and destination, RGB and Alpha)
    if (target_blend.sfactorRGB != m_gpu_blend_state.sfactorRGB ||
        target_blend.dfactorRGB != m_gpu_blend_state.dfactorRGB ||
        target_blend.sfactorAlpha != m_gpu_blend_state.sfactorAlpha ||
        target_blend.dfactorAlpha != m_gpu_blend_state.dfactorAlpha) {
        
        glBlendFuncSeparate(
            static_cast<GLenum>(target_blend.sfactorRGB),
            static_cast<GLenum>(target_blend.dfactorRGB),
            static_cast<GLenum>(target_blend.sfactorAlpha),
            static_cast<GLenum>(target_blend.dfactorAlpha));
        GL_CHECK("sync blend function");
        
        m_gpu_blend_state.sfactorRGB = target_blend.sfactorRGB;
        m_gpu_blend_state.dfactorRGB = target_blend.dfactorRGB;
        m_gpu_blend_state.sfactorAlpha = target_blend.sfactorAlpha;
        m_gpu_blend_state.dfactorAlpha = target_blend.dfactorAlpha;
    }
    
    // Compare and sync blend constant color
    if (target_blend.constant_color_r != m_gpu_blend_state.constant_color_r ||
        target_blend.constant_color_g != m_gpu_blend_state.constant_color_g ||
        target_blend.constant_color_b != m_gpu_blend_state.constant_color_b ||
        target_blend.constant_color_a != m_gpu_blend_state.constant_color_a) {
        
        glBlendColor(
            target_blend.constant_color_r,
            target_blend.constant_color_g,
            target_blend.constant_color_b,
            target_blend.constant_color_a);
        GL_CHECK("sync blend constant color");
        
        m_gpu_blend_state.constant_color_r = target_blend.constant_color_r;
        m_gpu_blend_state.constant_color_g = target_blend.constant_color_g;
        m_gpu_blend_state.constant_color_b = target_blend.constant_color_b;
        m_gpu_blend_state.constant_color_a = target_blend.constant_color_a;
    }
    
    // ========== Sync Depth State ==========
    const DepthState& target_depth = target_state.depth_state;
    
    // Compare and sync depth test enabled/disabled
    if (target_depth.test_enabled != m_gpu_depth_state.test_enabled) {
        if (target_depth.test_enabled) {
            glEnable(GL_DEPTH_TEST);
        } else {
            glDisable(GL_DEPTH_TEST);
        }
        GL_CHECK("sync depth test");
        m_gpu_depth_state.test_enabled = target_depth.test_enabled;
    }
    
    // Compare and sync depth write mask
    if (target_depth.write_enabled != m_gpu_depth_state.write_enabled) {
        glDepthMask(target_depth.write_enabled ? GL_TRUE : GL_FALSE);
        GL_CHECK("sync depth write mask");
        m_gpu_depth_state.write_enabled = target_depth.write_enabled;
    }
    
    // Compare and sync depth function
    if (target_depth.func != m_gpu_depth_state.func) {
        glDepthFunc(static_cast<GLenum>(target_depth.func));
        GL_CHECK("sync depth function");
        m_gpu_depth_state.func = target_depth.func;
    }
    
    // Compare and sync depth clamp enabled/disabled
    if (target_depth.clamp_enabled != m_gpu_depth_state.clamp_enabled) {
        if (target_depth.clamp_enabled) {
            glEnable(GL_DEPTH_CLAMP);
        } else {
            glDisable(GL_DEPTH_CLAMP);
        }
        GL_CHECK("sync depth clamp");
        m_gpu_depth_state.clamp_enabled = target_depth.clamp_enabled;
    }
    
    // Compare and sync depth range
    if (target_depth.range_near != m_gpu_depth_state.range_near ||
        target_depth.range_far != m_gpu_depth_state.range_far) {
        
        glDepthRange(target_depth.range_near, target_depth.range_far);
        GL_CHECK("sync depth range");
        
        m_gpu_depth_state.range_near = target_depth.range_near;
        m_gpu_depth_state.range_far = target_depth.range_far;
    }
}

/**
 * @brief Singleton accessor for FramebufferStack (Meyers pattern)
 */
inline FramebufferStackPtr stack() {
    static FramebufferStackPtr instance = FramebufferStackPtr(new FramebufferStack());
    return instance;
}

} // namespace framebuffer

#endif // FRAMEBUFFER_H
