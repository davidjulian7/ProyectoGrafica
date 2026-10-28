/*
* Laboratorio de Computación Gráfica e Interacción Humano-Computadora
* Proyecto Final - Mundo Submarino
*
* Escena acuática que integra las técnicas vistas durante el curso:
*  - Carga de modelos 3D (Assimp)
*  - Iluminación Phong con múltiples luces
*  - Cubemap (fondo submarino)
*  - Modelo de Fresnel (superficie del agua)
*  - Animación de partículas (burbujas)
*  - Animación procedural (peces nadando en círculos)
*  - Reflexión (espejo) en el piso del océano
*  - Picking con el mouse (seleccionar peces)
*  - Audio ambiental (irrKlang)
*/

#include <iostream>
#include <stdlib.h>
#include <sstream>
#include <vector>
#include <string>

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
#include <shader_m.h>
#include <camera.h>
#include <model.h>
#include <material.h>
#include <light.h>
#include <cubemap.h>
#include <particles.h>

#include <irrKlang.h>
using namespace irrklang;

// Functions
bool Start();
bool UpdateMirrorFramebuffer();
bool UpdatePickingProcess();
bool Update();

// Definición de callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

// Gobals
GLFWwindow* window;

// Tamaño en pixeles de la ventana
const unsigned int SCR_WIDTH = 1024;
const unsigned int SCR_HEIGHT = 768;

// Definición de cámara (posición en XYZ)
Camera camera(glm::vec3(0.0f, 3.0f, 12.0f));
Camera mirrorCamera(glm::vec3(0.0f, 0.0f, 0.0f));

// Controladores para el movimiento del mouse
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// Variables para la velocidad de reproducción de la animación
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float elapsedTime = 0.0f;

// Shaders
Shader* cubemapShader;
Shader* mLightsShader;
Shader* fresnelShader;
Shader* particlesShader;
Shader* mirrorShader;
Shader* pickingShader;

// Modelos de la escena
Model* oceanFloor;    // piso del océano (reflejo/espejo)
Model* fish01;        // pez payaso 1
Model* fish02;        // pez payaso 2
Model* fish03;        // pez payaso 3
Model* bubbleModel;   // modelo de burbuja (partícula)

// Cubemap
CubeMap* mainCubeMap;

// Sistema de partículas (burbujas subiendo)
Particles bubbles(150);

// Fuentes de luz
std::vector<Light> gLights;

// Materiales
Material sandMaterial;
Material fishMaterial;

// Animación de los peces (parámetros de órbita circular)
struct FishOrbit {
	float radius;      // radio de la órbita
	float speed;       // velocidad angular
	float phase;       // fase inicial
	float height;      // altura (Y) base
	float bobAmp;      // amplitud del movimiento de "bob"
	float bobSpeed;    // velocidad del bob
	float scale;       // escala del modelo
	unsigned int pickingID; // ID para picking
	Model* model;      // puntero al modelo
};

std::vector<FishOrbit> fishes;

// Tiempo para animaciones
float animTime = 0.0f;

// Picking
int pickedFish = -1; // índice del pez seleccionado, -1 = ninguno

// Audio
ISoundEngine* SoundEngine = createIrrKlangDevice();

// ---------------------------------------------------------------------------
// Helpers para uniformes de luces (iguales a los de la práctica 13)
// ---------------------------------------------------------------------------
void SetLightUniformInt(Shader* shader, const char* propertyName, size_t lightIndex, int value) {
	std::ostringstream ss;
	ss << "allLights[" << lightIndex << "]." << propertyName;
	shader->setInt(ss.str().c_str(), value);
}
void SetLightUniformFloat(Shader* shader, const char* propertyName, size_t lightIndex, float value) {
	std::ostringstream ss;
	ss << "allLights[" << lightIndex << "]." << propertyName;
	shader->setFloat(ss.str().c_str(), value);
}
void SetLightUniformVec4(Shader* shader, const char* propertyName, size_t lightIndex, glm::vec4 value) {
	std::ostringstream ss;
	ss << "allLights[" << lightIndex << "]." << propertyName;
	shader->setVec4(ss.str().c_str(), value);
}
void SetLightUniformVec3(Shader* shader, const char* propertyName, size_t lightIndex, glm::vec3 value) {
	std::ostringstream ss;
	ss << "allLights[" << lightIndex << "]." << propertyName;
	shader->setVec3(ss.str().c_str(), value);
}

