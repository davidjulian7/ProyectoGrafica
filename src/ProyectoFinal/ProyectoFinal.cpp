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
#include <menu.h>          // menú de inicio y HUD de zonas
#include <zonas.h>         // contenido y render de las tres zonas
#include <cmath>

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
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

// Menú de inicio y zonas
bool UpdateMenu();
void EntrarZona(int zona);
void VolverAlMenu();

// Gobals
GLFWwindow* window;

// ---------------------------------------------------------------------------
// Estados del programa: el menú de inicio y las tres zonas de inmersión
// ---------------------------------------------------------------------------
enum Estado { MENU = 0, ARRECIFES = 1, AGUAS_FRIAS = 2, NAUFRAGIOS = 3 };
Estado estadoActual = MENU;
const char* nombresZona[] = { "Menu", "Zona 1 - Arrecifes", "Zona 2 - Aguas Frias", "Zona 3 - Naufragios" };

MenuInicio menuInicio;      // menú de inicio (include/menu.h)
float zonaFade = 0.0f;      // fundido de entrada a una zona: 1 = negro, 0 = visible
float zonaTiempo = 0.0f;    // segundos dentro de la zona actual

Escenario escenario;        // zonas de inmersión (include/zonas.h)

// --- Buzo: nado con inercia (etapa 1) ---
glm::vec3 diverVelocity(0.0f);
const float DIVER_ACCEL = 6.0f;   // qué tan rápido arranca al aletear
const float DIVER_DRAG  = 2.5f;   // resistencia del agua (frena al soltar las teclas)

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
		if (estadoActual == MENU) {
			// Menú de inicio: elegir una de las tres zonas
			if (!UpdateMenu()) break;
		}
		else {
			// Dentro de una zona: picking de la mira + render de la zona
			if (!Update()) break;
		}
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
	window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Acuario Virtual Interactivo", NULL, NULL);
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
	glfwSetKeyCallback(window, key_callback);

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

	// Menú de inicio (shaders y texturas en shaders/menu_* y textures/menu/)
	if (!menuInicio.Init())
		std::cout << "[Menu] Se cargo con errores, revisa los mensajes anteriores." << std::endl;

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

	// Cubemap submarino (generado proceduralmente; el 01 de las prácticas era un bosque)
	vector<std::string> faces
	{
		"textures/cubemap/submarino/posx.png",
		"textures/cubemap/submarino/negx.png",
		"textures/cubemap/submarino/posy.png",
		"textures/cubemap/submarino/negy.png",
		"textures/cubemap/submarino/posz.png",
		"textures/cubemap/submarino/negz.png"
	};
	mainCubeMap = new CubeMap();
	mainCubeMap->loadCubemap(faces);

	// Zonas de inmersión (los modelos de cada zona se cargan al entrar a ella)
	escenario.Init(mainCubeMap->textureID);

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
	std::cout << "  ACUARIO VIRTUAL INTERACTIVO" << std::endl;
	std::cout << "============================================" << std::endl;
	std::cout << "Menu de inicio:" << std::endl;
	std::cout << "  1 / 2 / 3       - Entrar a una zona" << std::endl;
	std::cout << "  Flechas + Enter - Navegar y entrar" << std::endl;
	std::cout << "  Clic            - Entrar a la zona bajo el cursor" << std::endl;
	std::cout << "Dentro de una zona:" << std::endl;
	std::cout << "  WASD     - Nadar (Shift: aletear fuerte)" << std::endl;
	std::cout << "  Espacio  - Subir      Ctrl / C - Bajar" << std::endl;
	std::cout << "  Mouse    - Mirar alrededor" << std::endl;
	std::cout << "  Click    - Interactuar con lo que apunta la mira" << std::endl;
	std::cout << "  F        - Linterna" << std::endl;
	std::cout << "  M/N/B    - Wireframe / Fill / Points" << std::endl;
	std::cout << "  F1       - Activar sonido ambiental" << std::endl;
	std::cout << "  ESC      - Volver al menu (en el menu: salir)" << std::endl;
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
	// Tiempo entre cuadros
	float currentFrame = (float)glfwGetTime();
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;
	animTime += deltaTime;
	zonaTiempo += deltaTime;

	// Nado del buzo (teclado) y animaciones de la zona (burbujas)
	processInput(window);
	escenario.Actualizar(deltaTime, animTime, camera);

	// Vaivén de la vista: respiración siempre presente + aleteo al nadar
	float nado = glm::length(diverVelocity);
	camera.BobOffset = glm::vec3(0.0f, sinf(animTime * 1.3f) * 0.04f + sinf(animTime * 4.0f) * 0.02f * nado, 0.0f);

	int fbW, fbH;
	glfwGetFramebufferSize(window, &fbW, &fbH);
	if (fbW == 0 || fbH == 0) { glfwPollEvents(); return true; }   // ventana minimizada
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, fbW, fbH);
	glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)fbW / (float)fbH, 0.1f, 400.0f);
	glm::mat4 view = camera.GetViewMatrix();

	// 1) Picking: qué hay en el centro de la pantalla (la mira del buzo)
	escenario.Picking(projection, view, animTime, fbW, fbH);

	// 2) Escena de la zona: agua, terreno, fauna, superficie y burbujas
	escenario.Dibujar(projection, view, camera, animTime, fbW, fbH);

	// 3) HUD: nombre de la zona, ayuda, mira y fundido de entrada
	menuInicio.DrawHUD(window, (int)estadoActual, zonaFade, zonaTiempo);
	const Elemento* sel = escenario.Seleccionado();
	menuInicio.DrawMira(window, sel ? sel->etiqueta : std::string(), sel && sel->basura);
	zonaFade = zonaFade - deltaTime * 1.4f;
	if (zonaFade < 0.0f) zonaFade = 0.0f;

	glUseProgram(0);
	glfwSwapBuffers(window);
	glfwPollEvents();
	return true;
}

