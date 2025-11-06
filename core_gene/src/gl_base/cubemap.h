#ifndef CUBEMAP_H
#define CUBEMAP_H
#pragma once

#include <memory>
#include <string>
#include <array>
#include <unordered_map>
#include <iostream>

#include "gl_includes.h"
#include "error.h"
#include "texture.h"
#include "../exceptions/texture_exception.h"

// stb_image is already included in texture.h with implementation
// Do not include it again here

namespace texture {

class Cubemap;
using CubemapPtr = std::shared_ptr<Cubemap>;

/**
 * @class Cubemap
 * @brief Manages OpenGL cubemap textures with RAII semantics.
 * 
 * This class provides three loading modes:
 * 1. Individual face files (6 separate images)
 * 2. Cross-layout extraction (single image with 6 faces arranged in cross pattern)
 * 3. Direct data upload (raw pixel data for each face)
 * 
 * Cubemaps are automatically cached to prevent duplicate GPU uploads.
 * The class inherits from ITexture for polymorphic texture handling.
 */
class Cubemap : public ITexture {
private:
    GLuint m_tid;                    // OpenGL texture ID
    int m_face_width;                // Width of each cube face
    int m_face_height;               // Height of each cube face
    
    // Static cache to prevent duplicate GPU uploads
    inline static std::unordered_map<std::string, CubemapPtr> s_cache;
    
    // Private constructors for RAII - only accessible via factory methods
    
    /**
     * @brief Constructor for loading from individual face files.
     * @param face_paths Array of 6 file paths in OpenGL cubemap order:
     *        [0] = +X (right), [1] = -X (left),
     *        [2] = +Y (top),   [3] = -Y (bottom),
     *        [4] = +Z (front), [5] = -Z (back)
     */
    Cubemap(const std::array<std::string, 6>& face_paths);
    
    /**
     * @brief Constructor for loading from cross-layout image.
     * @param cross_layout_path Path to single image with cross layout
     */
    Cubemap(const std::string& cross_layout_path);
    
    /**
     * @brief Constructor for direct data upload.
     * @param face_width Width of each cube face
     * @param face_height Height of each cube face
     * @param face_data Array of 6 pointers to pixel data (RGB or RGBA)
     */
    Cubemap(int face_width, int face_height, const std::array<unsigned char*, 6>& face_data);
    
    // Helper methods
    
    /**
     * @brief Loads a single cubemap face from file using stb_image.
     * @param target OpenGL cubemap face target (GL_TEXTURE_CUBE_MAP_POSITIVE_X + i)
     * @param path File path to the face image
     * @param expected_width Expected width (for validation), or 0 for first face
     * @param expected_height Expected height (for validation), or 0 for first face
     * @param out_width Output parameter for actual width
     * @param out_height Output parameter for actual height
     */
    static void LoadCubemapFace(GLenum target, const std::string& path, 
                                int expected_width, int expected_height,
                                int& out_width, int& out_height);
    
    /**
     * @brief Extracts 6 sub-images from a cross-layout image.
     * @param path Path to the cross-layout image
     * @param face_width Output parameter for face width
     * @param face_height Output parameter for face height
     * @param channels Output parameter for number of color channels
     * @return Array of 6 pointers to extracted face data (caller must free)
     */
    static std::array<unsigned char*, 6> ExtractCrossLayout(
        const std::string& path, int& face_width, int& face_height, int& channels);
    
    /**
     * @brief Generates a cache key from face paths.
     * @param face_paths Array of 6 file paths
     * @return Concatenated string for cache lookup
     */
    static std::string GenerateCacheKey(const std::array<std::string, 6>& face_paths);
    
public:
    // Factory methods
    
    /**
     * @brief Creates a cubemap from 6 individual face files.
     * @param face_paths Array of 6 file paths in OpenGL cubemap order
     * @return Shared pointer to Cubemap (may be cached)
     */
    static CubemapPtr Make(const std::array<std::string, 6>& face_paths);
    
    /**
     * @brief Creates a cubemap from a cross-layout image.
     * @param cross_layout_path Path to single cross-layout image
     * @return Shared pointer to Cubemap (may be cached)
     */
    static CubemapPtr Make(const std::string& cross_layout_path);
    
    /**
     * @brief Creates a cubemap from direct pixel data.
     * @param face_width Width of each cube face
     * @param face_height Height of each cube face
     * @param face_data Array of 6 pointers to pixel data
     * @return Shared pointer to Cubemap (not cached)
     */
    static CubemapPtr Make(int face_width, int face_height, 
                          const std::array<unsigned char*, 6>& face_data);
    
