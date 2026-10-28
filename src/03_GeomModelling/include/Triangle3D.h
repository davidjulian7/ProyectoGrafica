#ifndef TRIANGLE3D_H
#define TRIANGLE3D_H

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

class Triangle3D {
public:
	Triangle3D(float size) {
		buildVertices(size);
		buildIndices();
	}

	~Triangle3D() {
		vertices.clear();
		indices.clear();
	}

	vector<Vertex>       vertices;
	vector<unsigned int> indices;

private:

	void buildVertices(float size) {

		for (int theta = 0; theta < 360; theta += 120) {
			Vertex  v;
			double rdeg = glm::radians((double)theta);
			v.Position.x = size * (float)cos(rdeg);
			v.Position.y = size * (float)sin(rdeg);
			v.Position.z = 0.0f;
			if (theta == 0) { v.Color.r = 0.0f; v.Color.g = 1.0f; v.Color.b = 1.0f; }
			if (theta == 120) { v.Color.r = 1.0f; v.Color.g = 0.0f; v.Color.b = 1.0f; }
			if (theta == 240) { v.Color.r = 0.0f; v.Color.g = 0.0f; v.Color.b = 1.0f; }
			v.Color.a = 1.0f;
			vertices.push_back(v);
		}
		for (int theta = 0; theta < 360; theta += 120) {
			Vertex  v;
			double rdeg = glm::radians((double)theta);
			v.Position.x = size * (float)cos(rdeg);
			v.Position.y = size * (float)sin(rdeg);
			v.Position.z = -1.0f;
			if (theta == 0) { v.Color.r = 0.0f; v.Color.g = 1.0f; v.Color.b = 1.0f; }
			if (theta == 120) { v.Color.r = 1.0f; v.Color.g = 0.0f; v.Color.b = 1.0f; }
			if (theta == 240) { v.Color.r = 0.0f; v.Color.g = 0.0f; v.Color.b = 1.0f; }
			v.Color.a = 1.0f;
			vertices.push_back(v);
		}
	}

	void buildIndices() {
		for ( int i = 0; i < 6; i++ )
			indices.push_back(i);

		indices.push_back(1); indices.push_back(3); indices.push_back(4);
		indices.push_back(0); indices.push_back(1); indices.push_back(3);

		indices.push_back(0); indices.push_back(2); indices.push_back(3);
		indices.push_back(2); indices.push_back(3); indices.push_back(5);

		indices.push_back(1); indices.push_back(2); indices.push_back(4);
		indices.push_back(4); indices.push_back(2); indices.push_back(5);
	}
};

#endif