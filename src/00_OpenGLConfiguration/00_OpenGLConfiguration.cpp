/*
* 
* 00 - Configuración de OpenGL
* Based on: https://www.glfw.org/documentation.html
*/

#include <stdlib.h>
#include <stdio.h>

#include <GLFW/glfw3.h>
#include <iostream>
#include <sstream>

using namespace std;

int main(void)
{
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;
    
    /* Maximize window */
    // glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

    /* Transparent framebuffer */
    // glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Define the window position in screen coordinates */
    // glfwSetWindowPos(window, 50, 50);

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    /* Graphics device information */
    cout << "> Version: " << glGetString(GL_VERSION) << endl;
    cout << "> Vendor: " << glGetString(GL_VENDOR) << endl;
    cout << "> Renderer: " << glGetString(GL_RENDERER) << endl;
    cout << "> Supported Extensions: " << glGetString(GL_EXTENSIONS) << endl;

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT);

        /* Change the background Color */
        glClearColor(0.4f, 0.0f, 0.4f, 1.0f);
        ///            R    G     B    alpha

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    glfwTerminate();

    return 0;
}