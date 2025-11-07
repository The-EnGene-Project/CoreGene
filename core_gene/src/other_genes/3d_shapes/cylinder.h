#ifndef CYLINDER_H
#define CYLINDER_H
#pragma once

#include <memory>
#include <vector>
#include <cmath>
#include <glm/glm.hpp>

#include "../../gl_base/geometry.h"
#include "../grid.h"

#define PI 3.14159265359f

class Cylinder;
using CylinderPtr = std::shared_ptr<Cylinder>;

/**
 * @class Cylinder
 * @brief Generates a 3D cylindrical mesh with optional top and bottom caps.
 *
 * Each vertex contains position (3), normal (3), tangent (3), and UV (2).
 */
class Cylinder : public geometry::Geometry {
protected:
    // Protected constructor receives pre-generated arrays
    Cylinder(float* vertices, unsigned int* indices, int nverts, int nindices)
        : geometry::Geometry(vertices, indices, nverts, nindices, 3, {3,3,2})
    {}

    // Mesh generation functions
    static float* generateVertexData(
        float radius,
        float height,
        int radialSegments,
        int heightSegments,
        bool withCaps,
        int& outVertexCount
    );

    static unsigned int* generateIndices(
        int radialSegments,
        int heightSegments,
        bool withCaps,
        int& outIndexCount
    );

public:
    // Factory method to create a Cylinder
    static CylinderPtr Make(
        float radius = 1.0f,
        float height = 1.0f,
        int radialSegments = 32,
        int heightSegments = 1,
        bool withCaps = true
    ) {
        // Input validation
        if (radius <= 0.0f) radius = 0.01f;
        if (height <= 0.0f) height = 0.01f;
        if (radialSegments < 3) radialSegments = 3;
        if (heightSegments < 1) heightSegments = 1;
        
        int nverts = 0;
        int nindices = 0;
        float* vertices = generateVertexData(radius, height, radialSegments, heightSegments, withCaps, nverts);
        unsigned int* indices = generateIndices(radialSegments, heightSegments, withCaps, nindices);

        return CylinderPtr(new Cylinder(vertices, indices, nverts, nindices));
    }

    virtual ~Cylinder() {}
};

