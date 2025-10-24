#ifndef CUBE_H
#define CUBE_H
#pragma once

#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "../../gl_base/geometry.h"

class Cube;
using CubePtr = std::shared_ptr<Cube>;

class Cube : public geometry::Geometry {
protected:
    Cube(float* vertices, unsigned int* indices, int nverts, int nindices)
        : geometry::Geometry(vertices, indices, nverts, nindices, 3, {3,3,2})
    {}

    // Gera vértices: posição, normal, tangente, uv
    static float* generateVertexData(int& outVertexCount) {
        outVertexCount = 24; // 4 vértices por face * 6 faces
        const int floatsPerVertex = 3 + 3 + 3 + 2;
        float* vertices = new float[outVertexCount * floatsPerVertex];

        // Posições por face
        glm::vec3 positions[24] = {
            {-0.5f,0.0f,-0.5f},{-0.5f,1.0f,-0.5f},{0.5f,1.0f,-0.5f},{0.5f,0.0f,-0.5f}, // back
            {-0.5f,0.0f,0.5f},{0.5f,0.0f,0.5f},{0.5f,1.0f,0.5f},{-0.5f,1.0f,0.5f},    // front
            {-0.5f,0.0f,-0.5f},{-0.5f,0.0f,0.5f},{-0.5f,1.0f,0.5f},{-0.5f,1.0f,-0.5f}, // left
            {0.5f,0.0f,-0.5f},{0.5f,1.0f,-0.5f},{0.5f,1.0f,0.5f},{0.5f,0.0f,0.5f},     // right
            {-0.5f,1.0f,-0.5f},{-0.5f,1.0f,0.5f},{0.5f,1.0f,0.5f},{0.5f,1.0f,-0.5f},   // top
            {-0.5f,0.0f,-0.5f},{0.5f,0.0f,-0.5f},{0.5f,0.0f,0.5f},{-0.5f,0.0f,0.5f}    // bottom
        };

        glm::vec3 normals[24] = {
            {0,0,-1},{0,0,-1},{0,0,-1},{0,0,-1},
            {0,0,1},{0,0,1},{0,0,1},{0,0,1},
            {-1,0,0},{-1,0,0},{-1,0,0},{-1,0,0},
            {1,0,0},{1,0,0},{1,0,0},{1,0,0},
            {0,1,0},{0,1,0},{0,1,0},{0,1,0},
            {0,-1,0},{0,-1,0},{0,-1,0},{0,-1,0}
        };

        glm::vec3 tangents[24] = {
            {-1,0,0},{-1,0,0},{-1,0,0},{-1,0,0},
            {1,0,0},{1,0,0},{1,0,0},{1,0,0},
            {0,1,0},{0,1,0},{0,1,0},{0,1,0},
            {0,-1,0},{0,-1,0},{0,-1,0},{0,-1,0},
            {1,0,0},{1,0,0},{1,0,0},{1,0,0},
            {-1,0,0},{-1,0,0},{-1,0,0},{-1,0,0}
        };

        glm::vec2 uvs[24] = {
            {0,0},{1,0},{1,1},{0,1},
            {0,0},{1,0},{1,1},{0,1},
            {0,0},{1,0},{1,1},{0,1},
            {0,0},{1,0},{1,1},{0,1},
            {0,0},{1,0},{1,1},{0,1},
            {0,0},{1,0},{1,1},{0,1}
        };

        int idx = 0;
        for(int i=0;i<24;i++){
            vertices[idx++] = positions[i].x;
            vertices[idx++] = positions[i].y;
            vertices[idx++] = positions[i].z;

            vertices[idx++] = normals[i].x;
            vertices[idx++] = normals[i].y;
            vertices[idx++] = normals[i].z;

            vertices[idx++] = tangents[i].x;
            vertices[idx++] = tangents[i].y;
            vertices[idx++] = tangents[i].z;

            vertices[idx++] = uvs[i].x;
            vertices[idx++] = uvs[i].y;
        }

        return vertices;
    }

    static unsigned int* generateIndices(int& outIndexCount) {
        outIndexCount = 36; // 6 faces * 2 triângulos * 3 vertices
        unsigned int* indices = new unsigned int[outIndexCount]{
            0,1,2,0,2,3,
            4,5,6,4,6,7,
            8,9,10,8,10,11,
            12,13,14,12,14,15,
            16,17,18,16,18,19,
            20,21,22,20,22,23
        };
        return indices;
    }

public:
    static CubePtr Make() {
        int nverts = 0;
        int nindices = 0;
        float* vertices = generateVertexData(nverts);
        unsigned int* indices = generateIndices(nindices);
        return CubePtr(new Cube(vertices, indices, nverts, nindices));
    }

    virtual ~Cube() {}
};

#endif