    /**
     * @brief Destructor - automatically cleans up OpenGL resources.
     */
    ~Cubemap();
    
    // ITexture interface implementation
    
    /**
     * @brief Binds the cubemap to the specified texture unit.
     * @param unit Texture unit (0-31)
     */
    void Bind(GLuint unit = 0) const override;
    
    /**
     * @brief Unbinds the cubemap from the specified texture unit.
     * @param unit Texture unit (0-31)
     */
    void Unbind(GLuint unit = 0) const override;
    
    /**
     * @brief Gets the OpenGL texture ID.
     * @return OpenGL texture handle
     */
    GLuint GetTextureID() const override { return m_tid; }
    
    /**
     * @brief Gets the texture target type.
     * @return GL_TEXTURE_CUBE_MAP
     */
    GLenum GetTextureTarget() const override { return GL_TEXTURE_CUBE_MAP; }
    
    // Accessors
    
    /**
     * @brief Gets the width of each cube face.
     * @return Face width in pixels
     */
    int GetFaceWidth() const { return m_face_width; }
    
    /**
     * @brief Gets the height of each cube face.
     * @return Face height in pixels
     */
    int GetFaceHeight() const { return m_face_height; }
};

// ============================================================================
// Implementation (inline for header-only library)
// ============================================================================

// Destructor
inline Cubemap::~Cubemap() {
    if (m_tid != 0) {
        // Only delete if OpenGL context is still valid
        // Use glGetError() to check if OpenGL is still functional
        // This is safer than glfwGetCurrentContext() which requires GLFW to be initialized
        GLenum error = glGetError(); // Clear any existing errors
        glDeleteTextures(1, &m_tid);
        error = glGetError();
        
        // Only report error if it's not GL_INVALID_OPERATION (context destroyed)
        // GL_INVALID_OPERATION (0x0502) means the context is gone, which is fine during shutdown
        if (error != GL_NO_ERROR && error != GL_INVALID_OPERATION) {
            std::cerr << "Warning: Error deleting cubemap texture: 0x" 
                      << std::hex << error << std::dec << std::endl;
        }
    }
}

// Bind method
inline void Cubemap::Bind(GLuint unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    GL_CHECK("activate texture unit for cubemap");
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_tid);
    GL_CHECK("bind cubemap texture");
}

// Unbind method
inline void Cubemap::Unbind(GLuint unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    GL_CHECK("activate texture unit for cubemap unbind");
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    GL_CHECK("unbind cubemap texture");
}

// Generate cache key from face paths
inline std::string Cubemap::GenerateCacheKey(const std::array<std::string, 6>& face_paths) {
    std::string key;
    for (const auto& path : face_paths) {
        key += path + "|";
    }
    return key;
}

