// a textura de fato e a cor de reflexão/material

#ifndef TEXTURE_H
#define TEXTURE_H
#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <functional>

#include "gl_includes.h"
#include "error.h"

// This implementation uses the popular stb_image library for loading images.
// You'll need to add stb_image.h to your project and define STB_IMAGE_IMPLEMENTATION
// in one of your .cpp files. You can get it from here: https://github.com/nothings/stb
#define STB_IMAGE_STATIC // solves the case of two different .cpp including texture.h
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

// Forward declaration for friend class
namespace framebuffer {
    class Framebuffer;
}

namespace texture {

// Base interface for all texture types
class ITexture {
public:
    virtual ~ITexture() = default;
    virtual void Bind(GLuint unit = 0) const = 0;
    virtual void Unbind(GLuint unit = 0) const = 0;
    virtual GLuint GetTextureID() const = 0;
    virtual GLenum GetTextureTarget() const = 0;
};

using ITexturePtr = std::shared_ptr<ITexture>;

class Texture;
using TexturePtr = std::shared_ptr<Texture>;

class TextureStack;
using TextureStackPtr = std::shared_ptr<TextureStack>;


// Helper function to load image data and configure an OpenGL texture.
// This parallels the MakeShader function in shader.h.
static void LoadAndConfigureTexture(GLuint tid, const std::string& filename, int& width, int& height) {
    glBindTexture(GL_TEXTURE_2D, tid);
    GL_CHECK("bind texture for configuration");

    // Set texture wrapping and filtering options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GL_CHECK("set texture parameters");

    // Load image data using stb_image
    int channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 0);

    // Error check
    if (!data) {
        std::cerr << "Failed to load texture file: " << filename << std::endl;
        return;
    }

    GLenum internalFormat = GL_RGB8; // How the GPU stores it
    GLenum dataFormat = GL_RGB;      // The format of the source 'data'

    std::cout << "Loaded texture file: " << filename << std::endl;
    std::cout << "  - Width: " << width << ", Channels: " << channels << std::endl;
    std::cout << "  - Bytes per row: " << (width * channels) << std::endl;

    if (channels == 4) {
        internalFormat = GL_RGBA8;
        dataFormat = GL_RGBA;
    } else if (channels == 3) {
        internalFormat = GL_RGB8;
        dataFormat = GL_RGB;
    } else if (channels == 1) {
        // Store as 3-channel RGB, but the source is 1-channel
        // This makes OpenGL convert (R) -> (R, R, R) for a grayscale image
        internalFormat = GL_RGB8;
        dataFormat = GL_RED;
    }

    // Tell OpenGL data is tightly packed
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    GL_CHECK("set 1-byte pixel alignment");

    // Upload texture data to the GPU
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
    GL_CHECK("upload texture data");
    
    // Restore default alignment
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    GL_CHECK("restore 4-byte pixel alignment");

    // Generate mipmaps AFTER uploading data
    glGenerateMipmap(GL_TEXTURE_2D);
    GL_CHECK("generate mipmaps");

    // Free the image data from CPU memory
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
}


class Texture : public ITexture {
private:
    GLuint m_tid;
    int m_width;
    int m_height;
    inline static std::unordered_map<std::string, TexturePtr> s_cache;

    // Friend declaration for Framebuffer class to access protected constructor
    friend class framebuffer::Framebuffer;

protected:
    // Constructor for FBO texture creation (used by Framebuffer)
    Texture(GLuint tid, int width, int height)
        : m_tid(tid), m_width(width), m_height(height) {}

    // Constructor acquires the OpenGL resource.
    Texture(const std::string& filename) : m_width(0), m_height(0) {
        glGenTextures(1, &m_tid);
        GL_CHECK("generate texture");

        if (m_tid == 0) {
            std::cerr << "Could not create texture object" << std::endl;
            return;
        }

        // Use helper to load and configure the texture
        LoadAndConfigureTexture(m_tid, filename, m_width, m_height);
    }

