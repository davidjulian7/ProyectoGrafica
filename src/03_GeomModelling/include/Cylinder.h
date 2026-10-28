#ifndef CYLINDER_H
#define CYLINDER_H

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

class Cylinder {
public:
	Cylinder(float size) {
		buildVertices(size);
		buildIndices();
	}

	~Cylinder() {
		vertices.clear();
		indices.clear();
	}

	vector<Vertex>       vertices;
	vector<unsigned int> indices;

private:

	void buildVertices(float size) {
		for (int theta = 0; theta < 360; theta += 5) {
			Vertex  v;
			double rdeg = glm::radians((double)theta);
			v.Position.x = size * (float)cos(rdeg);
			v.Position.y = size * (float)sin(rdeg);
			v.Position.z = -size;
			v.Color.r = 1.0f;
			v.Color.g = 1.0f;
			v.Color.b = 1.0f;
			v.Color.a = 1.0f;
			if (theta == 0) { v.Color.r = 0.0f; v.Color.g = 1.0f; v.Color.b = 1.0f; }
			if (theta == 120) { v.Color.r = 1.0f; v.Color.g = 0.0f; v.Color.b = 1.0f; }
			if (theta == 240) { v.Color.r = 0.0f; v.Color.g = 0.0f; v.Color.b = 1.0f; }
			vertices.push_back(v);
		}

		for (int theta = 0; theta < 360; theta += 5) {
			Vertex  v;
			double rdeg = glm::radians((double)theta);
			v.Position.x = size * (float)cos(rdeg);
			v.Position.y = size * (float)sin(rdeg);
			v.Position.z = size;
			v.Color.r = 1.0f;
			v.Color.g = 1.0f;
			v.Color.b = 1.0f;
			v.Color.a = 1.0f;
			if (theta == 0) { v.Color.r = 0.0f; v.Color.g = 1.0f; v.Color.b = 1.0f; }
			if (theta == 120) { v.Color.r = 1.0f; v.Color.g = 0.0f; v.Color.b = 1.0f; }
			if (theta == 240) { v.Color.r = 0.0f; v.Color.g = 0.0f; v.Color.b = 1.0f; }
			vertices.push_back(v);
		}
	}

	void buildIndices() {
		int numVertices2 = vertices.size() / 2.0;
		for (int i = 0; i < numVertices2 - 1; i++) {
			indices.push_back(i); indices.push_back(i + 1); indices.push_back(numVertices2 + i);
			//cout << i << "-" << i+1 << "-" << numVertices2 + i << endl;
		}
		indices.push_back(numVertices2-1); indices.push_back(2*numVertices2 - 1); indices.push_back(0);
		
		for (int i = 0; i < numVertices2 - 1; i++) {
			indices.push_back(numVertices2 + i); indices.push_back(numVertices2 + i + 1); indices.push_back(i + 1);
			//cout << numVertices2 + i << "*" << numVertices2 + i + 1 << "*" << i + 1 << endl;
		}

		indices.push_back(0); indices.push_back(numVertices2); indices.push_back(2 * numVertices2 - 1);
	}
};

#endif