// ---------------------------------------------------------------------------
// Procesamos entradas del teclado
// ---------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
	// Esc se maneja en key_callback (volver al menú / salir)

	// Nado del buzo: las teclas empujan una velocidad y el agua la frena poco a poco
	glm::vec3 wish(0.0f);
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) wish += camera.Front;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) wish -= camera.Front;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) wish -= camera.Right;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) wish += camera.Right;
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) wish.y += 1.0f;          // subir
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
	    glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) wish.y -= 1.0f;                // bajar
	if (glm::length(wish) > 0.0f) wish = glm::normalize(wish);
	float aleteo = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ? 2.2f : 1.0f;

	diverVelocity += wish * DIVER_ACCEL * aleteo * deltaTime;
	diverVelocity *= std::exp(-DIVER_DRAG * deltaTime);
	camera.Position += diverVelocity * deltaTime;

	// Límites: no atravesar el fondo, no salir del agua, no salir de la zona
	float suelo = escenario.AlturaSuelo(camera.Position.x, camera.Position.z) + 1.0f;
	if (camera.Position.y < suelo) { camera.Position.y = suelo; if (diverVelocity.y < 0.0f) diverVelocity.y = 0.0f; }
	float techo = escenario.cfg.superficieY - 0.6f;
	if (camera.Position.y > techo) { camera.Position.y = techo; if (diverVelocity.y > 0.0f) diverVelocity.y = 0.0f; }
	glm::vec2 xz(camera.Position.x, camera.Position.z);
	if (glm::length(xz) > 42.0f) {
		xz = glm::normalize(xz) * 42.0f;
		camera.Position.x = xz.x; camera.Position.z = xz.y;
	}

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
	if (estadoActual == MENU) {
		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
			menuInicio.OnClick();
		return;
	}
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		escenario.Interactuar();   // recoger basura o ver el nombre del animal
		return;
	}
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
	// En el menú el ratón no gira la cámara (el menú lee el cursor por su cuenta)
	if (estadoActual == MENU) {
		firstMouse = true;
		return;
	}

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
	if (estadoActual == MENU) return;
	camera.ProcessMouseScroll((float)yoffset);
}

// ---------------------------------------------------------------------------
// Teclado por eventos: una pulsación = una acción (no se repite al mantener)
// ---------------------------------------------------------------------------
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action != GLFW_PRESS) return;

	if (estadoActual == MENU) {
		if (key == GLFW_KEY_ESCAPE)
			glfwSetWindowShouldClose(window, true);
		else
			menuInicio.OnKey(key);
		return;
	}

	// Dentro de una zona: Esc regresa al menú de inicio, F enciende/apaga la linterna
	if (key == GLFW_KEY_ESCAPE)
		VolverAlMenu();
	else if (key == GLFW_KEY_F) {
		escenario.linterna = !escenario.linterna;
		std::cout << (escenario.linterna ? ">> Linterna encendida" : ">> Linterna apagada") << std::endl;
	}
}

// ---------------------------------------------------------------------------
// Menú de inicio
// ---------------------------------------------------------------------------
bool UpdateMenu()
{
	float currentFrame = (float)glfwGetTime();
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;

	int zona = menuInicio.Update(window, deltaTime);
	menuInicio.Draw(window, currentFrame);

	glfwSwapBuffers(window);
	glfwPollEvents();

	if (zona > 0)
		EntrarZona(zona);
	return true;
}

void EntrarZona(int zona)
{
	estadoActual = (Estado)zona;

	// Carga la zona (la primera vez lee sus modelos) y coloca al buzo en su punto de inicio
	escenario.Entrar(zona, camera);
	diverVelocity = glm::vec3(0.0f);
	firstMouse = true;
	pickedFish = -1;
	zonaTiempo = 0.0f;
	lastFrame = (float)glfwGetTime();   // la carga no cuenta como tiempo de nado

	// El cursor desaparece: el centro de la pantalla es la mira del buzo
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	zonaFade = 1.0f;   // la zona aparece desde negro
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	std::string titulo = std::string("Acuario Virtual Interactivo - ") + nombresZona[zona];
	glfwSetWindowTitle(window, titulo.c_str());
	std::cout << ">> Entrando a " << nombresZona[zona] << " (Esc para volver al menu)" << std::endl;
}

void VolverAlMenu()
{
	estadoActual = MENU;
	menuInicio.Reiniciar();
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glfwSetWindowTitle(window, "Acuario Virtual Interactivo");
	std::cout << ">> Menu de inicio" << std::endl;
}
