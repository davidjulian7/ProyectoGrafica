#ifndef MFUNCTION_H
#define MFUNCTION_H

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

class MathFunction {
public:
	MathFunction(float size) {
		buildVertices(size);
		buildIndices();
	}

	~MathFunction() {
		vertices.clear();
		indices.clear();
	}

	vector<Vertex>       vertices;
	vector<unsigned int> indices;

private:

	void buildVertices(float size) {
		for (int theta = 0; theta <= 360; theta += 5) {
			Vertex  v;
			double rdeg = glm::radians((double)theta);
			v.Position.x = (float)theta/180.0f-1.0f;
			v.Position.y = size * (float)cos( rdeg - glm::radians(180.0) );
			v.Position.z = 0.0f;
			if (theta == 0) { v.Color.r = 1.0f; v.Color.g = 0.0f; v.Color.b = 0.0f; }
			if (theta == 120) { v.Color.r = 0.0f; v.Color.g = 1.0f; v.Color.b = 0.0f; }
			if (theta == 240) { v.Color.r = 0.0f; v.Color.g = 0.0f; v.Color.b = 1.0f; }
			v.Color.a = 1.0f;
			vertices.push_back(v);
		}
	}

	void buildIndices() {
	}
};

#endif 