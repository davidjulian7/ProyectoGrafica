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
#include <arrecife.h>      // Zona 1: termómetro y blanqueamiento de coral
#include <naufragios.h>    // Zona 3: limpieza, pez león, red fantasma y cofre
#include <aguas_frias.h>    // Zona 2: termómetro, deshielo, fantasmas, fotos y cadena trófica
#include <cmath>

#include <irrKlang.h>
using namespace irrklang;

// Functions
bool Start();
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
MecanicaArrecife arrecife;  // mecánica de la Zona 1 (include/arrecife.h)
MecanicaNaufragios naufragio; // mecánica de la Zona 3 (include/naufragios.h)
MecanicaAguasFrias aguasFrias; // mecánica de la Zona 2 (include/aguas_frias.h)

// --- Buzo: nado con inercia (etapa 1) ---
glm::vec3 diverVelocity(0.0f);
const float DIVER_ACCEL = 3.5f;   // qué tan rápido arranca al aletear (velocidad máxima ~1.4 m/s)
const float DIVER_DRAG  = 2.5f;   // resistencia del agua (frena al soltar las teclas)

// Tamaño en pixeles de la ventana
const unsigned int SCR_WIDTH = 1024;
const unsigned int SCR_HEIGHT = 768;

// Definición de cámara (posición en XYZ)
Camera camera(glm::vec3(0.0f, 3.0f, 12.0f));
Camera mirrorCamera(glm::vec3(0.0f, 0.0f, 0.0f));

// Segunda cámara: órbita detrás del buzo (tecla V)
bool  modoOrbita = false;
float orbitDist  = 6.0f;                 // rango 3..20 (scroll)
Camera cameraOrbita(glm::vec3(0.0f, 3.0f, 18.0f));

//Variable para cambiar entre cámara perspectiva y ortogonal (tecla 5)
bool modoOrtogonal = false;

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
ISound* sonidoAmbiente = nullptr;   // ambiente en bucle de la zona actual
bool silencio = false;
bool ambienteInspeccionPausado = false;

// Cambia el sonido ambiente (nullptr = detener)
void Ambiente(const char* archivo, float volumen)
{
	if (!SoundEngine) return;
	if (sonidoAmbiente) { sonidoAmbiente->stop(); sonidoAmbiente->drop(); sonidoAmbiente = nullptr; }
	if (archivo) {
		sonidoAmbiente = SoundEngine->play2D(archivo, true, false, true);
		if (sonidoAmbiente) sonidoAmbiente->setVolume(volumen);
	}
}

