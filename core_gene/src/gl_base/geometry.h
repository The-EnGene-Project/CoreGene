#ifndef GEOMETRY_H
#define GEOMETRY_H
#pragma once

#include "gl_includes.h"
#include "shader.h"

#include <memory>
#include <vector>


namespace geometry {

class Geometry;
using GeometryPtr = std::shared_ptr<Geometry>;


class Geometry {
    unsigned int mode = GL_TRIANGLES;
    unsigned int nverts;
    unsigned int type = GL_UNSIGNED_INT;
    unsigned int offset = 0;
    unsigned int m_vao;
    unsigned int m_vbo;
    unsigned int m_ebo; 
    int n_indices;

protected:
    // Construtor agora armazena vbo e ebo
    // Novo argumento: const std::vector<int>& attr_sizes
    // attr_sizes: cada elemento indica quantos floats para cada atributo extra (além da posição)
    Geometry(float* dados_vertices, unsigned int* indices, int nverts, int n_indices, int pos_size, const std::vector<int>& attr_sizes) : 
    nverts(nverts),
    n_indices(n_indices)
    {
        // Define os parâmetros para o Draw()
        mode = GL_TRIANGLES;
        type = GL_UNSIGNED_INT;
        int amt_of_floats_per_vertex = pos_size;
        for (int sz : attr_sizes) amt_of_floats_per_vertex += sz;
        int vertex_stride_in_bytes = amt_of_floats_per_vertex * sizeof(float);

        // 1. Geração e bind do VAO
        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);

        // 2. Geração, bind e envio de dados para o VBO
        glGenBuffers(1, &m_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, amt_of_floats_per_vertex * nverts * sizeof(float), dados_vertices, GL_STATIC_DRAW);

        // 3. Configuração dos atributos dos vértices
        // Diz ao OpenGL como interpretar os dados de posição do VBO
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, vertex_stride_in_bytes, (void*)0);
        glEnableVertexAttribArray(0);

        // Configura atributos extras
        int attrib_index = 1;
        int offset_floats = pos_size;
        for (int sz : attr_sizes) {
            glVertexAttribPointer(attrib_index, sz, GL_FLOAT, GL_FALSE, vertex_stride_in_bytes, (void*)(offset_floats * sizeof(float)));
            glEnableVertexAttribArray(attrib_index);
            offset_floats += sz;
            attrib_index++;
        }

        // 4. Geração, bind e envio de dados para o EBO
        glGenBuffers(1, &m_ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, n_indices * sizeof(unsigned int), indices, GL_STATIC_DRAW);
        
        // 5. Unbind de tudo para evitar modificações acidentais
        // O VAO "lembra" dos binds do VBO e EBO, então podemos desvinculá-los.
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

public:
    // Novo argumento: const std::vector<int>& attr_sizes
    static GeometryPtr Make(float* dados_vertices, unsigned int* indices, int nverts, int n_indices, int pos_size, const std::vector<int>& attr_sizes) {
        // é trabalho do caller interpolar as posições e cores
        return GeometryPtr(new Geometry(dados_vertices, indices, nverts, n_indices, pos_size, attr_sizes));
    }

    // Destrutor que libera os buffers da GPU
    virtual ~Geometry() {
        glDeleteBuffers(1, &m_vbo);
        glDeleteBuffers(1, &m_ebo);
        glDeleteVertexArrays(1, &m_vao);
    }

    // Função de desenho
    virtual void Draw() {
        glBindVertexArray(m_vao);
        // Desenha índices (3*(nverts-2))
        glDrawElements(mode, n_indices, type, (void*)0);
        glBindVertexArray(0);
    }
};

}

#endif
