#ifndef SKYBOX_CUBE_H
#define SKYBOX_CUBE_H
#pragma once

#include "geometry.h"
#include <memory>

namespace geometry {

/**
 * @brief Vertex shader source for skybox rendering.
 * 
 * This shader transforms the skybox cube vertices and passes position as texture coordinates.
 * The z-coordinate is set to w to ensure the skybox is always at maximum depth.
 */
inline constexpr const char* SKYBOX_VERTEX_SHADER = R"(
#version 430 core

layout(location = 0) in vec3 a_position;

out vec3 v_texCoords;

uniform mat4 u_viewProjection;

void main() {
    vec4 pos = u_viewProjection * vec4(a_position, 1.0);
    
    // Set z = w to ensure skybox is always at maximum depth
    gl_Position = pos.xyww;
    
    // Use local position as texture coordinate
    v_texCoords = a_position;
}
)";

/**
 * @brief Fragment shader source for skybox rendering.
 * 
 * This shader samples the cubemap texture using the interpolated position as texture coordinates.
 * Includes coordinate system conversion for EnGene's coordinate system.
 */
inline constexpr const char* SKYBOX_FRAGMENT_SHADER = R"(
#version 430 core

in vec3 v_texCoords;
out vec4 FragColor;

uniform samplerCube u_skybox;

void main() {
    // Coordinate system conversion for EnGene (if needed)
    vec3 texCoords = vec3(v_texCoords.x, v_texCoords.y, -v_texCoords.z);
    
    FragColor = texture(u_skybox, texCoords);
}
)";

class SkyboxCube;
using SkyboxCubePtr = std::shared_ptr<SkyboxCube>;

/**
 * @class SkyboxCube
 * @brief Specialized cube geometry for skybox rendering.
 * 
 * This class provides a cube mesh optimized for skybox rendering. Unlike standard cubes,
 * it only contains position data (no normals, tangents, or UVs) since the vertex positions
 * are used directly as cubemap texture coordinates in the skybox shader.
 * 
 * The cube is centered at the origin and sized appropriately to encompass the view frustum.
 * During rendering, the skybox component will position this cube at the camera's location.
 * 
 * @note The vertex positions serve dual purpose: geometry position and cubemap sampling direction.
 * @note This class follows the "other genes" philosophy but is placed in gl_base/ since it's
 *       a fundamental primitive for the skybox system.
 */
class SkyboxCube : public Geometry {
protected:
    /**
     * @brief Protected constructor for RAII pattern.
     * @param vertices Array of vertex position data (x, y, z per vertex).
     * @param indices Array of triangle indices.
     * @param nverts Number of vertices (should be 36 for cube with 6 faces).
     * @param nindices Number of indices (should be 36 for 12 triangles).
     */
    SkyboxCube(float* vertices, unsigned int* indices, int nverts, int nindices)
        : Geometry(vertices, indices, nverts, nindices, 3, {}) // 3 floats per position, no additional attributes
    {}

    /**
     * @brief Generates vertex data for a skybox cube.
     * 
     * Creates a cube with 36 vertices (6 faces * 2 triangles * 3 vertices each).
     * Each vertex contains only position data (x, y, z).
     * The cube is centered at origin with size 1.0 (extends from -0.5 to +0.5 on each axis).
     * 
     * @param outVertexCount Output parameter receiving the number of vertices generated.
     * @return Pointer to dynamically allocated vertex data array.
     */
    static float* generateVertexData(int& outVertexCount) {
        outVertexCount = 36; // 6 faces * 2 triangles * 3 vertices
        const int floatsPerVertex = 3; // Only position (x, y, z)
        float* vertices = new float[outVertexCount * floatsPerVertex];

        // Cube vertices for each face (centered at origin, size 1.0)
        // Each face is defined with two triangles (6 vertices per face)
        // Positions are used directly as cubemap texture coordinates
        float cubeVertices[] = {
            // Back face (negative Z)
            -0.5f, -0.5f, -0.5f,
             0.5f, -0.5f, -0.5f,
             0.5f,  0.5f, -0.5f,
             0.5f,  0.5f, -0.5f,
            -0.5f,  0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,

            // Front face (positive Z)
            -0.5f, -0.5f,  0.5f,
             0.5f,  0.5f,  0.5f,
             0.5f, -0.5f,  0.5f,
            -0.5f, -0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,
             0.5f,  0.5f,  0.5f,

            // Left face (negative X)
            -0.5f,  0.5f,  0.5f,
            -0.5f, -0.5f, -0.5f,
            -0.5f,  0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,
            -0.5f,  0.5f,  0.5f,
            -0.5f, -0.5f,  0.5f,

            // Right face (positive X)
             0.5f,  0.5f,  0.5f,
             0.5f,  0.5f, -0.5f,
             0.5f, -0.5f, -0.5f,
             0.5f, -0.5f, -0.5f,
             0.5f, -0.5f,  0.5f,
             0.5f,  0.5f,  0.5f,

            // Top face (positive Y)
            -0.5f,  0.5f, -0.5f,
             0.5f,  0.5f,  0.5f,
             0.5f,  0.5f, -0.5f,
            -0.5f,  0.5f, -0.5f,
            -0.5f,  0.5f,  0.5f,
             0.5f,  0.5f,  0.5f,

            // Bottom face (negative Y)
            -0.5f, -0.5f, -0.5f,
             0.5f, -0.5f, -0.5f,
             0.5f, -0.5f,  0.5f,
             0.5f, -0.5f,  0.5f,
            -0.5f, -0.5f,  0.5f,
            -0.5f, -0.5f, -0.5f
        };

        // Copy vertex data
        for (int i = 0; i < outVertexCount * floatsPerVertex; i++) {
            vertices[i] = cubeVertices[i];
        }

        return vertices;
    }

    /**
     * @brief Generates index data for a skybox cube.
     * 
     * Creates indices for 36 vertices (already arranged as triangles).
     * Since we're using non-indexed drawing with pre-arranged triangles,
     * the indices are simply sequential: 0, 1, 2, 3, 4, 5, ...
     * 
     * @param outIndexCount Output parameter receiving the number of indices generated.
     * @return Pointer to dynamically allocated index data array.
     */
    static unsigned int* generateIndices(int& outIndexCount) {
        outIndexCount = 36; // 6 faces * 2 triangles * 3 vertices
        unsigned int* indices = new unsigned int[outIndexCount];
        
        // Sequential indices since vertices are already arranged as triangles
        for (int i = 0; i < outIndexCount; i++) {
            indices[i] = i;
        }
        
        return indices;
    }

public:
    /**
     * @brief Factory method to create a skybox cube.
     * 
     * Creates a cube geometry optimized for skybox rendering. The cube is centered
     * at the origin with unit size. During rendering, the skybox component will
     * position this at the camera location and scale it appropriately.
     * 
     * @return Shared pointer to the created SkyboxCube instance.
     * 
     * @note The cube size is 1.0 (from -0.5 to +0.5 on each axis). The skybox shader
     *       will handle proper positioning and depth to ensure it always appears at
     *       maximum distance.
     */
    static SkyboxCubePtr Make() {
        int nverts = 0;
        int nindices = 0;
        float* vertices = generateVertexData(nverts);
        unsigned int* indices = generateIndices(nindices);
        return SkyboxCubePtr(new SkyboxCube(vertices, indices, nverts, nindices));
    }

    /**
     * @brief Virtual destructor for proper cleanup.
     */
    virtual ~SkyboxCube() {}
};

} // namespace geometry

#endif // SKYBOX_CUBE_H
