#ifndef MODELSTRUCTS_H
#define MODELSTRUCTS_H

#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <mesh.h>
#include <shader.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include <stdlib.h>
#include <cmath>
using namespace std;

#include <glm/gtx/string_cast.hpp>

unsigned int TextureFromFile(const char *path, const string &directory, bool gamma = false);
unsigned int TextureFromColor(const aiColor4D &color);

struct BoneInfo
{
	aiMatrix4x4 BoneOffset;
	aiMatrix4x4 FinalTransformation;

	BoneInfo()
	{
		aiMatrix4x4::Scaling(aiVector3D(0.0),BoneOffset);
		aiMatrix4x4::Scaling(aiVector3D(0.0), FinalTransformation);
	}
};

struct VertexBoneData
{
	unsigned int IDs[2*MAX_NUM_BONES];
	float Weights[2*MAX_NUM_BONES];
	int numBones;

	VertexBoneData()
	{
		Reset();
	};

	void Reset()
	{
		numBones = 0;
		for (int i = 0; i < 2 * MAX_NUM_BONES; i++) // ...initialize it
		{
			IDs[i] = 0;
			Weights[i] = 0.0f;
		}
	}

	void AddBoneData(unsigned int BoneID, float Weight) {
		if (numBones < 2 * MAX_NUM_BONES) {
			IDs[numBones] = BoneID;
			Weights[numBones] = Weight;
			numBones++;
		}
	}
};

struct Bone {
	aiString name;
	vector<unsigned int>    IDs;
	vector<float>           weights;
	glm::mat4               transformation;
	glm::mat4               offsetMatrix;

	Bone() {
		name = "";
		transformation = glm::mat4(1.0f);
		offsetMatrix   = glm::mat4(1.0f);
	};

	void push(unsigned int ID, float w) {
		IDs.push_back(ID);
		weights.push_back(w);
	}

};

// Carga una textura referenciada por un modelo.
// Es tolerante a rutas exportadas desde Blender: acepta separadores '\\',
// busca el archivo junto al modelo si la carpeta .fbm no existe y, si el formato
// no lo soporta stb_image (por ejemplo .webp), intenta la misma imagen en .png o .jpg.
// Si nada funciona crea una textura blanca de 1x1 para que el objeto muestre el
// color de su material en lugar de verse negro.
unsigned int TextureFromFile(const char *path, const string &directory, bool gamma)
{
    string original = string(path);
    string rel = original;
    for (char &c : rel) if (c == '\\') c = '/';
    string base = rel.substr(rel.find_last_of('/') + 1);
    string sinExt = base.substr(0, base.find_last_of('.'));

    vector<string> candidatos = {
        directory + '/' + rel,
        directory + '/' + base,
        directory + '/' + sinExt + ".png",
        directory + '/' + sinExt + ".jpg",
        directory + '/' + sinExt + ".fbm/" + sinExt + ".png"
    };

    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    int width = 0, height = 0, nrComponents = 0;
    unsigned char *data = nullptr;
    for (const string &ruta : candidatos) {
        data = stbi_load(ruta.c_str(), &width, &height, &nrComponents, 0);
        if (data) {
            if (ruta != candidatos[0])
                std::cout << "Textura " << original << " cargada desde " << ruta << std::endl;
            break;
        }
    }

    if (data)
    {
        GLenum format = GL_RGB;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << original
                  << " (se usa blanco; convierte la imagen a .png junto al modelo)" << std::endl;
        unsigned char blanco[4] = { 255, 255, 255, 255 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, blanco);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    return textureID;
}

// Textura de 1x1 con el color del material: para piezas de color liso que no
// traen imagen. Sin esto se usaria la ultima textura que quedo activa.
// El color de Blender viene en espacio lineal; se pasa a sRGB como las imagenes.
unsigned int TextureFromColor(const aiColor4D &color)
{
    auto aSRGB = [](float c) {
        c = c < 0.0f ? 0.0f : (c > 1.0f ? 1.0f : c);
        return (unsigned char)(powf(c, 1.0f / 2.2f) * 255.0f + 0.5f);
    };
    unsigned char px[4] = { aSRGB(color.r), aSRGB(color.g), aSRGB(color.b), 255 };

    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    return textureID;
}
#endif