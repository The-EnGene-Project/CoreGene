#ifndef GRID_H
#define GRID_H
#pragma once

#include <memory>
#include <vector>

/**
 * @class Grid
 * @brief Representa uma malha 2D de coordenadas paramétricas (u,v) usada para gerar superfícies 3D.
 *
 * O grid define um conjunto de vértices distribuídos uniformemente no intervalo [0,1]x[0,1],
 * e os índices triangulares que conectam esses vértices.
 *
 * É amplamente utilizado por geometrias paramétricas como esferas, cilindros, toros, etc.
 */
class Grid;
using GridPtr = std::shared_ptr<Grid>;

class Grid {
private:
    int nx, ny;
    std::vector<float> coords;         // 2 floats por vértice (u,v)
    std::vector<unsigned int> indices; // 6 índices por célula (2 triângulos)

public:
    /**
     * @brief Cria e retorna um grid com dimensões (nx, ny)
     */
    static GridPtr Make(int nx, int ny) {
        return std::make_shared<Grid>(nx, ny);
    }

    /**
     * @brief Constrói um grid de tamanho (nx, ny)
     */
    Grid(int nx, int ny) : nx(nx), ny(ny)
    {
        if (nx < 1) nx = 1;
        if (ny < 1) ny = 1;

        // Gera coordenadas UV (u,v) no intervalo [0,1]
        coords.resize(2 * VertexCount());
        float dx = 1.0f / nx;
        float dy = 1.0f / ny;

        int k = 0;
        for (int j = 0; j <= ny; ++j) {
            for (int i = 0; i <= nx; ++i) {
                coords[k++] = i * dx; // u
                coords[k++] = j * dy; // v
            }
        }

        // Gera índices triangulares
        indices.resize(IndexCount());
        int t = 0;
        for (int j = 0; j < ny; ++j) {
            for (int i = 0; i < nx; ++i) {
                unsigned int a = (unsigned int)(j * (nx + 1) + i);
                unsigned int b = a + 1;
                unsigned int c = a + (nx + 1);
                unsigned int d = c + 1;

                // Triângulo 1
                indices[t++] = a;
                indices[t++] = b;
                indices[t++] = d;

                // Triângulo 2
                indices[t++] = a;
                indices[t++] = d;
                indices[t++] = c;
            }
        }
    }

    // ==========================================================
    // Métodos de acesso
    // ==========================================================
    int GetNx() const { return nx; }
    int GetNy() const { return ny; }

    int VertexCount() const { return (nx + 1) * (ny + 1); }
    int IndexCount() const { return 6 * nx * ny; }

    const float* GetCoords() const { return coords.data(); }
    const unsigned int* GetIndices() const { return indices.data(); }

    const std::vector<float>& GetCoordsVec() const { return coords; }
    const std::vector<unsigned int>& GetIndicesVec() const { return indices; }
};

#endif // GRID_H