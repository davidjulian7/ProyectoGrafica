#ifndef PLANE_H
#define PLANE_H

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

class Plane {
public:
	Plane(float size) {
		buildVertices(size);
		buildIndices();
	}

	~Plane() {
		vertices.clear();
		indices.clear();
	}

	vector<Vertex>       vertices;
	vector<unsigned int> indices;

private:

	void buildVertices(float size) {
		float value = size / 2.0f;

		Vertex v;

		v.Position.x = value; v.Position.y = value; v.Position.z = 0.0f;
		v.Color.x = 1.0f; v.Color.y = 0.0f; v.Color.z = 0.0f;
		vertices.push_back(v);

		v.Position.x = -value; v.Position.y = value; v.Position.z = 0.0f;
		v.Color.x = 0.0f; v.Color.y = 1.0f; v.Color.z = 0.0f;
		vertices.push_back(v);

		v.Position.x = -value; v.Position.y = -value; v.Position.z = 0.0f;
		v.Color.x = 0.0f; v.Color.y = 0.0f; v.Color.z = 1.0f;
		vertices.push_back(v);

		v.Position.x = value; v.Position.y = -value; v.Position.z = 0.0f;
		v.Color.x = 0.0f; v.Color.y = 1.0f; v.Color.z = 1.0f;
		vertices.push_back(v);

	}

	void buildIndices() {
		indices.push_back(0); indices.push_back(1); indices.push_back(2);
		indices.push_back(0); indices.push_back(2); indices.push_back(3);
	}
};

#endif