// ---------------------------------------------------------------------------
// Entrada a función principal
// ---------------------------------------------------------------------------
int main()
{
	if (!Start())
		return -1;

	// Loop principal: primero el framebuffer del espejo, luego picking y render normal
	while (!glfwWindowShouldClose(window))
	{
		if (!UpdateMirrorFramebuffer()) break;
		if (!UpdatePickingProcess()) break;
		if (!Update()) break;
	}

	glfwTerminate();
	return 0;
}

// ---------------------------------------------------------------------------
// Inicialización
// ---------------------------------------------------------------------------
bool Start() {
	// Inicialización de GLFW
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Creación de la ventana con GLFW
	window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Proyecto Final - Mundo Submarino", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouseButtonCallback);

	// glad: Cargar todos los apuntadores
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return false;
	}

	// Activación de buffer de profundidad
	glEnable(GL_DEPTH_TEST);

	// Compilación y enlace de shaders
	cubemapShader = new Shader("shaders/10_vertex_cubemap.vs", "shaders/10_fragment_cubemap.fs");
	mLightsShader = new Shader("shaders/11_PhongShaderMultLights.vs", "shaders/11_PhongShaderMultLights.fs");
	fresnelShader = new Shader("shaders/11_Fresnel.vs", "shaders/11_Fresnel.fs");
	particlesShader = new Shader("shaders/13_particles.vs", "shaders/13_particles.fs");
	mirrorShader = new Shader("shaders/14_mirror.vs", "shaders/14_mirror.fs");
	pickingShader = new Shader("shaders/15_picking.vs", "shaders/15_picking.fs");

	// Modelos (PezPayaso.fbx está en models/)
	oceanFloor = new Model("models/plane.fbx", 100);
	fish01 = new Model("models/PezPayaso.fbx", 201);
	fish02 = new Model("models/PezPayaso.fbx", 202);
	fish03 = new Model("models/PezPayaso.fbx", 203);
	bubbleModel = new Model("models/snow/snow.fbx");

	// Configuración de órbitas de los peces (animación procedural)
	fishes.push_back({ 4.0f, 0.4f, 0.0f, 2.0f, 0.3f, 1.2f, 0.15f, 201, fish01 });
	fishes.push_back({ 6.0f, -0.3f, 2.0f, 3.0f, 0.4f, 0.9f, 0.20f, 202, fish02 });
	fishes.push_back({ 3.0f, 0.5f, 4.0f, 1.5f, 0.25f, 1.5f, 0.12f, 203, fish03 });

	// Cubemap (fondo submarino - se puede sustituir por un cubemap azul/acuático)
	vector<std::string> faces
	{
		"textures/cubemap/01/posx.png",
		"textures/cubemap/01/negx.png",
		"textures/cubemap/01/posy.png",
		"textures/cubemap/01/negy.png",
		"textures/cubemap/01/posz.png",
		"textures/cubemap/01/negz.png"
	};
	mainCubeMap = new CubeMap();
	mainCubeMap->loadCubemap(faces);

	// Configuración del sistema de partículas (burbujas que suben)
	// gravedad negativa hacia arriba para simular flotabilidad
	bubbles.setGravity(glm::vec3(0.0f, 0.15f, 0.0f));

	// Materiales
	sandMaterial.ambient = glm::vec4(0.2f, 0.25f, 0.3f, 1.0f);
	sandMaterial.diffuse = glm::vec4(0.6f, 0.65f, 0.5f, 1.0f);
	sandMaterial.specular = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
	sandMaterial.shininess = 8.0f;
	sandMaterial.transparency = 1.0f;

	fishMaterial.ambient = glm::vec4(0.3f, 0.2f, 0.1f, 1.0f);
	fishMaterial.diffuse = glm::vec4(0.9f, 0.5f, 0.2f, 1.0f);
	fishMaterial.specular = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
	fishMaterial.shininess = 32.0f;
	fishMaterial.transparency = 1.0f;

	// Configuración de luces (tonos azulados/acuáticos)
	Light light01;
	light01.Position = glm::vec3(8.0f, 6.0f, 8.0f);
	light01.Color = glm::vec4(0.3f, 0.5f, 0.8f, 1.0f); // azul luz solar filtrada
	gLights.push_back(light01);

	Light light02;
	light02.Position = glm::vec3(-8.0f, 4.0f, 6.0f);
	light02.Color = glm::vec4(0.1f, 0.3f, 0.5f, 1.0f); // azul profundo
	gLights.push_back(light02);

	Light light03;
	light03.Position = glm::vec3(6.0f, 2.0f, -8.0f);
	light03.Color = glm::vec4(0.0f, 0.4f, 0.4f, 1.0f); // cian
	gLights.push_back(light03);

	Light light04;
	light04.Position = glm::vec3(-6.0f, 5.0f, -6.0f);
	light04.Color = glm::vec4(0.2f, 0.2f, 0.6f, 1.0f); // violeta marino
	gLights.push_back(light04);

	// Audio ambiental submarino (opcional - field.mp3 ya existe en sound/)
	// SoundEngine->play2D("sound/field.mp3", true);

	std::cout << "============================================" << std::endl;
	std::cout << "  PROYECTO FINAL - MUNDO SUBMARINO" << std::endl;
	std::cout << "============================================" << std::endl;
	std::cout << "Controles:" << std::endl;
	std::cout << "  WASD     - Mover cámara" << std::endl;
	std::cout << "  Mouse    - Rotar cámara" << std::endl;
	std::cout << "  Click    - Seleccionar pez (picking)" << std::endl;
	std::cout << "  M/N/B    - Wireframe / Fill / Points" << std::endl;
	std::cout << "  F1       - Activar sonido ambiental" << std::endl;
	std::cout << "  ESC      - Salir" << std::endl;
	std::cout << "============================================" << std::endl;

	return true;
}