// Efecto corto (burbujas, alerta, descubrimiento)
void Efecto(const char* archivo, float volumen)
{
	if (!SoundEngine || silencio) return;
	ISound* s = SoundEngine->play2D(archivo, false, false, true);
	if (s) { s->setVolume(volumen); s->drop(); }
}

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
	// La escena de prueba de las prácticas ya no se carga: cada zona compila sus propios shaders


	// Menú de inicio (shaders y texturas en shaders/menu_* y textures/menu/)
	if (!menuInicio.Init())
		std::cout << "[Menu] Se cargo con errores, revisa los mensajes anteriores." << std::endl;


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
	Ambiente("sound/ambiente_arrecife.wav", 0.25f);   // ambiente suave en el menú


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
	std::cout << "  V        - Camara en orbita (rueda: distancia)" << std::endl;
	std::cout << "  Click    - Interactuar con lo que apunta la mira" << std::endl;
	std::cout << "  E        - Modo edicion de objetos (Tab: ciclar, F5: guardar pose)" << std::endl;
	std::cout << "  F        - Linterna" << std::endl;
	std::cout << "  L        - Lista de especies por registrar" << std::endl;
	std::cout << "  M/N/B    - Wireframe / Fill / Points" << std::endl;
	std::cout << "  F1       - Activar sonido ambiental" << std::endl;
	std::cout << "  ESC      - Volver al menu (en el menu: salir)" << std::endl;
	std::cout << "============================================" << std::endl;

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
	bool inspeccion = estadoActual == ARRECIFES && escenario.InspeccionActiva();
	if (sonidoAmbiente && ambienteInspeccionPausado != inspeccion) {
		sonidoAmbiente->setIsPaused(inspeccion);
		ambienteInspeccionPausado = inspeccion;
	}
	float dtEscena = inspeccion ? 0.0f : deltaTime;
	animTime += dtEscena;
	zonaTiempo += dtEscena;

	// Nado del buzo (teclado) y animaciones de la zona (burbujas)
	if (!inspeccion) processInput(window);
	if (escenario.Actualizar(dtEscena, animTime, camera))
		Efecto("sound/burbujas.wav", 0.35f);          // exhalación del buzo
	if (estadoActual == ARRECIFES) {
		arrecife.Actualizar(dtEscena, window, escenario, escenario.Seleccionado());
		if (arrecife.sonarAlerta) { Efecto("sound/alerta.wav", 0.6f); arrecife.sonarAlerta = false; }
		if (arrecife.sonarDescubrir) { Efecto("sound/descubrir.wav", 0.5f); arrecife.sonarDescubrir = false; }
	}
	if (estadoActual == AGUAS_FRIAS) {
		aguasFrias.Actualizar(deltaTime, window, escenario, escenario.Seleccionado());
		if (aguasFrias.sonarAlerta) { Efecto("sound/alerta.wav", 0.5f); aguasFrias.sonarAlerta = false; }
		if (aguasFrias.sonarDescubrir) { Efecto("sound/descubrir.wav", 0.5f); aguasFrias.sonarDescubrir = false; }
		if (aguasFrias.sonarRecoger) { Efecto("sound/recoger.wav", 0.55f); aguasFrias.sonarRecoger = false; }
	}
	if (estadoActual == NAUFRAGIOS) {
		naufragio.Actualizar(deltaTime, animTime, window, escenario, camera, escenario.Seleccionado());
		if (naufragio.sonarRecoger) { Efecto("sound/recoger.wav", 0.55f); naufragio.sonarRecoger = false; }
		if (naufragio.sonarAlerta) { Efecto("sound/alerta.wav", 0.5f); naufragio.sonarAlerta = false; }
		if (naufragio.sonarDescubrir) { Efecto("sound/descubrir.wav", 0.5f); naufragio.sonarDescubrir = false; }
		if (naufragio.sonarMonedas) { Efecto("sound/monedas.wav", 0.8f); naufragio.sonarMonedas = false; }
	}

	// Vaivén de la vista: respiración siempre presente + aleteo al nadar
	float nado = glm::length(diverVelocity);
	camera.BobOffset = glm::vec3(0.0f, sinf(animTime * 1.3f) * 0.04f + sinf(animTime * 4.0f) * 0.02f * nado, 0.0f);

	// Cámara activa: FP del buzo u órbita que lo persigue por detrás
	const Camera* camVista = &camera;
	if (modoOrbita && !inspeccion) {
		glm::vec3 objetivo = camera.Position + camera.Front * 2.0f;
		glm::vec3 pos = objetivo - camera.Front * orbitDist + glm::vec3(0.0f, 1.2f, 0.0f);
		float suelo = escenario.AlturaSuelo(pos.x, pos.z) + 0.5f;
		if (pos.y < suelo) pos.y = suelo;
		cameraOrbita.Position = pos;
		cameraOrbita.Front = glm::normalize(objetivo - pos);
		cameraOrbita.Right = glm::normalize(glm::cross(cameraOrbita.Front, cameraOrbita.WorldUp));
		cameraOrbita.Up = glm::normalize(glm::cross(cameraOrbita.Right, cameraOrbita.Front));
		cameraOrbita.Zoom = 50.0f;
		camVista = &cameraOrbita;
	}

	int fbW, fbH;
	glfwGetFramebufferSize(window, &fbW, &fbH);
	if (fbW == 0 || fbH == 0) { glfwPollEvents(); return true; }   // ventana minimizada
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, fbW, fbH);
	glm::mat4 projection;
	glm::mat4 view;

	if (modoOrtogonal) {
		// El tamaño cubre el radio de 42.0f del nado del buzo
		float orthoSize = 20.0f;
		float aspect = (float)fbW / (float)fbH;

		// Proyección ortogonal (sin punto de fuga)
		projection = glm::ortho(-orthoSize * aspect, orthoSize * aspect, -orthoSize, orthoSize, 0.1f, 200.0f);

		// Matriz de vista: Posición alta (Y=80), mirando al centro (0,0,0), con el "Arriba" hacia -Z
		view = glm::lookAt(
			glm::vec3(0.0f, 3.0f, 10.0f),  // DESDE: 2 unidades arriba, 10 hacia atrás
			glm::vec3(0.0f, 0.0f, 0.0f),    // HACIA: El centro del mapa (0,0,0)
			glm::vec3(0.0f, 1.0f, 0.0f)     // ARRIBA: El eje Y estándar
		);
	}
	else {
		// Proyección de perspectiva normal y cámara del buzo
		projection = glm::perspective(glm::radians(camVista->Zoom), (float)fbW / (float)fbH, 0.1f, 400.0f);
		view = camVista->GetViewMatrix();
	}

	// 1) Picking: qué hay en el centro de la pantalla (la mira del buzo)
	escenario.Picking(projection, view, animTime, fbW, fbH);

	// 2) Escena de la zona: agua, terreno, fauna, superficie y burbujas
	//    camVista gobierna la vista; la linterna sigue atada al buzo (camera FP)
	escenario.Dibujar(projection, view, *camVista, animTime, fbW, fbH, &camera);
	if (inspeccion) escenario.DibujarInspeccion(projection, view, camera, animTime);

	// 3) HUD: nombre de la zona, ayuda, mira y fundido de entrada
	const Elemento* sel = escenario.Seleccionado();
	glm::vec3 colorDetalle(1.0f);
	std::string detalle = !sel ? std::string()
		: estadoActual == ARRECIFES ? arrecife.Detalle(*sel, colorDetalle)
		: estadoActual == AGUAS_FRIAS ? aguasFrias.Detalle(*sel, colorDetalle)
		: estadoActual == NAUFRAGIOS ? naufragio.Detalle(*sel, colorDetalle) : std::string();
	if (!inspeccion) menuInicio.DrawMira(window, sel ? sel->etiqueta : std::string(), sel && sel->basura, detalle, colorDetalle);
	if (estadoActual == ARRECIFES) arrecife.DibujarHUD(menuInicio, window, animTime, zonaTiempo);
	if (estadoActual == AGUAS_FRIAS) aguasFrias.DibujarHUD(menuInicio, window, animTime, zonaTiempo);
	if (estadoActual == NAUFRAGIOS) naufragio.DibujarHUD(menuInicio, window, animTime, zonaTiempo, projection, view);
	menuInicio.DrawHUD(window, (int)estadoActual, zonaFade, zonaTiempo, estadoActual == AGUAS_FRIAS);
	if (inspeccion) {
		const Elemento* observado = escenario.ElementoInspeccionado();
		if (observado) menuInicio.DrawInspeccion(window, observado->etiqueta, observado->archivo, detalle, colorDetalle);
	}
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
	// En modo edición (E) WASD mueven el objeto seleccionado, no al buzo
	if (escenario.modoEdicion) {
		Elemento* ed = escenario.Editable();
		if (ed) {
			float velObj = 4.0f * deltaTime;
			float velRot = 60.0f * deltaTime;
			// Movimiento relativo al plano horizontal de la cámara
			glm::vec3 fwd = glm::normalize(glm::vec3(camera.Front.x, 0.0f, camera.Front.z));
			glm::vec3 rgt = glm::normalize(glm::vec3(camera.Right.x, 0.0f, camera.Right.z));
			if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) ed->pos += fwd * velObj;
			if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) ed->pos -= fwd * velObj;
			if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) ed->pos -= rgt * velObj;
			if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) ed->pos += rgt * velObj;
			if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) ed->pos.y += velObj;
			if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
			    glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) ed->pos.y -= velObj;
			// Escala con - / =  (o +)
			if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) {
				ed->escala -= 0.5f * deltaTime;
				if (ed->escala < 0.05f) ed->escala = 0.05f;
			}
			if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) ed->escala += 0.5f * deltaTime;
			// Rotación en Y con Q / E... E es el toggle; usar Q y R
			if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) ed->yaw += velRot;
			if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) ed->yaw -= velRot;
			// Rotación extra en X/Z con 1..4 / 7..0 (opcionales)
			if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) ed->rotExtra.x += velRot;
			if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) ed->rotExtra.x -= velRot;
			if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) ed->rotExtra.z += velRot;
			if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) ed->rotExtra.z -= velRot;
			// Objeto sobre el suelo: la altura se mide desde el terreno en la nueva (x,z)
			if (ed->sobreSuelo) ed->baseY = escenario.AlturaSuelo(ed->pos.x, ed->pos.z);

		}
	}
	else {
		glm::vec3 wish(0.0f);
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) wish += camera.Front;
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) wish -= camera.Front;
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) wish -= camera.Right;
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) wish += camera.Right;
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) wish.y += 1.0f;          // subir
		if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
		    glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) wish.y -= 1.0f;                // bajar
		if (glm::length(wish) > 0.0f) wish = glm::normalize(wish);
		float aleteo = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ? 2.0f : 1.0f;

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
	}

	if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);

	// Activar sonido ambiental con F1
	// F1 (silenciar) se maneja en key_callback
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
	if (estadoActual == ARRECIFES && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && !escenario.modoEdicion) {
		if (escenario.InspeccionActiva()) escenario.CerrarInspeccion();
		else escenario.AbrirInspeccion();
		firstMouse = true;
		return;
	}

	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		if (estadoActual == NAUFRAGIOS && !escenario.modoEdicion)
			naufragio.Clic(escenario.seleccion, escenario, animTime);   // recoger, retirar invasor, cofre
		else {
			if (estadoActual == AGUAS_FRIAS && !escenario.modoEdicion)
				aguasFrias.Clic(escenario);   // recoger krill/pescado y alimentar
			escenario.Interactuar();   // ver el nombre del animal
		}
		// En modo edición, el clic también fija el objeto a editar
		if (escenario.modoEdicion)
			escenario.FijarEditableDesdeSeleccion();
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
	if (estadoActual == ARRECIFES && escenario.InspeccionActiva()) {
		if (firstMouse) {
			lastX = (float)xpos; lastY = (float)ypos; firstMouse = false;
			return;
		}
		float dx = (float)xpos - lastX, dy = lastY - (float)ypos;
		lastX = (float)xpos; lastY = (float)ypos;
		escenario.RotarInspeccion(dx, dy);
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
	if (estadoActual == ARRECIFES && escenario.InspeccionActiva()) {
		escenario.ZoomInspeccion((float)yoffset);
		return;
	}
	if (modoOrbita) {
		orbitDist -= (float)yoffset * 0.5f;
		if (orbitDist < 3.0f) orbitDist = 3.0f;
		if (orbitDist > 20.0f) orbitDist = 20.0f;
	}
	else camera.ProcessMouseScroll((float)yoffset);
}

