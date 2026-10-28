#ifndef MESH_H
#define MESH_H

#include <glad/glad.h> // holds all OpenGL type declarations

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Shader.h"
#include "Vertex.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
using namespace std;

class Mesh {
public:
    /*  Mesh Data  */
    vector<Vertex>       vertices;
    vector<unsigned int> indices;
    unsigned int         VAO;

    /*  Functions  */
    // constructor
    Mesh(vector<Vertex> vertices, vector<unsigned int> indices)
    {
        if ( vertices.size() > 0 )
            this->vertices = vertices;

        if ( indices.size() > 0 )
            this->indices = indices;

        setupMesh();
    }

    ~Mesh() {
        this->vertices.clear();
        this->indices.clear();
    }

    // render the mesh
    void Draw(Shader shader) 
    {
   
        // draw mesh
        glBindVertexArray(VAO);
        /* glDrawElements — render primitives from array data
        void glDrawElements(GLenum mode,
            GLsizei count,
            GLenum type,
            const void* indices);
        mode
            Specifies what kind of primitives to render. Symbolic constants GL_POINTS, GL_LINE_STRIP, GL_LINE_LOOP, GL_LINES, GL_LINE_STRIP_ADJACENCY, GL_LINES_ADJACENCY, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, GL_TRIANGLES, GL_TRIANGLE_STRIP_ADJACENCY, GL_TRIANGLES_ADJACENCY and GL_PATCHES are accepted.

        count
            Specifies the number of elements to be rendered.

        type
            Specifies the type of the values in indices. Must be one of GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT, or GL_UNSIGNED_INT.

        indices
            Specifies a pointer to the location where the indices are stored.
        */
        if (indices.size() > 0)
            glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
        else if (vertices.size() > 0) 
            glDrawArrays(GL_LINES, 0, (GLsizei)vertices.size());

        glBindVertexArray(0);

    }

private:
    /*  Render data  */
    unsigned int VBO, EBO;

    /*  Functions    */
    // initializes all the buffer objects/arrays
    void setupMesh()
    {

        // create buffers/arrays
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        if (vertices.size() > 0)
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);  

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        if ( indices.size() > 0 )
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
		
        // set the vertex attribute pointers
        // vertex Positions
        glEnableVertexAttribArray(0);	
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        // vertex Color
        glEnableVertexAttribArray(1);	
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Color));
        
        glBindVertexArray(0);
    }

};
#endif