// ---------------------------------------------------------------------------
// Render al framebuffer del espejo (reflexión del piso) - técnica de la práctica 12
// ---------------------------------------------------------------------------
bool UpdateMirrorFramebuffer() {

	if (!oceanFloor->renderFramebufferToTexture()) {
		printf("Render buffer error");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, oceanFloor->framebuffer_gl);

	// Renderizado R - G - B - A
	glClearColor(0.0f, 0.2f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 projection;
	glm::mat4 view;

	// Cámara espejo: refleja la vista a través del plano del piso (Y=0)
	glm::vec3 L = camera.Front;
	glm::vec3 N = glm::vec3(0.0f, 1.0f, 0.0f); // normal del piso (hacia arriba)
	glm::vec3 vreflected = L - 2.0f * glm::dot(L, N) * N;

	mirrorCamera.Position = glm::vec3(camera.Position.x, -camera.Position.y, camera.Position.z);
	mirrorCamera.Front = vreflected;
	mirrorCamera.Up = glm::vec3(0.0f, 1.0f, 0.0f);

	projection = glm::perspective(glm::radians(mirrorCamera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 10000.0f);
	view = mirrorCamera.GetViewMatrix();

	// Cubemap (fondo en el reflejo)
	{
		mainCubeMap->drawCubeMap(*cubemapShader, projection, view);
	}

	glUseProgram(0);

	// Dibujo de los peces en el reflejo (se ven "al revés" en el piso)
	{
		mLightsShader->use();

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		mLightsShader->setMat4("projection", projection);
		mLightsShader->setMat4("view", view);

		// Configuramos propiedades de fuentes de luz
		mLightsShader->setInt("numLights", (int)gLights.size());
		for (size_t i = 0; i < gLights.size(); ++i) {
			SetLightUniformVec3(mLightsShader, "Position", i, gLights[i].Position);
			SetLightUniformVec3(mLightsShader, "Direction", i, gLights[i].Direction);
			SetLightUniformVec4(mLightsShader, "Color", i, gLights[i].Color);
			SetLightUniformVec4(mLightsShader, "Power", i, gLights[i].Power);
			SetLightUniformInt(mLightsShader, "alphaIndex", i, gLights[i].alphaIndex);
			SetLightUniformFloat(mLightsShader, "distance", i, gLights[i].distance);
		}

		mLightsShader->setVec3("eye", mirrorCamera.Position);

		// Aplicamos propiedades materiales
		mLightsShader->setVec4("MaterialAmbientColor", fishMaterial.ambient);
		mLightsShader->setVec4("MaterialDiffuseColor", fishMaterial.diffuse);
		mLightsShader->setVec4("MaterialSpecularColor", fishMaterial.specular);
		mLightsShader->setFloat("transparency", fishMaterial.transparency);

		// Dibujamos cada pez en su posición orbital actual
		for (size_t i = 0; i < fishes.size(); ++i) {
			FishOrbit& f = fishes[i];

			float angle = animTime * f.speed + f.phase;
			float x = cos(angle) * f.radius;
			float z = sin(angle) * f.radius;
			float y = f.height + sin(animTime * f.bobSpeed + f.phase) * f.bobAmp;

			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(x, y, z));

			// Rotar el pez para que mire en la dirección del movimiento
			float heading = atan2f(cos(angle), -sin(angle)); // tangente a la órbita
			model = glm::rotate(model, heading, glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			model = glm::scale(model, glm::vec3(f.scale, f.scale, f.scale));

			mLightsShader->setMat4("model", model);
			f.model->Draw(*mLightsShader);
		}
	}

	glUseProgram(0);

	//glfwSwapBuffers(window);
	//glfwPollEvents();

	return true;
}

// ---------------------------------------------------------------------------
// Proceso de picking: se renderiza la escena con colores de ID y se lee el pixel
// central al hacer click - técnica de la práctica 13
// ---------------------------------------------------------------------------
bool UpdatePickingProcess()
{
	// Nota: aquí solo preparamos el estado; el picking real ocurre en mouseButtonCallback
	// después de que Update() haya dibujado la escena con pickingShader en el framebuffer.
	// En este proyecto simplificamos: el picking se procesa en Update() mediante un
	// segundo framebuffer... pero para mantenerlo simple usamos el enfoque de la práctica:
	// dibujamos con pickingShader y leemos el píxel central en el callback del mouse.
	return true;
}

// ---------------------------------------------------------------------------
// Render principal de la escena
// ---------------------------------------------------------------------------
bool Update() {
	// Cálculo del framerate
	float currentFrame = (float)glfwGetTime();
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;

	// Acumulamos tiempo para animaciones
	animTime += deltaTime;

	elapsedTime += deltaTime;
	if (elapsedTime > 1.0f / 30.0f) {
		elapsedTime = 0.0f;
		bubbles.UpdatePhysics(deltaTime);
	}

	// Procesa la entrada del teclado o mouse
	processInput(window);

	// Renderizado principal (pantalla)
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Color de fondo azul profundo (mar)
	glClearColor(0.0f, 0.15f, 0.35f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 10000.0f);
	glm::mat4 view = camera.GetViewMatrix();

	// 1) Cubemap (fondo submarino)
	{
		mainCubeMap->drawCubeMap(*cubemapShader, projection, view);
	}

	// 2) Piso del océano con efecto espejo (reflexión)
	{
		mirrorShader->use();

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		mirrorShader->setMat4("projection", projection);
		mirrorShader->setMat4("view", view);

		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(20.0f, 20.0f, 20.0f)); // piso grande
		mirrorShader->setMat4("model", model);

		oceanFloor->DrawMirror(*mirrorShader);
	}

	glUseProgram(0);

	// 3) Peces con iluminación Phong de múltiples luces
	{
		mLightsShader->use();

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		mLightsShader->setMat4("projection", projection);
		mLightsShader->setMat4("view", view);

		// Configuramos propiedades de fuentes de luz
		mLightsShader->setInt("numLights", (int)gLights.size());
		for (size_t i = 0; i < gLights.size(); ++i) {
			SetLightUniformVec3(mLightsShader, "Position", i, gLights[i].Position);
			SetLightUniformVec3(mLightsShader, "Direction", i, gLights[i].Direction);
			SetLightUniformVec4(mLightsShader, "Color", i, gLights[i].Color);
			SetLightUniformVec4(mLightsShader, "Power", i, gLights[i].Power);
			SetLightUniformInt(mLightsShader, "alphaIndex", i, gLights[i].alphaIndex);
			SetLightUniformFloat(mLightsShader, "distance", i, gLights[i].distance);
		}

		mLightsShader->setVec3("eye", camera.Position);

		// Material de los peces
		mLightsShader->setVec4("MaterialAmbientColor", fishMaterial.ambient);
		mLightsShader->setVec4("MaterialDiffuseColor", fishMaterial.diffuse);
		mLightsShader->setVec4("MaterialSpecularColor", fishMaterial.specular);
		mLightsShader->setFloat("transparency", fishMaterial.transparency);

		// Dibujamos cada pez con animación orbital
		for (size_t i = 0; i < fishes.size(); ++i) {
			FishOrbit& f = fishes[i];

			float angle = animTime * f.speed + f.phase;
			float x = cos(angle) * f.radius;
			float z = sin(angle) * f.radius;
			float y = f.height + sin(animTime * f.bobSpeed + f.phase) * f.bobAmp;

			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(x, y, z));

			// Rotación para que el pez mire en la dirección del movimiento (tangente a la órbita)
			float heading = atan2f(cos(angle), -sin(angle));
			model = glm::rotate(model, heading, glm::vec3(0.0f, 1.0f, 0.0f));
			// Los FBX suelen venir rotados -90° en X
			model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

			// Si el pez está seleccionado, hacerlo un poco más grande (feedback visual)
			float s = f.scale;
			if ((int)i == pickedFish) s *= 1.3f;
			model = glm::scale(model, glm::vec3(s, s, s));

			mLightsShader->setMat4("model", model);
			f.model->Draw(*mLightsShader);
		}
	}

	glUseProgram(0);

	// 4) Superficie del agua con efecto Fresnel (transparencia/refracción)
	{
		fresnelShader->use();

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		fresnelShader->setMat4("projection", projection);
		fresnelShader->setMat4("view", view);

		// Plano del agua en la parte superior (Y alto) simulando la superficie
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 10.0f, 0.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(30.0f, 30.0f, 1.0f));
		fresnelShader->setMat4("model", model);
		fresnelShader->setVec3("cameraPosition", camera.Position);
		fresnelShader->setFloat("mRefractionRatio", 1.0f / 1.333f); // 1.333 Agua
		fresnelShader->setFloat("_Bias", 0.02f);
		fresnelShader->setFloat("_Scale", 0.5f);
		fresnelShader->setFloat("_Power", 2.0f);

		// Usamos el piso como geometría del plano del agua (es un plano)
		// Nota: si plane.fbx tiene textura se mezclará; el efecto Fresnel domina
		oceanFloor->Draw(*fresnelShader);
	}

	glUseProgram(0);

	// 5) Partículas: burbujas subiendo
	{
		particlesShader->use();
		particlesShader->setMat4("projection", projection);
		particlesShader->setMat4("view", view);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		for (int psc = 0; psc < (int)bubbles.particles.size(); psc++) {
			Particle p_i = bubbles.particles.at(psc);

			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, p_i.position);
			model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));

			particlesShader->setMat4("model", model);
			bubbleModel->Draw(*particlesShader);
		}
	}

	glUseProgram(0);

	// 6) Picking: renderizar la escena con colores de ID y leer el píxel del centro
	// (se ejecuta al final para no interferir con el render visual normal)
	{
		// Limpiar solo el buffer de color para el pase de picking
		// (en la práctica 13 se hace un pase completo; aquí lo integramos)
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // blanco = fondo
		glClear(GL_COLOR_BUFFER_BIT);

		pickingShader->use();
		pickingShader->setMat4("projection", projection);
		pickingShader->setMat4("view", view);

		// Piso (ID 100)
		{
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::scale(model, glm::vec3(20.0f, 20.0f, 20.0f));
			pickingShader->setMat4("model", model);

			int r = (oceanFloor->IDforPicking & 0x000000FF) >> 0;
			int g = (oceanFloor->IDforPicking & 0x0000FF00) >> 8;
			int b = (oceanFloor->IDforPicking & 0x00FF0000) >> 16;
			pickingShader->setVec4("PickingColor", glm::vec4(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f));
			oceanFloor->Draw(*pickingShader);
		}

		// Peces (IDs 201, 202, 203)
		for (size_t i = 0; i < fishes.size(); ++i) {
			FishOrbit& f = fishes[i];

			float angle = animTime * f.speed + f.phase;
			float x = cos(angle) * f.radius;
			float z = sin(angle) * f.radius;
			float y = f.height + sin(animTime * f.bobSpeed + f.phase) * f.bobAmp;

			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(x, y, z));
			float heading = atan2f(cos(angle), -sin(angle));
			model = glm::rotate(model, heading, glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			model = glm::scale(model, glm::vec3(f.scale, f.scale, f.scale));
			pickingShader->setMat4("model", model);

			int r = (f.pickingID & 0x000000FF) >> 0;
			int g = (f.pickingID & 0x0000FF00) >> 8;
			int b = (f.pickingID & 0x00FF0000) >> 16;
			pickingShader->setVec4("PickingColor", glm::vec4(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f));
			f.model->Draw(*pickingShader);
		}

		glUseProgram(0);

		// Leer el píxel del centro (como en la práctica 13)
		unsigned char data[4];
		glReadPixels(SCR_WIDTH / 2, SCR_HEIGHT / 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);
		unsigned int pickedID = data[0] + data[1] * 256 + data[2] * 256 * 256;

		if (pickedID == 0x00ffffff) {
			// Fondo - no hay selección
			pickedFish = -1;
		}
		else {
			// Buscar qué pez corresponde al ID
			for (size_t i = 0; i < fishes.size(); ++i) {
				if (fishes[i].pickingID == pickedID) {
					pickedFish = (int)i;
					std::cout << "Pez #" << (i + 1) << " seleccionado! (ID: " << pickedID << ")" << std::endl;
					break;
				}
			}
			if (pickedID == 100) {
				std::cout << "Piso del oceano seleccionado (ID: 100)" << std::endl;
				pickedFish = -1;
			}
		}
	}

	// glfw: swap buffers
	glfwSwapBuffers(window);
	glfwPollEvents();

	return true;
}

// ---------------------------------------------------------------------------
// Procesamos entradas del teclado
// ---------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);

	// Activar sonido ambiental con F1
	if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS) {
		static bool soundOn = false;
		if (!soundOn) {
			SoundEngine->play2D("sound/field.mp3", true);
			soundOn = true;
			std::cout << "Sonido ambiental activado" << std::endl;
		}
	}
}

// ---------------------------------------------------------------------------
// Callback del botón del mouse (picking)
// ---------------------------------------------------------------------------
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		// El picking se procesa en Update() leyendo el píxel central.
		// Aquí solo imprimimos confirmación si hay un pez seleccionado.
		if (pickedFish >= 0) {
			std::cout << ">> Pez #" << (pickedFish + 1) << " activo. Es más grande ahora." << std::endl;
		}
	}
}

// ---------------------------------------------------------------------------
// Callbacks de GLFW
// ---------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = (float)xpos;
		lastY = (float)ypos;
		firstMouse = false;
	}

	float xoffset = (float)xpos - lastX;
	float yoffset = lastY - (float)ypos;

	lastX = (float)xpos;
	lastY = (float)ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll((float)yoffset);
}