// ---------------------------------------------------------------------------
// Teclado por eventos: una pulsación = una acción (no se repite al mantener)
// ---------------------------------------------------------------------------
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action != GLFW_PRESS) return;

	if (key == GLFW_KEY_F1) {   // silenciar / activar sonido
		silencio = !silencio;
		if (sonidoAmbiente) sonidoAmbiente->setVolume(silencio ? 0.0f : (estadoActual == MENU ? 0.25f : 0.6f));
		std::cout << (silencio ? ">> Sonido silenciado" : ">> Sonido activado") << std::endl;
		return;
	}

	if (estadoActual == MENU) {
		if (key == GLFW_KEY_ESCAPE)
			glfwSetWindowShouldClose(window, true);
		else
			menuInicio.OnKey(key);
		return;
	}
	if (estadoActual == ARRECIFES && escenario.InspeccionActiva()) {
		if (key == GLFW_KEY_ESCAPE) escenario.CerrarInspeccion();
		return;
	}

	// Dentro de una zona: Esc regresa al menú de inicio, F enciende/apaga la linterna
	if (key == GLFW_KEY_ESCAPE)
		VolverAlMenu();
	else if (key == GLFW_KEY_F) {
		escenario.linterna = !escenario.linterna;
		std::cout << (escenario.linterna ? ">> Linterna encendida" : ">> Linterna apagada") << std::endl;
	}
	else if (key == GLFW_KEY_V) {
		modoOrbita = !modoOrbita;
		firstMouse = true;
		std::cout << (modoOrbita
			? ">> Camara en orbita (rueda: distancia, V: volver a 1a persona)"
			: ">> Camara en 1a persona") << std::endl;
	}
	// Cambiar entre cámara perspectiva y ortogonal (tecla 5)
	else if (key == GLFW_KEY_5) {
		modoOrtogonal = !modoOrtogonal;
		std::cout << (modoOrtogonal
			? ">> Vista Ortogonal activada (Mapa completo)"
			: ">> Vista Ortogonal desactivada (Volviendo al buzo)") << std::endl;
	}
	// --- Modo edición de objetos (A3) ---
	else if (key == GLFW_KEY_E) {
		escenario.modoEdicion = !escenario.modoEdicion;
		if (escenario.modoEdicion) {
			escenario.FijarEditableDesdeSeleccion();
			std::cout << ">> MODO EDICION ON\n"
				"   WASD/Space/Ctrl: mover | -/=: escala | Q/R: rotar Y | 1-4: rotar X/Z\n"
				"   Tab: siguiente objeto | F5: guardar pose | E: salir" << std::endl;
		}
		else {
			std::cout << ">> MODO EDICION OFF" << std::endl;
		}
	}
	else if (escenario.modoEdicion && key == GLFW_KEY_TAB) {
		escenario.CiclarEditable();
	}
	else if (escenario.modoEdicion && key == GLFW_KEY_F5) {
		escenario.GuardarPoseActual();
	}
	else if (estadoActual == ARRECIFES && !escenario.modoEdicion)
		arrecife.Tecla(key, escenario);     // R restaurar, T calentamiento automático, L lista de especies
	else if (estadoActual == AGUAS_FRIAS && !escenario.modoEdicion)
		aguasFrias.Tecla(key, escenario); // R reiniciar, X foto
	else if (estadoActual == NAUFRAGIOS && !escenario.modoEdicion)
		naufragio.Tecla(key);                // L lista de especies
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
	escenario.CerrarInspeccion();
	ambienteInspeccionPausado = false;

	// Carga la zona (la primera vez lee sus modelos) y coloca al buzo en su punto de inicio
	escenario.Entrar(zona, camera);
	if (zona == ARRECIFES) arrecife.Reiniciar(escenario);
	if (zona == AGUAS_FRIAS) aguasFrias.Reiniciar(escenario);
	if (zona == NAUFRAGIOS) naufragio.Reiniciar(escenario, animTime);
	const char* ambientes[] = { "", "sound/ambiente_arrecife.wav", "sound/ambiente_frio.wav", "sound/ambiente_naufragio.wav" };
	Ambiente(ambientes[zona], silencio ? 0.0f : 0.6f);
	diverVelocity = glm::vec3(0.0f);
	firstMouse = true;
	pickedFish = -1;
	zonaTiempo = 0.0f;
	modoOrbita = false;
	orbitDist = 6.0f;
	escenario.modoEdicion = false;
	escenario.editIdx = -1;
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
	Ambiente("sound/ambiente_arrecife.wav", silencio ? 0.0f : 0.25f);   // ambiente suave en el menú
	modoOrbita = false;
	orbitDist = 6.0f;
	escenario.modoEdicion = false;
	escenario.editIdx = -1;
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glfwSetWindowTitle(window, "Acuario Virtual Interactivo");
	std::cout << ">> Menu de inicio" << std::endl;
}