// Implementation of generateVertexData
inline float* Cylinder::generateVertexData(
    float radius,
    float height,
    int radialSegments,
    int heightSegments,
    bool withCaps,
    int& outVertexCount
) {
    // Input validation
    if (radius <= 0.0f) radius = 0.01f;
    if (height <= 0.0f) height = 0.01f;
    if (radialSegments < 3) radialSegments = 3;
    if (heightSegments < 1) heightSegments = 1;
    
    // Create parametric UV grid for the cylindrical side surface
    GridPtr grid = Grid::Make(radialSegments, heightSegments);
    const float* texcoord = grid->GetCoords();
    int sideVertexCount = grid->VertexCount();
    
    // Calculate total vertex count
    int capVertexCount = 0;
    if (withCaps) {
        // Each cap has: 1 center vertex + (radialSegments + 1) perimeter vertices
        capVertexCount = 2 * (1 + radialSegments + 1);
    }
    
    outVertexCount = sideVertexCount + capVertexCount;
    
    const int floatsPerVertex = 3 + 3 + 3 + 2; // position + normal + tangent + uv
    float* vertices = new float[outVertexCount * floatsPerVertex];
    
    // Generate side surface vertices using cylindrical coordinates
    int idx = 0;
    for (int i = 0; i < sideVertexCount; ++i) {
        float u = texcoord[2*i];     // wraps around circumference [0,1]
        float v = texcoord[2*i + 1]; // goes from bottom to top [0,1]
        
        // Convert UV to cylindrical coordinates
        float theta = u * 2.0f * PI;
        
        // Position: cylindrical coordinates
        glm::vec3 pos(
            radius * cosf(theta),  // x
            v * height,            // y
            radius * sinf(theta)   // z
        );
        
        // Normal: points radially outward (normalized)
        glm::vec3 normal = glm::normalize(glm::vec3(pos.x, 0.0f, pos.z));
        
        // Tangent: perpendicular to normal, along circumference
        glm::vec3 tangent(-sinf(theta), 0.0f, cosf(theta));
        
        // UV coordinates from grid
        glm::vec2 uv(u, v);
        
        // Write vertex data
        vertices[idx++] = pos.x; vertices[idx++] = pos.y; vertices[idx++] = pos.z;
        vertices[idx++] = normal.x; vertices[idx++] = normal.y; vertices[idx++] = normal.z;
        vertices[idx++] = tangent.x; vertices[idx++] = tangent.y; vertices[idx++] = tangent.z;
        vertices[idx++] = uv.x; vertices[idx++] = uv.y;
    }
    
    // Generate cap vertices if requested
    if (withCaps) {
        // Bottom cap: center vertex at (0, 0, 0) with normal (0, -1, 0)
        glm::vec3 bottomCenter(0.0f, 0.0f, 0.0f);
        glm::vec3 bottomNormal(0.0f, -1.0f, 0.0f);
        glm::vec3 bottomTangent(1.0f, 0.0f, 0.0f); // Tangent along X-axis
        glm::vec2 bottomCenterUV(0.5f, 0.5f); // Center of UV space
        
        vertices[idx++] = bottomCenter.x; vertices[idx++] = bottomCenter.y; vertices[idx++] = bottomCenter.z;
        vertices[idx++] = bottomNormal.x; vertices[idx++] = bottomNormal.y; vertices[idx++] = bottomNormal.z;
        vertices[idx++] = bottomTangent.x; vertices[idx++] = bottomTangent.y; vertices[idx++] = bottomTangent.z;
        vertices[idx++] = bottomCenterUV.x; vertices[idx++] = bottomCenterUV.y;
        
        // Bottom cap: radial vertices at y=0
        for (int i = 0; i <= radialSegments; ++i) {
            float theta = (float)i * 2.0f * PI / (float)radialSegments;
            
            glm::vec3 pos(
                radius * cosf(theta),
                0.0f,
                radius * sinf(theta)
            );
            
            glm::vec3 normal(0.0f, -1.0f, 0.0f);
            glm::vec3 tangent(cosf(theta), 0.0f, sinf(theta)); // Radial direction as tangent
            
            // Radial UV mapping: map from center (0.5, 0.5) to edge
            glm::vec2 uv(
                0.5f + 0.5f * cosf(theta),
                0.5f + 0.5f * sinf(theta)
            );
            
            vertices[idx++] = pos.x; vertices[idx++] = pos.y; vertices[idx++] = pos.z;
            vertices[idx++] = normal.x; vertices[idx++] = normal.y; vertices[idx++] = normal.z;
            vertices[idx++] = tangent.x; vertices[idx++] = tangent.y; vertices[idx++] = tangent.z;
            vertices[idx++] = uv.x; vertices[idx++] = uv.y;
        }
        
        // Top cap: center vertex at (0, height, 0) with normal (0, 1, 0)
        glm::vec3 topCenter(0.0f, height, 0.0f);
        glm::vec3 topNormal(0.0f, 1.0f, 0.0f);
        glm::vec3 topTangent(1.0f, 0.0f, 0.0f); // Tangent along X-axis
        glm::vec2 topCenterUV(0.5f, 0.5f); // Center of UV space
        
        vertices[idx++] = topCenter.x; vertices[idx++] = topCenter.y; vertices[idx++] = topCenter.z;
        vertices[idx++] = topNormal.x; vertices[idx++] = topNormal.y; vertices[idx++] = topNormal.z;
        vertices[idx++] = topTangent.x; vertices[idx++] = topTangent.y; vertices[idx++] = topTangent.z;
        vertices[idx++] = topCenterUV.x; vertices[idx++] = topCenterUV.y;
        
        // Top cap: radial vertices at y=height
        for (int i = 0; i <= radialSegments; ++i) {
            float theta = (float)i * 2.0f * PI / (float)radialSegments;
            
            glm::vec3 pos(
                radius * cosf(theta),
                height,
                radius * sinf(theta)
            );
            
            glm::vec3 normal(0.0f, 1.0f, 0.0f);
            glm::vec3 tangent(cosf(theta), 0.0f, sinf(theta)); // Radial direction as tangent
            
            // Radial UV mapping: map from center (0.5, 0.5) to edge
            glm::vec2 uv(
                0.5f + 0.5f * cosf(theta),
                0.5f + 0.5f * sinf(theta)
            );
            
            vertices[idx++] = pos.x; vertices[idx++] = pos.y; vertices[idx++] = pos.z;
            vertices[idx++] = normal.x; vertices[idx++] = normal.y; vertices[idx++] = normal.z;
            vertices[idx++] = tangent.x; vertices[idx++] = tangent.y; vertices[idx++] = tangent.z;
            vertices[idx++] = uv.x; vertices[idx++] = uv.y;
        }
    }
    
    return vertices;
}

