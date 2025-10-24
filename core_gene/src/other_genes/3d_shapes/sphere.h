#ifndef SPHERE_H
#define SPHERE_H
#pragma once

#include <memory>
#include <vector>
#include <cmath>
#include <glm/glm.hpp>

#include "../../gl_base/geometry.h"
#include "../grid.h"

#define PI 3.14159265359f

class Sphere;
using SpherePtr = std::shared_ptr<Sphere>;

/**
 * @class Sphere
 * @brief Gera uma malha esférica 3D a partir de um grid paramétrico (u,v).
 *
 * Cada vértice contém posição (3), normal (3), tangente (3) e UV (2).
 */
class Sphere : public geometry::Geometry {
protected:
    // Construtor protegido recebe diretamente os arrays já gerados
    Sphere(float* vertices, unsigned int* indices, int nverts, int nindices)
        : geometry::Geometry(vertices, indices, nverts, nindices, 3, {3,3,2})
    {}

    // Funções de geração de malha
    static float* generateVertexData(GridPtr grid, int& outVertexCount) {
        const float* texcoord = grid->GetCoords();
        int vertexCount = grid->VertexCount();
        outVertexCount = vertexCount;

        const int floatsPerVertex = 3 + 3 + 3 + 2;
        float* vertices = new float[vertexCount * floatsPerVertex];

        int idx = 0;
        for (int i = 0; i < vertexCount; ++i) {
            float u = texcoord[2*i];
            float v = texcoord[2*i + 1];

            float theta = u * 2.0f * PI;
            float phi   = v * PI;

            glm::vec3 pos(
                sinf(phi) * cosf(theta),
                cosf(phi),
                sinf(phi) * sinf(theta)
            );

            glm::vec3 normal = glm::normalize(pos);
            glm::vec3 tangent(-sinf(theta), 0.0f, cosf(theta));
            glm::vec2 uv(u, 1.0f - v);

            vertices[idx++] = pos.x; vertices[idx++] = pos.y; vertices[idx++] = pos.z;
            vertices[idx++] = normal.x; vertices[idx++] = normal.y; vertices[idx++] = normal.z;
            vertices[idx++] = tangent.x; vertices[idx++] = tangent.y; vertices[idx++] = tangent.z;
            vertices[idx++] = uv.x; vertices[idx++] = uv.y;
        }

        return vertices;
    }

    static unsigned int* generateIndices(GridPtr grid, int& outIndexCount) {
        const unsigned int* gridIndices = grid->GetIndices();
        int count = grid->IndexCount();
        outIndexCount = count;

        unsigned int* indices = new unsigned int[count];
        for(int i=0;i<count;++i)
            indices[i] = gridIndices[i];

        return indices;
    }

public:
    // Wrapper seguro para criar uma Sphere
    static SpherePtr Make(int nstack = 64, int nslice = 64) {
        GridPtr grid = Grid::Make(nslice, nstack);

        int nverts = 0;
        int nindices = 0;
        float* vertices = generateVertexData(grid, nverts);
        unsigned int* indices = generateIndices(grid, nindices);

        return SpherePtr(new Sphere(vertices, indices, nverts, nindices));
    }

    virtual ~Sphere() {}
};

#endif // SPHERE_H
