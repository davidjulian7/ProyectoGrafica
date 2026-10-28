#ifndef SPHERE_H
#define SPHERE_H

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

class Sphere {
public:
	Sphere(float size) {
		buildVertices(size);
		buildIndices();
	}

	~Sphere() {
		vertices.clear();
		indices.clear();
	}

	vector<Vertex>       vertices;
	vector<unsigned int> indices;

private:

	void buildVertices(float size) {
		int cols = 360 / 5;  // 72 vertices por fila
		for (int phi = 0; phi < 180; phi += 5) {
			for (int theta = 0; theta < 360; theta += 5) {
				Vertex  v;
				double trdeg = glm::radians((double)theta);
				double prdeg = glm::radians((double)phi);
				v.Position.x = size * (float)sin(prdeg) * (float)cos(trdeg);
				v.Position.y = size * (float)sin(prdeg) * (float)sin(trdeg);
				v.Position.z = size * (float)cos(prdeg);
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
	}

	void buildIndices() {
		int cols = 360 / 5;   // 72 vertices por fila
		int rows = 180 / 5;   // 36 filas (phi: 0, 5, 10, ..., 175)

		for (int p = 0; p < rows - 1; p++) {
			for (int t = 0; t < cols; t++) {
				int topLeft     = p * cols + t;
				int topRight    = p * cols + (t + 1) % cols;
				int bottomLeft  = (p + 1) * cols + t;
				int bottomRight = (p + 1) * cols + (t + 1) % cols;

				// Primer triangulo del quad
				indices.push_back(topLeft);
				indices.push_back(bottomLeft);
				indices.push_back(topRight);

				// Segundo triangulo del quad
				indices.push_back(topRight);
				indices.push_back(bottomLeft);
				indices.push_back(bottomRight);
			}
		}
	}
};

#endif