    Texture(int width, int height, unsigned char* data) : m_width(width), m_height(height) {
        glGenTextures(1, &m_tid);
        GL_CHECK("generate texture");

        if (m_tid == 0) {
            std::cerr << "Could not create texture object" << std::endl;
            return;
        }

        glBindTexture(GL_TEXTURE_2D, m_tid);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, 0);
    }
public:
    static TexturePtr Make(const std::string& filename) {
        // 1. Check if the texture is already in our cache.
        auto it = s_cache.find(filename);
        if (it != s_cache.end()) {
            // If it is, return the existing pointer. No new texture is created.
            return it->second;
        }

        // 2. If not found, create a new Texture object.
        //    The constructor will be called, loading the file and uploading to the GPU.
        TexturePtr new_texture = TexturePtr(new Texture(filename));

        // 3. Store the newly created texture in the cache for future requests.
        s_cache[filename] = new_texture;

        // 4. Return the new texture.
        return new_texture;
    }

    static TexturePtr Make(int width, int height, unsigned char* data) {
        return TexturePtr(new Texture(width, height, data));
    }
    ~Texture() {
        glDeleteTextures(1, &m_tid);
    }
    
    // ITexture interface implementation
    void Bind(GLuint unit = 0) const override {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, m_tid);
    }
    void Unbind(GLuint unit = 0) const override {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    GLuint GetTextureID() const override {
        return m_tid;
    }
    
    GLenum GetTextureTarget() const override {
        return GL_TEXTURE_2D;
    }

    int GetWidth() const {
        return m_width;
    }

    int GetHeight() const {
        return m_height;
    }

    /**
     * @brief Generate mipmaps for this texture.
     * 
     * This method generates mipmaps for the texture, which are pre-calculated,
     * optimized sequences of images that accompany a main texture, intended to
     * increase rendering speed and reduce aliasing artifacts.
     */
    void generateMipmaps() {
        glBindTexture(GL_TEXTURE_2D, m_tid);
        glGenerateMipmap(GL_TEXTURE_2D);
        GL_CHECK("generate mipmaps");
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    /**
     * @brief Configure texture parameters after creation.
     * 
     * This method allows post-creation configuration of texture parameters,
     * which is useful for FBO textures that need specific wrapping or filtering modes.
     * 
     * @param wrapS Wrapping mode for S coordinate (e.g., GL_CLAMP_TO_EDGE, GL_REPEAT)
     * @param wrapT Wrapping mode for T coordinate (e.g., GL_CLAMP_TO_EDGE, GL_REPEAT)
     * @param minFilter Minification filter (e.g., GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR)
     * @param magFilter Magnification filter (e.g., GL_LINEAR, GL_NEAREST)
     */
    void setTextureParameters(GLenum wrapS, GLenum wrapT, GLenum minFilter, GLenum magFilter) {
        glBindTexture(GL_TEXTURE_2D, m_tid);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
        GL_CHECK("set texture parameters");
        glBindTexture(GL_TEXTURE_2D, 0);
    }
};


// Your proposed TextureStack singleton.
class TextureStack {
private:
    // This is the core of the state machine.
    // It's a stack of maps. Each map represents the complete texture state
    // (all active units) at that point in the scene graph.
    std::vector<std::unordered_map<GLuint, ITexturePtr>> m_stack;
    // This map links a sampler uniform name to a texture unit.
    std::unordered_map<std::string, GLuint> m_sampler_to_unit_map;

    // This map tracks the ACTUAL state on the GPU to prevent redundant calls.
    std::unordered_map<GLuint, ITexturePtr> m_active_gpu_state;

    TextureStack() {
        // Start with a base level on the stack representing the default (empty) state.
        m_stack.push_back({});
    }
    friend TextureStackPtr stack();

public:
    TextureStack(const TextureStack&) = delete;
    TextureStack& operator=(const TextureStack&) = delete;

    ITexturePtr top() {
        if (m_stack.empty()) {
            return nullptr;
        }
        auto& state = m_stack.back();
        if (state.empty()) {
            return nullptr;
        }
        return state.begin()->second;
    }

    // The intelligent push operation.
    void push(ITexturePtr texture, GLuint unit = 0) {
        // 1. Get the state from the previous level of the stack.
        auto new_state = m_stack.back();
        // 2. Modify it with the new texture.
        new_state[unit] = texture;
        // 3. Push the new, complete state onto our stack.
        m_stack.push_back(new_state);

        // --- Non-repeating logic is here ---
        // 4. Only bind if the GPU state for this unit is different.
        if (m_active_gpu_state[unit] != texture) {
            texture->Bind(unit);
            m_active_gpu_state[unit] = texture; // Update our cache
        }
    }

    // The intelligent pop operation.
    void pop() {
        if (m_stack.size() <= 1) {
            std::cerr << "Warning: Attempt to pop the base texture state." << std::endl;
            return;
        }

        // 1. Remove the current state from the stack.
        m_stack.pop_back();
        // 2. Get the state we are restoring to.
        const auto& state_to_restore = m_stack.back();

        // --- Non-repeating logic is here ---
        // 3. Synchronize the GPU state with the state we are restoring.
        // This is more complex: we check for changes and apply them.
        for (auto const& [unit, tex] : m_active_gpu_state) {
            // If the unit in the state-to-restore is different from the active one...
            if (state_to_restore.count(unit) == 0 || state_to_restore.at(unit) != tex) {
                // ... we need to update the binding.
                if (state_to_restore.count(unit)) {
                    // Restore to the previous texture
                    state_to_restore.at(unit)->Bind(unit);
                } else {
                    // Or unbind if this unit is no longer used
                    tex->Unbind(unit);
                }
            }
        }
        // 4. Update our cache to match the new state.
        m_active_gpu_state = state_to_restore;
    }

    // NEW: Called by TextureComponent during its 'apply' phase.
    void registerSamplerUnit(const std::string& sampler_name, GLuint unit) {
        m_sampler_to_unit_map[sampler_name] = unit;
    }

    // NEW: Called by TextureComponent during its 'unapply' phase.
    void unregisterSamplerUnit(const std::string& sampler_name) {
        m_sampler_to_unit_map.erase(sampler_name);
    }

    // NEW: Called by the shader's uniform provider to get the current unit for a sampler.
    GLuint getUnitForSampler(const std::string& sampler_name) const {
        auto it = m_sampler_to_unit_map.find(sampler_name);
        if (it != m_sampler_to_unit_map.end()) {
            return it->second;
        }
        // Return 0 and log a warning if the sampler isn't registered.
        // Returning 0 is often a safe default.
        std::cerr << "Warning: Sampler '" << sampler_name << "' not registered. Defaulting to unit 0." << std::endl;
        return 0;
    }
};

// Singleton accessor function, just like shader::stack().
inline TextureStackPtr stack() {
    static TextureStackPtr instance = TextureStackPtr(new TextureStack());
    return instance;
}

// This is the bridge between the shader system and the texture system.
inline std::function<int()> getUnitProvider(const std::string& samplerName) {
    // Return a lambda that captures the sampler name by value.
    // This lambda will be called by the shader every frame to get the uniform's value.
    return [samplerName]() -> int {
        // It pulls the up-to-date texture unit from the global stack.
        return static_cast<int>(texture::stack()->getUnitForSampler(samplerName));
    };
}

/**
 * @brief Returns a provider function for sampler uniforms (works for all sampler types).
 * 
 * This is the type-safe version that returns uniform::detail::Sampler
 * which works for sampler2D, samplerCube, and other sampler types.
 * 
 * Example:
 * @code
 * // For sampler2D:
 * shader->configureDynamicUniform<uniform::detail::Sampler>("u_texture", 
 *     texture::getSamplerProvider("u_texture"));
 * 
 * // For samplerCube:
 * shader->configureDynamicUniform<uniform::detail::Sampler>("u_cubemap", 
 *     texture::getSamplerProvider("u_cubemap"));
 * @endcode
 */
inline std::function<uniform::detail::Sampler()> getSamplerProvider(const std::string& samplerName) {
    return [samplerName]() -> uniform::detail::Sampler {
        return uniform::detail::Sampler{static_cast<int>(texture::stack()->getUnitForSampler(samplerName))};
    };
}

} // namespace texture
#endif // TEXTURE_H