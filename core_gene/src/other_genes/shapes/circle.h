#ifndef CIRCLE_H
#define CIRCLE_H
#pragma once

#include <memory>
class Circle;
using CirclePtr = std::shared_ptr<Circle>;

#include <math.h>
#include "polygon.h"

class Circle : public Polygon { // BACALHAU falta extender para circulos com mais de uma cor sólida
    float radius;
    unsigned int discretization;
    bool centered;

    float* geraDadosVertices(float x_center, float y_center, float radius, float* corRGB, unsigned int edge_points, bool has_center_vertex) {
        int num_points = edge_points;
        if (has_center_vertex) num_points++;

        float* vertices = new float[num_points * 5]; // 2 para posição, 3 para cor
        this->radius = radius;
        this->discretization = edge_points;
        this->centered = has_center_vertex;

        float angle_step = 2.0f * M_PI / edge_points;
        if (has_center_vertex) {
            vertices[0] = x_center;
            vertices[1] = y_center;
            vertices[2] = corRGB[0];
            vertices[3] = corRGB[1];
            vertices[4] = corRGB[2];
        }
        for (unsigned int i = has_center_vertex; i < num_points; i++) {
            float angle = i * angle_step;
            float x = x_center + radius * cos(angle);
            float y = y_center + radius * sin(angle);
            vertices[i * 5 + 0] = x;
            vertices[i * 5 + 1] = y;
            vertices[i * 5 + 2] = corRGB[3*has_center_vertex + 0];
            vertices[i * 5 + 3] = corRGB[3*has_center_vertex + 1];
            vertices[i * 5 + 4] = corRGB[3*has_center_vertex + 2];
        }
        return vertices;
    }

    unsigned int* geraIndices(unsigned int edge_points, bool has_center_vertex) {
        int num_points = edge_points;
        if (has_center_vertex) num_points++;

        unsigned int* indices = new unsigned int[(num_points - 1) * 3]; 

        for (unsigned int i = 0; i < edge_points; i++) {
            if (has_center_vertex) {
                indices[i * 3 + 0] = 0; // centro
                indices[i * 3 + 1] = i + 1;
                indices[i * 3 + 2] = (i + 1) % edge_points + 1;
            } else {
                indices[i * 3 + 0] = 0;
                indices[i * 3 + 1] = (i + 1) % edge_points;
                indices[i * 3 + 2] = (i + 2) % edge_points;
            }
        }
        return indices;
    }

    int geraNumVertices(unsigned int edge_points, bool has_center_vertex) {
        int num_points = edge_points;
        if (has_center_vertex) num_points++;
        return num_points;
    }

    int geraNumIndices(unsigned int edge_points, bool has_center_vertex) {
        return (edge_points - 1 + has_center_vertex) * 3;
    }

    Circle(float x_center, float y_center, float radius, float* corRGB, unsigned int edge_points, bool has_center_vertex) : 
        Polygon(
            geraDadosVertices(x_center, y_center, radius, corRGB, edge_points, has_center_vertex),
            geraIndices(edge_points, has_center_vertex),
            geraNumVertices(edge_points, has_center_vertex),
            geraNumIndices(edge_points, has_center_vertex), 
            {3}
        ) 
        {};
public:
    static CirclePtr Make(float x_center, float y_center, float radius, float* corRGB, unsigned int edge_points, bool has_center_vertex) {
        CirclePtr circle (new Circle(x_center, y_center, radius, corRGB, edge_points, has_center_vertex));
        glFlush();
        return circle;
    }
    virtual ~Circle () {}
    virtual void Draw () {
        Geometry::Draw();
    }
};

#endif