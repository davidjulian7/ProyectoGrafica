#ifndef CUBE_H
#define CUBE_H

#include <glad/glad.h> // holds all OpenGL type declarations

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Vertex.h"
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
using namespace std;
using namespace glm;

class Cube {
public:
	Cube(float size) {
		buildVertices(size);
		buildIndices();
	}

	~Cube() {
		vertices.clear();
		indices.clear();
	}

	vector<Vertex>       vertices;
	vector<unsigned int> indices;

private:

	void buildVertices(float size) {
		float value = size / 2.0f;

		Vertex v;

		// Primeros cuatro vértices
		v.Position.x = value; v.Position.y = value; v.Position.z = -value;
		v.Color.x = 1.0f; v.Color.y = 0.0f; v.Color.z = 0.0f;
		vertices.push_back(v);

		v.Position.x = -value; v.Position.y = value; v.Position.z = -value;
		v.Color.x = 0.0f; v.Color.y = 1.0f; v.Color.z = 0.0f;
		vertices.push_back(v);

		v.Position.x = -value; v.Position.y = -value; v.Position.z = -value;
		v.Color.x = 0.0f; v.Color.y = 0.0f; v.Color.z = 1.0f;
		vertices.push_back(v);

		v.Position.x = value; v.Position.y = -value; v.Position.z = -value;
		v.Color.x = 0.0f; v.Color.y = 1.0f; v.Color.z = 1.0f;
		vertices.push_back(v);

		// Siguientes cuatro vértices

		v.Position.x = value; v.Position.y = value; v.Position.z = value;
		v.Color.x = 1.0f; v.Color.y = 0.0f; v.Color.z = 0.0f;
		vertices.push_back(v);

		v.Position.x = -value; v.Position.y = value; v.Position.z = value;
		v.Color.x = 0.0f; v.Color.y = 1.0f; v.Color.z = 0.0f;
		vertices.push_back(v);

		v.Position.x = -value; v.Position.y = -value; v.Position.z = value;
		v.Color.x = 0.0f; v.Color.y = 0.0f; v.Color.z = 1.0f;
		vertices.push_back(v);

		v.Position.x = value; v.Position.y = -value; v.Position.z = value;
		v.Color.x = 0.0f; v.Color.y = 1.0f; v.Color.z = 1.0f;
		vertices.push_back(v);
	}

	void buildIndices() {
		indices.push_back(0); indices.push_back(1); indices.push_back(2);
		indices.push_back(0); indices.push_back(2); indices.push_back(3);

		indices.push_back(4); indices.push_back(5); indices.push_back(6);
		indices.push_back(4); indices.push_back(6); indices.push_back(7);

		indices.push_back(0); indices.push_back(3); indices.push_back(4);
		indices.push_back(3); indices.push_back(4); indices.push_back(7);

		indices.push_back(1); indices.push_back(5); indices.push_back(2);
		indices.push_back(2); indices.push_back(5); indices.push_back(6);

		indices.push_back(0); indices.push_back(1); indices.push_back(5);
		indices.push_back(0); indices.push_back(4); indices.push_back(5);

		indices.push_back(2); indices.push_back(3); indices.push_back(6);
		indices.push_back(3); indices.push_back(6); indices.push_back(7);

	}
};

#endif