// Load a single cubemap face
inline void Cubemap::LoadCubemapFace(GLenum target, const std::string& path,
                                     int expected_width, int expected_height,
                                     int& out_width, int& out_height) {
    int channels;
    stbi_set_flip_vertically_on_load(false); // Cubemaps don't need vertical flip
    unsigned char* data = stbi_load(path.c_str(), &out_width, &out_height, &channels, 0);
    
    if (!data) {
        throw exception::TextureException(
            "Failed to load cubemap face: " + path + " (file not found or invalid format)");
    }
    
    // Validate dimensions if this is not the first face
    if (expected_width > 0 && expected_height > 0) {
        if (out_width != expected_width || out_height != expected_height) {
            stbi_image_free(data);
            throw exception::TextureException(
                "Cubemap face dimension mismatch in " + path + ": expected " +
                std::to_string(expected_width) + "x" + std::to_string(expected_height) +
                ", got " + std::to_string(out_width) + "x" + std::to_string(out_height));
        }
    }
    
    // Validate that face is square
    if (out_width != out_height) {
        stbi_image_free(data);
        throw exception::TextureException(
            "Cubemap faces must be square in " + path + ": got " +
            std::to_string(out_width) + "x" + std::to_string(out_height));
    }
    
    // Determine format based on number of channels
    GLenum internalFormat = GL_RGB8;
    GLenum dataFormat = GL_RGB;

    if (channels == 4) {
        internalFormat = GL_RGBA8;
        dataFormat = GL_RGBA;
    } else if (channels == 3) {
        internalFormat = GL_RGB8;
        dataFormat = GL_RGB;
    } else if (channels == 1) {
        internalFormat = GL_R8;  // Use GL_R8 for modern GL
        dataFormat = GL_RED;
    }

    // --- THE FIX ---
    // Tell OpenGL our data is tightly packed (no 4-byte row alignment)
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Upload texture data to GPU
    // Note: I've separated internalFormat and dataFormat for best practice
    // (This also fixes the potential "all red" issue from my last post)
    glTexImage2D(target, 0, internalFormat, out_width, out_height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
    GL_CHECK("upload cubemap face data");

    // Restore the default alignment for other texture operations in your engine
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    GL_CHECK("upload cubemap face data");
    
    // Free the image data from CPU memory
    stbi_image_free(data);
}

// Constructor for individual face files
inline Cubemap::Cubemap(const std::array<std::string, 6>& face_paths)
    : m_tid(0), m_face_width(0), m_face_height(0) {
    
    // Generate texture ID
    glGenTextures(1, &m_tid);
    GL_CHECK("generate cubemap texture");
    
    if (m_tid == 0) {
        throw exception::TextureException("Failed to generate cubemap texture object");
    }
    
    // Bind the cubemap texture
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_tid);
    GL_CHECK("bind cubemap for configuration");
    
    // Load all 6 faces
    int width = 0, height = 0;
    for (int i = 0; i < 6; i++) {
        GLenum target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + i;
        int face_width, face_height;
        
        // For first face, we don't have expected dimensions yet
        if (i == 0) {
            LoadCubemapFace(target, face_paths[i], 0, 0, face_width, face_height);
            width = face_width;
            height = face_height;
        } else {
            LoadCubemapFace(target, face_paths[i], width, height, face_width, face_height);
        }
    }
    
    m_face_width = width;
    m_face_height = height;
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GL_CHECK("set cubemap texture parameters");
    
    // Unbind
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

// Factory method for individual face files
inline CubemapPtr Cubemap::Make(const std::array<std::string, 6>& face_paths) {
    // Check cache
    std::string cache_key = GenerateCacheKey(face_paths);
    auto it = s_cache.find(cache_key);
    if (it != s_cache.end()) {
        return it->second;
    }
    
    // Create new cubemap
    CubemapPtr new_cubemap = CubemapPtr(new Cubemap(face_paths));
    
    // Store in cache
    s_cache[cache_key] = new_cubemap;
    
    return new_cubemap;
}

// Extract 6 faces from cross-layout image
inline std::array<unsigned char*, 6> Cubemap::ExtractCrossLayout(
    const std::string& path, int& face_width, int& face_height, int& channels) {
    
    int width, height;
    stbi_set_flip_vertically_on_load(false);
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    
    if (!data) {
        throw exception::TextureException(
            "Failed to load cross-layout cubemap: " + path + " (file not found or invalid format)");
    }
    
    // Validate cross-layout dimensions
    // Cross layout should be 4:3 aspect ratio (width = 4*face, height = 3*face)
    if (width % 4 != 0 || height % 3 != 0) {
        stbi_image_free(data);
        throw exception::TextureException(
            "Invalid cross-layout dimensions in " + path + ": " +
            std::to_string(width) + "x" + std::to_string(height) +
            " (must be divisible by 4x3)");
    }
    
    int face_w = width / 4;
    int face_h = height / 3;
    
    if (face_w != face_h) {
        stbi_image_free(data);
        throw exception::TextureException(
            "Cross-layout faces must be square in " + path + ": calculated face size " +
            std::to_string(face_w) + "x" + std::to_string(face_h));
    }
    
    face_width = face_w;
    face_height = face_h;
    
    // Allocate memory for 6 faces
    std::array<unsigned char*, 6> faces;
    size_t face_size = face_w * face_h * channels;
    
    for (int i = 0; i < 6; i++) {
        faces[i] = new unsigned char[face_size];
    }
    
    // Extract faces from cross layout
    // Cross layout:
    //        [Top]
    // [Left] [Front] [Right] [Back]
    //        [Bottom]
    
    // Define face positions in the cross layout (x, y in face units)
    struct FacePos { int x, y; };
    std::array<FacePos, 6> positions = {{
        {2, 1}, // +X (right)
        {0, 1}, // -X (left)
        {1, 0}, // +Y (top)
        {1, 2}, // -Y (bottom)
        {1, 1}, // +Z (front)
        {3, 1}  // -Z (back)
    }};
    
    // Extract each face
    for (int face_idx = 0; face_idx < 6; face_idx++) {
        int start_x = positions[face_idx].x * face_w;
        int start_y = positions[face_idx].y * face_h;
        
        for (int y = 0; y < face_h; y++) {
            for (int x = 0; x < face_w; x++) {
                int src_idx = ((start_y + y) * width + (start_x + x)) * channels;
                int dst_idx = (y * face_w + x) * channels;
                
                for (int c = 0; c < channels; c++) {
                    faces[face_idx][dst_idx + c] = data[src_idx + c];
                }
            }
        }
    }
    
    // Free original image data
    stbi_image_free(data);
    
    return faces;
}

// Constructor for cross-layout image
inline Cubemap::Cubemap(const std::string& cross_layout_path)
    : m_tid(0), m_face_width(0), m_face_height(0) {
    
    // Extract faces from cross layout
    int face_w, face_h, channels;
    auto face_data = ExtractCrossLayout(cross_layout_path, face_w, face_h, channels);
    
    m_face_width = face_w;
    m_face_height = face_h;
    
    // Generate texture ID
    glGenTextures(1, &m_tid);
    GL_CHECK("generate cubemap texture");
    
    if (m_tid == 0) {
        // Clean up face data
        for (auto* data : face_data) {
            delete[] data;
        }
        throw exception::TextureException("Failed to generate cubemap texture object");
    }
    
    // Bind the cubemap texture
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_tid);
    GL_CHECK("bind cubemap for configuration");
    
    // Determine format based on number of channels
    GLenum internalFormat = GL_RGB8;
    GLenum dataFormat = GL_RGB;

    if (channels == 4) {
        internalFormat = GL_RGBA8;
        dataFormat = GL_RGBA;
    } else if (channels == 3) {
        internalFormat = GL_RGB8;
        dataFormat = GL_RGB;
    } else if (channels == 1) {
        internalFormat = GL_R8;
        dataFormat = GL_RED;
    }
    
    // Set pixel alignment for proper texture upload
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    // Upload all 6 faces
    for (int i = 0; i < 6; i++) {
        GLenum target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + i;
        glTexImage2D(target, 0, internalFormat, face_w, face_h, 0, dataFormat, GL_UNSIGNED_BYTE, face_data[i]);
        GL_CHECK("upload cubemap face from cross-layout");
    }
    
    // Restore default alignment
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GL_CHECK("set cubemap texture parameters");
    
    // Unbind
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    
    // Clean up face data
    for (auto* data : face_data) {
        delete[] data;
    }
}

