#include <memory>
class Polygon;
using PolygonPtr = std::shared_ptr<Polygon>; 

#ifndef POLYGON_H
#define POLYGON_H
#pragma once
#include "../../gl_base/gl_includes.h"
#include "../../gl_base/geometry.h"

class Polygon : public geometry::Geometry {
protected:
  Polygon(float* dados_vertices, unsigned int* indices, int nverts, int n_indices, const std::vector<int>& attr_sizes = {3}) : 
    geometry::Geometry(dados_vertices, indices, nverts, n_indices, 2, attr_sizes) 
    {};
public:
  static PolygonPtr Make (float* posicoes, float* cores, unsigned int* indices, int nverts, int n_indices) {

    // Interpola posições e cores
    float* dados_vertices = new float[5 * nverts]; // 2 para posição, 3 para cor
    for (int i = 0; i < nverts; i++) {
      dados_vertices[5*i + 0] = posicoes[2*i + 0];
      dados_vertices[5*i + 1] = posicoes[2*i + 1];
      dados_vertices[5*i + 2] = cores[3*i + 0];
      dados_vertices[5*i + 3] = cores[3*i + 1];
      dados_vertices[5*i + 4] = cores[3*i + 2];
    }

    // Cria o Polygon
    PolygonPtr polygon (new Polygon(dados_vertices, indices, nverts, n_indices));
    glFlush();
    delete[] dados_vertices;
    return polygon;

  }
  static PolygonPtr Make (float* dados_vertices, unsigned int* indices, int nverts, int n_indices) {
    PolygonPtr polygon (new Polygon(dados_vertices, indices, nverts, n_indices));
    glFlush();
    return polygon;
  }
  virtual ~Polygon () {}
  virtual void Draw () {
    geometry::Geometry::Draw();
  }
};
#endif