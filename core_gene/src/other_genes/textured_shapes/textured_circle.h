#ifndef TEXTURED_CIRCLE_H
#define TEXTURED_CIRCLE_H
#pragma once

#include <memory>
#include <vector>
#include <cmath>
#include "../../gl_base/geometry.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Forward declaration
class TexturedCircle;
using TexturedCirclePtr = std::shared_ptr<TexturedCircle>;

/**
 * @class TexturedCircle
 * @brief A factory for creating a 2D circle geometry with texture coordinates.
 *
 * This class generates a fan of triangles to approximate a circle. It automatically
 * calculates UV coordinates to map a square texture onto the circle's area,
 * inheriting directly from the base Geometry class. The portion of the texture
 * used can be customized, and the mapping can be mirrored.
 */
class TexturedCircle : public geometry::Geometry {
private:
    /**
     * @brief Generates the interleaved vertex data (position and UVs).
     * @param x_center The x-coordinate of the circle's center.
     * @param y_center The y-coordinate of the circle's center.
     * @param radius The radius of the circle.
     * @param edge_points The number of points on the circumference (discretization level).
     * @param uv_center_x The u-coordinate for the circle's center in texture space.
     * @param uv_center_y The v-coordinate for the circle's center in texture space.
     * @param uv_radius The radius of the texture portion to map, in texture space units.
     * @param mirror_texture If true, mirrors the texture mapping horizontally.
     * @return A dynamically allocated float array of vertex data.
     */
    static float* generateVertexData(float x_center, float y_center, float radius, unsigned int edge_points, float uv_center_x, float uv_center_y, float uv_radius, bool mirror_texture) {
        int num_vertices = edge_points + 1; // +1 for the center vertex
        // Each vertex has 2 position floats and 2 UV floats
        float* vertices = new float[num_vertices * 4];

        // Center Vertex (position and UV)
        vertices[0] = x_center;
        vertices[1] = y_center;
        vertices[2] = uv_center_x; // u
        vertices[3] = uv_center_y; // v

        float angle_step = 2.0f * static_cast<float>(M_PI) / edge_points;

        // Edge Vertices
        for (unsigned int i = 0; i < edge_points; i++) {
            float angle = i * angle_step;
            
            // Calculate vertex index in the array
            int v_idx = (i + 1) * 4;

            // Position
            vertices[v_idx + 0] = x_center + radius * cos(angle);
            vertices[v_idx + 1] = y_center + radius * sin(angle);

            // UV coordinates are calculated based on the specified texture region.
            float u_offset = cos(angle);
            if (mirror_texture) {
                u_offset = -u_offset; // Invert the horizontal component for mirroring
            }

            vertices[v_idx + 2] = (u_offset * uv_radius) + uv_center_x; // u
            vertices[v_idx + 3] = (sin(angle) * uv_radius) + uv_center_y; // v
        }
        return vertices;
    }

    /**
     * @brief Generates the EBO indices for a triangle fan circle.
     * @param edge_points The number of points on the circumference.
     * @return A dynamically allocated unsigned int array of indices.
     */
    static unsigned int* generateIndices(unsigned int edge_points) {
        // We create 'edge_points' number of triangles.
        unsigned int* indices = new unsigned int[edge_points * 3];

        for (unsigned int i = 0; i < edge_points; i++) {
            indices[i * 3 + 0] = 0;                      // Center vertex
            indices[i * 3 + 1] = i + 1;                  // Current edge vertex
            indices[i * 3 + 2] = (i + 1) % edge_points + 1; // Next edge vertex, wraps around
        }
        return indices;
    }

protected:
    TexturedCircle(float x_center, float y_center, float radius, unsigned int edge_points, float uv_center_x, float uv_center_y, float uv_radius, bool mirror_texture) :
        geometry::Geometry(
            generateVertexData(x_center, y_center, radius, edge_points, uv_center_x, uv_center_y, uv_radius, mirror_texture),
            generateIndices(edge_points),
            edge_points + 1,        // Number of vertices
            edge_points * 3,        // Number of indices
            2,                      // 2D positions
            {2}                     // One extra attribute (UVs) of size 2
        )
    {}

public:
    /**
     * @brief Factory function to create a new TexturedCircle.
     * @param x_center The x-coordinate of the circle's center.
     * @param y_center The y-coordinate of the circle's center.
     * @param radius The radius of the circle.
     * @param edge_points The number of segments to approximate the circle.
     * @param uv_center_x [Optional] The u-coordinate (horizontal) of the center of the texture region to apply. Defaults to 0.5.
     * @param uv_center_y [Optional] The v-coordinate (vertical) of the center of the texture region to apply. Defaults to 0.5.
     * @param uv_radius [Optional] The radius of the circular texture region to apply, in texture space units. Defaults to 0.5 (the full texture).
     * @param mirror_texture [Optional] If true, mirrors the texture mapping horizontally. Defaults to false.
     * @return A shared pointer to the newly created TexturedCircle.
     */
    static TexturedCirclePtr Make(float x_center, float y_center, float radius, unsigned int edge_points, 
                                  float uv_center_x = 0.5f, float uv_center_y = 0.5f, float uv_radius = 0.5f,
                                  bool mirror_texture = false) {
        return TexturedCirclePtr(new TexturedCircle(x_center, y_center, radius, edge_points, uv_center_x, uv_center_y, uv_radius, mirror_texture));
    }

    virtual ~TexturedCircle() {}
};

#endif // TEXTURED_CIRCLE_H