// Factory method for cross-layout image
inline CubemapPtr Cubemap::Make(const std::string& cross_layout_path) {
    // Check cache
    auto it = s_cache.find(cross_layout_path);
    if (it != s_cache.end()) {
        return it->second;
    }
    
    // Create new cubemap
    CubemapPtr new_cubemap = CubemapPtr(new Cubemap(cross_layout_path));
    
    // Store in cache
    s_cache[cross_layout_path] = new_cubemap;
    
    return new_cubemap;
}

// Constructor for direct data upload
inline Cubemap::Cubemap(int face_width, int face_height, 
                       const std::array<unsigned char*, 6>& face_data)
    : m_tid(0), m_face_width(face_width), m_face_height(face_height) {
    
    // Validate dimensions
    if (face_width <= 0 || face_height <= 0) {
        throw exception::TextureException(
            "Invalid cubemap face dimensions: " +
            std::to_string(face_width) + "x" + std::to_string(face_height));
    }
    
    if (face_width != face_height) {
        throw exception::TextureException(
            "Cubemap faces must be square: got " +
            std::to_string(face_width) + "x" + std::to_string(face_height));
    }
    
    // Validate that all face data pointers are valid
    for (int i = 0; i < 6; i++) {
        if (face_data[i] == nullptr) {
            throw exception::TextureException(
                "Null data pointer for cubemap face " + std::to_string(i));
        }
    }
    
    // Generate texture ID
    glGenTextures(1, &m_tid);
    GL_CHECK("generate cubemap texture");
    
    if (m_tid == 0) {
        throw exception::TextureException("Failed to generate cubemap texture object");
    }
    
    // Bind the cubemap texture
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_tid);
    GL_CHECK("bind cubemap for configuration");
    
    // Upload all 6 faces
    // Assume RGB format for direct data
    GLenum format = GL_RGB;
    for (int i = 0; i < 6; i++) {
        GLenum target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + i;
        glTexImage2D(target, 0, format, face_width, face_height, 0, 
                     format, GL_UNSIGNED_BYTE, face_data[i]);
        GL_CHECK("upload cubemap face from direct data");
    }
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GL_CHECK("set cubemap texture parameters");
    
    // Unbind
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

// Factory method for direct data upload
inline CubemapPtr Cubemap::Make(int face_width, int face_height,
                               const std::array<unsigned char*, 6>& face_data) {
    // Direct data cubemaps are NOT cached (data is transient)
    return CubemapPtr(new Cubemap(face_width, face_height, face_data));
}

} // namespace texture

#endif // CUBEMAP_H
