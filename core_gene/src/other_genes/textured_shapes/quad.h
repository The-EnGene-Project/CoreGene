#ifndef QUAD_H
#define QUAD_H
#pragma once

#include <memory>
#include <vector>
#include <algorithm>
#include "../../gl_base/geometry.h"

// Forward declaration
class Quad;
using QuadPtr = std::shared_ptr<Quad>;

/**
 * @class Quad
 * @brief A factory for creating a 2D rectangle geometry with texture coordinates.
 *
 * This class generates the vertices and indices for a quad defined by two
 * opposite corner points in world space. It also allows specifying a rectangular
 * region in texture space (UVs) to map onto the quad, with options to mirror
 * the texture on either axis.
 */
class Quad : public geometry::Geometry {
private:
    /**
     * @brief Generates the interleaved vertex data (position and UVs).
     * @param x1 The x-coordinate of the first world space corner.
     * @param y1 The y-coordinate of the first world space corner.
     * @param x2 The x-coordinate of the second world space corner.
     * @param y2 The y-coordinate of the second world space corner.
     * @param uv_x1 The u-coordinate of the first texture space corner.
     * @param uv_y1 The v-coordinate of the first texture space corner.
     * @param uv_x2 The u-coordinate of the second texture space corner.
     * @param uv_y2 The v-coordinate of the second texture space corner.
     * @param mirror_u If true, mirrors the texture horizontally.
     * @param mirror_v If true, mirrors the texture vertically.
     * @return A dynamically allocated float array of vertex data.
     */
    static float* generateVertexData(float x1, float y1, float x2, float y2, float uv_x1, float uv_y1, float uv_x2, float uv_y2, bool mirror_u, bool mirror_v) {
        // 4 vertices, each with 2 pos floats and 2 uv floats. Total = 16 floats.
        float* vertices = new float[4 * 4];

        // Determine the corners for world positions
        float minX = std::min(x1, x2);
        float minY = std::min(y1, y2);
        float maxX = std::max(x1, x2);
        float maxY = std::max(y1, y2);

        // Determine the corners for texture coordinates
        float minU = std::min(uv_x1, uv_x2);
        float minV = std::min(uv_y1, uv_y2);
        float maxU = std::max(uv_x1, uv_x2);
        float maxV = std::max(uv_y1, uv_y2);

        // Adjust UVs based on mirroring flags
        float u_left  = mirror_u ? maxU : minU;
        float u_right = mirror_u ? minU : maxU;
        float v_bottom = mirror_v ? maxV : minV;
        float v_top    = mirror_v ? minV : maxV;

        // Vertex 0: Bottom-Left
        vertices[0] = minX; vertices[1] = minY;    // Position
        vertices[2] = u_left; vertices[3] = v_bottom; // UV

        // Vertex 1: Bottom-Right
        vertices[4] = maxX; vertices[5] = minY;    // Position
        vertices[6] = u_right; vertices[7] = v_bottom; // UV

        // Vertex 2: Top-Right
        vertices[8] = maxX; vertices[9] = maxY;    // Position
        vertices[10] = u_right; vertices[11] = v_top;    // UV

        // Vertex 3: Top-Left
        vertices[12] = minX; vertices[13] = maxY;    // Position
        vertices[14] = u_left; vertices[15] = v_top;    // UV

        return vertices;
    }

    /**
     * @brief Generates the element buffer object indices for a quad.
     * @return A dynamically allocated unsigned int array of indices.
     */
    static unsigned int* generateIndices() {
        // 2 triangles, 3 indices per triangle
        unsigned int* indices = new unsigned int[6];

        // First triangle: bottom-left -> bottom-right -> top-right
        indices[0] = 0;
        indices[1] = 1;
        indices[2] = 2;

        // Second triangle: bottom-left -> top-right -> top-left
        indices[3] = 0;
        indices[4] = 2;
        indices[5] = 3;

        return indices;
    }

protected:
    Quad(float x1, float y1, float x2, float y2, float uv_x1, float uv_y1, float uv_x2, float uv_y2, bool mirror_u, bool mirror_v) :
        geometry::Geometry(
            generateVertexData(x1, y1, x2, y2, uv_x1, uv_y1, uv_x2, uv_y2, mirror_u, mirror_v),
            generateIndices(),
            4,      // 4 vertices
            6,      // 6 indices
            2,      // 2D positions
            {2}     // One extra attribute (UVs) of size 2
        )
    {}

public:
    /**
     * @brief Factory function to create a new Quad.
     * @param x1 The x-coordinate of the first world space corner.
     * @param y1 The y-coordinate of the first world space corner.
     * @param x2 The x-coordinate of the second world space corner.
     * @param y2 The y-coordinate of the second world space corner.
     * @param uv_x1 [Optional] The u-coordinate of the first texture space corner. Defaults to 0.0.
     * @param uv_y1 [Optional] The v-coordinate of the first texture space corner. Defaults to 0.0.
     * @param uv_x2 [Optional] The u-coordinate of the second texture space corner. Defaults to 1.0.
     * @param uv_y2 [Optional] The v-coordinate of the second texture space corner. Defaults to 1.0.
     * @param mirror_u [Optional] If true, mirrors the texture horizontally (flips U coords). Defaults to false.
     * @param mirror_v [Optional] If true, mirrors the texture vertically (flips V coords). Defaults to false.
     * @return A shared pointer to the newly created Quad.
     */
    static QuadPtr Make(float x1, float y1, float x2, float y2, 
                        float uv_x1 = 0.0f, float uv_y1 = 0.0f, float uv_x2 = 1.0f, float uv_y2 = 1.0f,
                        bool mirror_u = false, bool mirror_v = false) {
        return QuadPtr(new Quad(x1, y1, x2, y2, uv_x1, uv_y1, uv_x2, uv_y2, mirror_u, mirror_v));
    }

    virtual ~Quad() {}
};

#endif // QUAD_H

