// a textura de fato e a cor de reflexão/material

#ifndef TEXTURE_H
#define TEXTURE_H
#pragma once

#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <map>

#include "gl_includes.h"
#include "error.h"

// This implementation uses the popular stb_image library for loading images.
// You'll need to add stb_image.h to your project and define STB_IMAGE_IMPLEMENTATION
// in one of your .cpp files. You can get it from here: https://github.com/nothings/stb
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

namespace texture {

class Texture;
using TexturePtr = std::shared_ptr<Texture>;

class TextureManager;
using TextureManagerPtr = std::shared_ptr<TextureManager>;

// Helper function to load image data and configure an OpenGL texture.
// This parallels the MakeShader function in shader.h.
static void LoadAndConfigureTexture(GLuint tid, const std::string& filename, int& width, int& height) {
    glBindTexture(GL_TEXTURE_2D, tid);
    Error::Check("bind texture for configuration");

    // Set texture wrapping and filtering options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    Error::Check("set texture parameters");

    // Load image data using stb_image
    int channels;
    stbi_set_flip_vertically_on_load(true); // Most OpenGL projects need this
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 0);

    // Error check
    if (!data) {
        std::cerr << "Failed to load texture file: " << filename << std::endl;
        exit(1);
    }

    // Determine format based on number of channels
    GLenum format = GL_RGB;
    if (channels == 4) {
        format = GL_RGBA;
    } else if (channels == 1) {
        format = GL_RED;
    }

    // Upload texture data to the GPU
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    Error::Check("upload texture data");
    glGenerateMipmap(GL_TEXTURE_2D);
    Error::Check("generate mipmaps");

    // Free the image data from CPU memory
    stbi_image_free(data);

    glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture
}

class Texture {
private:
    GLuint m_tid;
    int m_width;
    int m_height;

protected:
    // Constructor acquires the OpenGL resource.
    Texture(const std::string& filename) : m_width(0), m_height(0) {
        glGenTextures(1, &m_tid);
        Error::Check("generate texture");

        if (m_tid == 0) {
            std::cerr << "Could not create texture object" << std::endl;
            exit(1);
        }

        // Use helper to load and configure the texture
        LoadAndConfigureTexture(m_tid, filename, m_width, m_height);
    }

public:
    // Factory function is the only public way to create a Texture.
    static TexturePtr Make(const std::string& filename) {
        // We use 'new Texture' because the constructor is protected.
        return TexturePtr(new Texture(filename));
    }

    // Destructor releases the OpenGL resource.
    virtual ~Texture() {
        glDeleteTextures(1, &m_tid);
    }

    void Bind(GLuint unit = 0) const {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, m_tid);
    }

    void Unbind(GLuint unit = 0) const {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    GLuint GetTextureID() const {
        return m_tid;
    }

    int GetWidth() const {
        return m_width;
    }

    int GetHeight() const {
        return m_height;
    }
};


// Singleton manager for texture state, parallels ShaderStack.
class TextureManager {
private:
    // Tracks the currently bound texture for each texture unit.
    std::map<GLuint, TexturePtr> m_active_textures;

    TextureManager() = default;

    // Grant friendship to the singleton accessor function.
    friend TextureManagerPtr manager();

public:
    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;
    ~TextureManager() = default;

    void bind(TexturePtr texture, GLuint unit = 0) {
        // Only re-bind if the texture is different from the currently active one on this unit.
        if (m_active_textures[unit] != texture) {
            texture->Bind(unit);
            m_active_textures[unit] = texture;
        }
    }

    void unbind(GLuint unit = 0) {
        if (m_active_textures.count(unit)) {
            m_active_textures[unit]->Unbind(unit);
            m_active_textures.erase(unit);
        }
    }
};

// Singleton accessor function, parallels stack().
inline TextureManagerPtr manager() {
    static TextureManagerPtr instance = TextureManagerPtr(new TextureManager());
    return instance;
}

} // namespace texture

#endif // TEXTURE_H