#ifndef VERTEX_H
#define VERTEX_H

#include <glad/glad.h> // holds all OpenGL type declarations

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Vertex {
    // position
    glm::vec3 Position;
    // color
    glm::vec4 Color;
};

#endif