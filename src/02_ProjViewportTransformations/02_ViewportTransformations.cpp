/*
* Laboratorio de Computación Gráfica e Interacción Humano-Computadora
* 02 - Proyecciones, puerto de vista y transformaciones en 3D
*/

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <sstream>

// GLAD: Multi-Language GL/GLES/EGL/GLX/WGL Loader-Generator
// https://glad.dav1d.de/
#include <glad/glad.h>

// GLFW: https://www.glfw.org/
#include <GLFW/glfw3.h>

// GLM: OpenGL Math library
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Model loading classes
#include "Shader.h"
#include "Mesh.h"
#include "Triangle.h"
#include "Triangle3D.h"
#include "MathFunction.h"
#include "Piramide.h"
#include "House3D.h"
#include "Camera.h"

// Functions
bool Start();
bool Update();

// Definición de callbacks
        
void mouseCallback    (GLFWwindow* window, double xpos, double ypos);       // Mouse
void scrollCallback   (GLFWwindow* window, double xoffset, double yoffset); // Mouse movement
void keyboardCallback (GLFWwindow* window);                                 // Keyboard

void resizeCallback   (GLFWwindow* window, int width, int height);          // Window resize

using namespace std;

// Globals
GLFWwindow* window;
GLfloat  bgR, bgG, bgB, bgA;
Shader    * myShader;
Mesh      * mesh;
int       screenWidth = 600, screenHeight = 600;

// Camera definition (XYZ position)
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f),
              glm::vec3(0.0f, 0.0f, -1.0f),
              glm::vec3(0.0f, 1.0f, 0.0f));

Vertex modelPosition;
float  modelScale = 1.0f;

int main(void)
{
    if (!Start())
        return -1;

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        if (!Update())
            break;
    }

    glfwTerminate();
    return 0;
}

bool Start() {

    /* Initialize the library GLFW */
    if (!glfwInit())
        return false;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(screenWidth, screenHeight, "Viewport Transformations", NULL, NULL);
    if (!window)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    /* Define callbacks */
    glfwSetFramebufferSizeCallback(window, resizeCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetScrollCallback(window, scrollCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    // Depth test activation
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Background color initialization
    bgR = bgG = bgB = bgA = 0.0f;

    // Inicializamos la posición del modelo
    modelPosition.Position.x = 0.0f;
    modelPosition.Position.y = 0.0f;
    modelPosition.Position.z = 0.0f;
    
    // Shaders compilation and linking
    myShader = new Shader("shaders/02-simplePVM.vs", "shaders/02-simplePVM.fs");
    if (myShader == nullptr) {
        cout << "Error creating shaders." << endl;
        return false;
    }

    // Init geometries
    House3D house;

    // Ejercicio 1: Casa (prisma de base pentagonal)
    mesh = new Mesh(house.vertices, house.indices);
    if (mesh == nullptr) {
        cout << "Error creating mesh object." << endl;
        return false;
    }

    return true;
}

bool Update() {
    /* Process keyboard */
    keyboardCallback(window);
    
    /* Change the background Color */
    glClearColor(bgR, bgG, bgB, bgA);
    
    /* Render here */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* Draw objects */
    if (mesh != nullptr) {
        glUseProgram(myShader->ID);

        // Aplicamos transformaciones de proyección y cámara (si las hubiera)
        // Ejercicio 3. Proyección ortogonal
        glm::mat4 projection = glm::ortho(-1.5f, 1.5f, -1.5f, 1.5f, 0.1f, 1000.0f);
        
        // Ejercicio 4. Proyección en perspectiva
        // glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)screenWidth / (float)screenHeight, 0.1f, 1000.0f);
        
        myShader->setMat4("projection", projection);

        glm::mat4 view = glm::lookAt(camera.Position, camera.View, camera.Up);
        myShader->setMat4("view", view);

        // Aplicamos transformaciones del modelo
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(modelPosition.Position.x, 
                                                modelPosition.Position.y,
                                                modelPosition.Position.z));
        model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(modelScale, modelScale, modelScale));
        myShader->setMat4("model", model);

        mesh->Draw(*myShader);
    }

    /* Swap front and back buffers */
    glfwSwapBuffers(window);

    /* Poll for and process events */
    glfwPollEvents();

    return true;
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    // Do something when mouse actioned
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Do something when mouse moved
}

void keyboardCallback(GLFWwindow* window) {
    // Do something when key pressed

    // Exit action
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    // ejercicio 2.
    // vista frontal (F1): cámara en +Z mirando al origen
    if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS) {
        camera.Position = glm::vec3(0.0f, 0.0f, 3.0f);
        camera.View     = glm::vec3(0.0f, 0.0f, 0.0f);
        camera.Up       = glm::vec3(0.0f, 1.0f, 0.0f);
    }
    // vista lateral (F2): cámara en +X mirando al origen
    if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS) {
        camera.Position = glm::vec3(3.0f, 0.0f, 0.0f);
        camera.View     = glm::vec3(0.0f, 0.0f, 0.0f);
        camera.Up       = glm::vec3(0.0f, 1.0f, 0.0f);
    }
    // vista superior (F3): cámara en +Y mirando al origen
    if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS) {
        camera.Position = glm::vec3(0.0f, 3.0f, 0.0f);
        camera.View     = glm::vec3(0.0f, 0.0f, 0.0f);
        camera.Up       = glm::vec3(0.0f, 0.0f, -1.0f);
    }

    // Ejercicio 5. Movimiento de la cámara
    // Standard keys for 3D movement
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.Position.y += 0.01f;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.Position.y -= 0.01f;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.Position.x -= 0.01f;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.Position.x += 0.01f;
    }

    // Ejercicio 6. Movimiento del objeto 3D
    // Arrows
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        modelPosition.Position.y += 0.01f;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        modelPosition.Position.y -= 0.01f;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        modelPosition.Position.x -= 0.01f;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        modelPosition.Position.x += 0.01f;
    }

    // ejercicio 7. Escalamientos
    // teclado numerico (+ y -)
    if (glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS) {
        modelScale -= 0.01f;
        if (modelScale < 0.05f) modelScale = 0.05f;
    }
    if (glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS) {
        modelScale += 0.01f;
    }
    // Alternativa: PageUp (aumenta) y PageDown (disminuye)
    if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS) {
        modelScale -= 0.01f;
        if (modelScale < 0.05f) modelScale = 0.05f;
    }
    if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS) {
        modelScale += 0.01f;
    }

    // User defined
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {

    }
    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) {

    }
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {

    }

}

void resizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}