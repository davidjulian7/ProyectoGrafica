#ifndef CIRCLE_H
#define CIRCLE_H

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

class Circle {
public:
	Circle(float size) {
		buildVertices(size);
		buildIndices();
	}

	~Circle() {
		vertices.clear();
		indices.clear();
	}

	vector<Vertex>       vertices;
	vector<unsigned int> indices;

private:

	void buildVertices(float size) {

		Vertex v0;
		v0.Position.x = 0.0f; v0.Position.y = 0.0f; v0.Position.z = 0.0f;
		v0.Color.x = 1.0f; v0.Color.y = 1.0f; v0.Color.z = 0.0f;
		vertices.push_back(v0);

		for (int theta = 0; theta <= 360; theta += 15) {
			Vertex  v;
			double rdeg = glm::radians((double)theta);
			v.Position.x = size * (float)cos(rdeg);
			v.Position.y = size * (float)sin(rdeg);
			v.Position.z = 0.0f;
			if (theta <= 45) { v.Color.r = 0.0f; v.Color.g = 1.0f; v.Color.b = 1.0f; }
			else if (theta <= 90) { v.Color.r = 1.0f; v.Color.g = 0.0f; v.Color.b = 1.0f; }
			else if (theta <= 135) { v.Color.r = 0.0f; v.Color.g = 0.0f; v.Color.b = 1.0f; }
			else if (theta <= 270) { v.Color.r = 0.0f; v.Color.g = 1.0f; v.Color.b = 1.0f; }
			else if (theta <= 315) { v.Color.r = 1.0f; v.Color.g = 1.0f; v.Color.b = 0.0f; }
			else if (theta <= 360) { v.Color.r = 0.0f; v.Color.g = 1.0f; v.Color.b = 1.0f; }
			v.Color.a = 1.0f;
			vertices.push_back(v);
		}
	}

	void buildIndices() {
		for ( int i = 0; i < vertices.size(); i++ )
			indices.push_back(i);
	}
};

#endif