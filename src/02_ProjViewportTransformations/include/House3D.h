#ifndef HOUSE3D_H
#define HOUSE3D_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Vertex.h"
#include <vector>
using namespace std;
using namespace glm;

class House3D {
public:
    House3D() {
        buildVertices();
        buildIndices();
    }

    ~House3D() {
        vertices.clear();
        indices.clear();
    }

    vector<Vertex>       vertices;
    vector<unsigned int> indices;

private:

    Vertex makeVertex(float x, float y, float z, float r, float g, float b) {
        Vertex v;
        v.Position = glm::vec3(x, y, z);
        v.Color = glm::vec4(r, g, b, 1.0f);
        return v;
    }

    void buildVertices() {
        // =============================================
        // Casa: prisma de base pentagonal (perfil casa)
        // Ancho (X): 1.5  -> -0.75 a +0.75
        // Paredes (Y): 1.2 -> -0.6 a +0.6
        // Techo   (Y): 0.6 -> +0.6 a +1.2
        // Profundidad (Z): 1.5 -> -0.75 a +0.75
        // Centrado en el origen (0, 0, 0)
        //
        //           4 / 9  (cumbrera)
        //          / \
        //     3---2   8---7  (pared/techo)
        //     |       |
        //     0---1   5---6  (base)
        //   frontal   posterior
        // =============================================

        // Frente: cyan | Fondo: magenta -> degradado en caras laterales
        float fr = 0.0f, fg = 1.0f, fb = 1.0f; // cyan
        float br = 1.0f, bg = 0.0f, bb = 1.0f; // magenta

        // Cara frontal (Z = +0.75) - cyan
        vertices.push_back(makeVertex(-0.75f, -0.60f,  0.75f, fr, fg, fb)); // 0
        vertices.push_back(makeVertex( 0.75f, -0.60f,  0.75f, fr, fg, fb)); // 1
        vertices.push_back(makeVertex( 0.75f,  0.60f,  0.75f, fr, fg, fb)); // 2
        vertices.push_back(makeVertex(-0.75f,  0.60f,  0.75f, fr, fg, fb)); // 3
        vertices.push_back(makeVertex( 0.00f,  1.20f,  0.75f, fr, fg, fb)); // 4

        // Cara posterior (Z = -0.75) - magenta
        vertices.push_back(makeVertex(-0.75f, -0.60f, -0.75f, br, bg, bb)); // 5
        vertices.push_back(makeVertex( 0.75f, -0.60f, -0.75f, br, bg, bb)); // 6
        vertices.push_back(makeVertex( 0.75f,  0.60f, -0.75f, br, bg, bb)); // 7
        vertices.push_back(makeVertex(-0.75f,  0.60f, -0.75f, br, bg, bb)); // 8
        vertices.push_back(makeVertex( 0.00f,  1.20f, -0.75f, br, bg, bb)); // 9
    }

    void buildIndices() {

    // =============================================
    // CARA FRONTAL
    // =============================================

    // Rectángulo
    indices.push_back(0);
    indices.push_back(1);
    indices.push_back(2);

    indices.push_back(0);
    indices.push_back(2);
    indices.push_back(3);

    // Techo frontal
    indices.push_back(3);
    indices.push_back(2);
    indices.push_back(4);


    // =============================================
    // CARA POSTERIOR
    // =============================================

    // Rectángulo
    indices.push_back(6);
    indices.push_back(5);
    indices.push_back(8);

    indices.push_back(7);
    indices.push_back(6);
    indices.push_back(8);

    // Techo posterior
    indices.push_back(7);
    indices.push_back(8);
    indices.push_back(9);


    // =============================================
    // BASE
    // =============================================

    indices.push_back(0);
    indices.push_back(5);
    indices.push_back(6);

    indices.push_back(0);
    indices.push_back(6);
    indices.push_back(1);


    // =============================================
    // PARED IZQUIERDA
    // =============================================

    indices.push_back(0);
    indices.push_back(8);
    indices.push_back(5);

    indices.push_back(0);
    indices.push_back(3);
    indices.push_back(8);


    // =============================================
    // PARED DERECHA
    // =============================================

    indices.push_back(1);
    indices.push_back(6);
    indices.push_back(7);

    indices.push_back(1);
    indices.push_back(7);
    indices.push_back(2);


    // =============================================
    // TECHO IZQUIERDO
    // =============================================

    indices.push_back(3);
    indices.push_back(4);
    indices.push_back(9);

    indices.push_back(3);
    indices.push_back(9);
    indices.push_back(8);


    // =============================================
    // TECHO DERECHO
    // =============================================

    indices.push_back(2);
    indices.push_back(7);
    indices.push_back(9);

    indices.push_back(2);
    indices.push_back(9);
    indices.push_back(4);
}
};

#endif
