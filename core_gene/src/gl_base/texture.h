// a textura de fato e a cor de reflexão/material

#ifndef TEXTURE_H
#define TEXTURE_H
#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <iostream>

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

class TextureStack;
using TextureStackPtr = std::shared_ptr<TextureStack>;


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
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 0);

    // Error check
    if (!data) {
        std::cerr << "Failed to load texture file: " << filename << std::endl;
        return;
    }

    // Determine format based on number of channels
    GLenum format = GL_RGB;
    if (channels == 4) {
        format = GL_RGBA;
    } else if (channels == 3) {
        format = GL_RGB;
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
    glBindTexture(GL_TEXTURE_2D, 0);
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
            return;
        }

        // Use helper to load and configure the texture
        LoadAndConfigureTexture(m_tid, filename, m_width, m_height);
    }
public:
    static TexturePtr Make(const std::string& filename) {
        return TexturePtr(new Texture(filename));
    }
    ~Texture() {
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


// Your proposed TextureStack singleton.
class TextureStack {
private:
    // This is the core of the state machine.
    // It's a stack of maps. Each map represents the complete texture state
    // (all active units) at that point in the scene graph.
    std::vector<std::map<GLuint, TexturePtr>> m_stack;

    // This map tracks the ACTUAL state on the GPU to prevent redundant calls.
    std::map<GLuint, TexturePtr> m_active_gpu_state;

    TextureStack() {
        // Start with a base level on the stack representing the default (empty) state.
        m_stack.push_back({});
    }
    friend TextureStackPtr stack();

public:
    TextureStack(const TextureStack&) = delete;
    TextureStack& operator=(const TextureStack&) = delete;

    TexturePtr top() {
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
    void push(TexturePtr texture, GLuint unit = 0) {
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
};

// Singleton accessor function, just like shader::stack().
inline TextureStackPtr stack() {
    static TextureStackPtr instance = TextureStackPtr(new TextureStack());
    return instance;
}

} // namespace texture
#endif // TEXTURE_H