// Implementation of generateIndices
inline unsigned int* Cylinder::generateIndices(
    int radialSegments,
    int heightSegments,
    bool withCaps,
    int& outIndexCount
) {
    // Input validation (match generateVertexData)
    if (radialSegments < 3) radialSegments = 3;
    if (heightSegments < 1) heightSegments = 1;
    
    // Generate grid indices for the cylindrical side surface
    GridPtr grid = Grid::Make(radialSegments, heightSegments);
    int sideIndexCount = grid->IndexCount();
    const unsigned int* gridIndices = grid->GetIndices();
    
    // Calculate total index count
    int capIndexCount = 0;
    if (withCaps) {
        // Each cap uses triangle fan: radialSegments triangles * 3 indices per triangle
        capIndexCount = 2 * radialSegments * 3;
    }
    
    outIndexCount = sideIndexCount + capIndexCount;
    
    unsigned int* indices = new unsigned int[outIndexCount];
    
    // Copy side surface indices from grid
    int idx = 0;
    for (int i = 0; i < sideIndexCount; ++i) {
        indices[idx++] = gridIndices[i];
    }
    
    // Generate cap indices if requested
    if (withCaps) {
        int sideVertexCount = grid->VertexCount();
        
        // Bottom cap indices (triangle fan)
        // Center vertex is at index: sideVertexCount
        // Perimeter vertices start at: sideVertexCount + 1
        unsigned int bottomCenterIdx = sideVertexCount;
        unsigned int bottomPerimeterStart = sideVertexCount + 1;
        
        for (int i = 0; i < radialSegments; ++i) {
            // Triangle fan: center -> current perimeter vertex -> next perimeter vertex
            // Winding order: counter-clockwise when viewed from below (normal points down)
            indices[idx++] = bottomCenterIdx;
            indices[idx++] = bottomPerimeterStart + i + 1;
            indices[idx++] = bottomPerimeterStart + i;
        }
        
        // Top cap indices (triangle fan)
        // Center vertex is at: sideVertexCount + 1 (bottom center) + (radialSegments + 1) (bottom perimeter)
        // Perimeter vertices start at: topCenterIdx + 1
        unsigned int topCenterIdx = sideVertexCount + 1 + (radialSegments + 1);
        unsigned int topPerimeterStart = topCenterIdx + 1;
        
        for (int i = 0; i < radialSegments; ++i) {
            // Triangle fan: center -> current perimeter vertex -> next perimeter vertex
            // Winding order: counter-clockwise when viewed from above (normal points up)
            indices[idx++] = topCenterIdx;
            indices[idx++] = topPerimeterStart + i;
            indices[idx++] = topPerimeterStart + i + 1;
        }
    }
    
    return indices;
}

#endif // CYLINDER_H
