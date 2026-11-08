// ============================================================================================
// ESCENA · Clase Escenario: administra la zona activa (objetos, terreno, render, picking).
// La clase esta repartida en carpetas por tema (ver los #include al final):
//   escena/estructuras.h ............ datos de cada objeto y de la zona
//   escena/colocar_modelos.h ........ Fijo(), Nadador(), Esparcir(): crear objetos
//   escena/render.h ................. Dibujar(): orden de dibujo (fondo, terreno, objetos, superficie)
//   iluminacion/luces_y_materiales.h  luces, niebla, linterna y materiales
//   animacion/movimientos.h ......... animacion de cada objeto (orbitas, cardumenes, KF)
//   animacion/particulas_zona.h ..... burbujas y monedas
//   interaccion/picking.h ........... seleccion con la mira
//   interaccion/modo_edicion.h ...... mover/rotar/escalar objetos con E
//   zonas/<zona>/<zona>_escena.h .... QUE hay en cada zona y donde (Construir...)
//   zonas/<zona>/<zona>_mecanica.h .. reglas del juego de cada zona
// ============================================================================================
#ifndef ZONAS_H
#define ZONAS_H
/*
* Zonas de inmersion del Acuario Virtual Interactivo.
*
*   Zona 1 - Arrecifes    : agua calida y clara, corales de colores, causticas fuertes
*   Zona 2 - Aguas Frias  : agua palida, plataformas de hielo, ballena, focas, pinguinos
*   Zona 3 - Naufragios   : agua oscura y turbia, submarino, ruinas, basura, linterna
*
* Cada zona es una lista de "Elementos" (modelo + como se mueve). Para agregar un
* animal u objeto basta con agregar un Elemento en ConstruirArrecifes(),
* ConstruirAguasFrias() o ConstruirNaufragios().
*
* Los modelos se exportan desde Blender con herramientas/exporta_zonas.py:
* todos miran a +X, el lomo hacia +Y, y miden su tamano real en metros.
*
* Shaders: shaders/zona.vs/.fs (objetos), shaders/zona_fondo.fs (agua de fondo),
*          shaders/zona_superficie.vs/.fs (superficie del agua vista desde abajo).
*/
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <vector>
#include <map>
#include <deque>
#include <cmath>
#include <iostream>
#include <fstream>

#include <base_opengl/shader_m.h>
#include <base_opengl/model.h>
#include <camara/camera.h>
#include <animacion/keyframes.h>

#include <escena/estructuras.h>      // TipoMov, Elemento, ZonaConfig, Burbuja (los datos de cada objeto)

class Escenario
{
public:
    ZonaConfig cfg;
    ZonaConfig cfgBase;                 // configuracion original de la zona (las mecanicas la modifican)
    bool linterna = false;
    int zona = 0;
    int seleccion = -1;                 // indice del elemento bajo la mira
    int basuraTotal = 0, basuraRecogida = 0;

    bool Init(unsigned int cubemapID)
    {
        cubo = cubemapID;
        sh = new Shader("shaders/zona.vs", "shaders/zona.fs");
        shFondo = new Shader("shaders/menu_fondo.vs", "shaders/zona_fondo.fs");
        shSuperficie = new Shader("shaders/zona_superficie.vs", "shaders/zona_superficie.fs");
        shPicking = new Shader("shaders/15_picking.vs", "shaders/15_picking.fs");
        glGenVertexArrays(1, &vaoVacio);
        // cuadrado de 1x1 en el plano XZ para la superficie del agua
        float q[] = { -0.5f,0,-0.5f,  0.5f,0,-0.5f,  0.5f,0,0.5f,  -0.5f,0,-0.5f,  0.5f,0,0.5f,  -0.5f,0,0.5f };
        glGenVertexArrays(1, &vaoQuad); glGenBuffers(1, &vboQuad);
        glBindVertexArray(vaoQuad); glBindBuffer(GL_ARRAY_BUFFER, vboQuad);
        glBufferData(GL_ARRAY_BUFFER, sizeof(q), q, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glBindVertexArray(0);
        return true;
    }

    // Prepara la zona (carga sus modelos la primera vez) y coloca la camara.
    void Entrar(int z, Camera& cam)
    {
        zona = z;
        elementos.clear(); burbujas.clear(); animaciones.clear(); monedasVuelo.clear(); seleccion = -1;
        // terreno primero: los elementos se colocan sobre el al construirse
        const char* terrenos[] = { "", "terreno_arrecifes", "terreno_aguas_frias", "terreno_naufragios" };
        const char* carpetas[] = { "", "arrecifes", "aguas_frias", "naufragios" };
        terreno = Cargar(terrenos[z], carpetas[z]);
        CalcularAlturas(terreno);
        if (z == 1) ConstruirArrecifes();
        else if (z == 2) ConstruirAguasFrias();
        else ConstruirNaufragios();

        // terreno primero: los demas elementos se apoyan en el
        terreno = Cargar(terrenoArchivo, terrenoCarpeta);
        CalcularAlturas(terreno);
        basuraTotal = 0; basuraRecogida = 0;
        for (size_t i = 0; i < elementos.size(); i++) {
            Elemento& e = elementos[i];
            e.modelo = Cargar(e.archivo, e.carpeta);
            e.medioLargo = MedioLargo(e.modelo);
            e.radioBase = RadioModelo(e.modelo);
            e.id = 1000 + (unsigned int)i;
            e.baseY = e.sobreSuelo ? AlturaSuelo(e.pos.x, e.pos.z) : 0.0f;
            if (e.basura) basuraTotal++;
        }
        for (auto& e : elementos) {
            float mn = 0.0f;
            for (auto& me : e.modelo->meshes) for (auto& v : me.vertices) mn = v.Position.y < mn ? v.Position.y : mn;
            e.pieY = -mn * e.escala;
        }
        for (auto& e : elementos) {
            if (!e.sobreHielo) continue;
            float y;
            if (SueloHielo(e.pos.x, e.pos.z, 200.0f, 0.0f, y)) e.pos.y = y + e.pieY;
            else e.pos.y += e.pieY;
        }
        if (z == 1) AjustarArrecife();
        burbujaModelo = Cargar("burbuja", "comun");
        if (!buzoModelo) buzoModelo = new Model("models/buzo.fbx");
        cfgBase = cfg;
        for (auto& f : fuentes) f.y += AlturaSuelo(f.x, f.z);   // las fuentes se miden desde el fondo
        linterna = cfg.linterna;

        cam.Position = cfg.inicioCamara;
        cam.Yaw = cfg.inicioYaw; cam.Pitch = -5.0f; cam.Zoom = ZOOM;
        cam.ProcessMouseMovement(0.0f, 0.0f);
    }

    // Altura del fondo marino en (x, z), interpolada del terreno cargado.
    float AlturaSuelo(float x, float z) const
    {
        if (alturas.empty()) return 0.0f;
        float fx = (x + 55.0f) / 110.0f * (N - 1), fz = (z + 55.0f) / 110.0f * (N - 1);
        fx = fx < 0 ? 0 : (fx > N - 1.001f ? N - 1.001f : fx);
        fz = fz < 0 ? 0 : (fz > N - 1.001f ? N - 1.001f : fz);
        int i = (int)fx, j = (int)fz; float u = fx - i, v = fz - j;
        float a = alturas[j * N + i], b = alturas[j * N + i + 1], c = alturas[(j + 1) * N + i], d = alturas[(j + 1) * N + i + 1];
        return (a * (1 - u) + b * u) * (1 - v) + (c * (1 - u) + d * u) * v;
    }

    // Burbujas y animaciones. 'exhala' agrega las burbujas del regulador del buzo.
    std::vector<Elemento>& Elementos() { return elementos; }

    // Lanza 'n' monedas desde 'origen' como una fuente que cae girando al fondo (cofre del tesoro)
    void LanzarMonedas(const glm::vec3& origen, int n);   // -> animacion/particulas_zona.h

    // Posicion actual (centro del primer miembro) de un elemento
    glm::vec3 Posicion(int i, float t) const { return glm::vec3(Matriz(elementos[i], i, 0, t)[3]); }
    bool HieloBajo(float x, float z, float maxY, float t, float& y) const { return SueloHielo(x, z, maxY, t, y); }

    // Guarda una animacion por cuadros clave propia de la zona (direccion estable)
    const AnimacionKF* GuardarKF(const AnimacionKF& a) { animaciones.push_back(a); return &animaciones.back(); }

    // Devuelve true en el cuadro en que el buzo exhala (para el sonido de burbujas)
    bool Actualizar(float dt, float t, const Camera& cam);   // -> animacion/particulas_zona.h

    // Pase de picking: dibuja los elementos interactivos con su color de ID y lee el centro.
    // Pase de picking optimizado:
    //  - solo se dibujan los objetos cuya esfera envolvente cruza el rayo de la mira
    //  - solo se rasteriza un cuadrito alrededor del centro (glScissor)
    //  - asistencia de punteria: si el centro no toca nada, se toma el objeto mas cercano al centro
    void Picking(const glm::mat4& proj, const glm::mat4& view, float t, int fbW, int fbH);   // -> interaccion/picking.h

    // Clic sobre lo seleccionado: la basura se recoge, la fauna muestra su nombre.
    void Interactuar();   // -> interaccion/picking.h

    const Elemento* Seleccionado() const { return seleccion >= 0 ? &elementos[seleccion] : nullptr; }

    bool InspeccionActiva() const { return inspeccionIdx >= 0 && inspeccionIdx < (int)elementos.size(); }
    const Elemento* ElementoInspeccionado() const { return InspeccionActiva() ? &elementos[inspeccionIdx] : nullptr; }
    void AbrirInspeccion()
    {
        if (zona != 1 || seleccion < 0 || seleccion >= (int)elementos.size() || !elementos[seleccion].visible) return;
        inspeccionIdx = seleccion;
        giroInspeccion = inclinacionInspeccion = 0.0f;
        zoomInspeccion = 1.0f;
    }
    void CerrarInspeccion() { inspeccionIdx = -1; }
    void RotarInspeccion(float dx, float dy)
    {
        giroInspeccion += dx * 0.45f;
        inclinacionInspeccion = glm::clamp(inclinacionInspeccion + dy * 0.45f, -75.0f, 75.0f);
    }
    void ZoomInspeccion(float delta) { zoomInspeccion = glm::clamp(zoomInspeccion + delta * 0.12f, 0.65f, 1.7f); }

    // ------------------------------------------------------------ modo edición (A3)
    bool modoEdicion = false;
    int  editIdx = -1;                     // índice del objeto en edición

    Elemento* Editable();   // -> interaccion/modo_edicion.h

    // Cicla al siguiente objeto editable (Tab)
    void CiclarEditable();   // -> interaccion/modo_edicion.h

    // Al entrar en edición: usa la selección actual si es editable
    void FijarEditableDesdeSeleccion();   // -> interaccion/modo_edicion.h

    // Guarda la pose del objeto editable: imprime el snippet C++ y escribe poses_editadas.json
    void GuardarPoseActual();   // -> interaccion/modo_edicion.h

    void Dibujar(const glm::mat4& proj, const glm::mat4& view, const Camera& cam, float t, int fbW, int fbH, const Camera* camBuzo = nullptr, bool mostrarBuzo = false);   // -> escena/render.h

    // Presenta el modelo seleccionado delante de la camara sin alterar su pose en la escena.
    void DibujarInspeccion(const glm::mat4& proj, const glm::mat4& view, const Camera& cam, float t);   // -> escena/render.h

private:
    Shader* sh = nullptr, * shFondo = nullptr, * shSuperficie = nullptr, * shPicking = nullptr;
    GLuint vaoVacio = 0, vaoQuad = 0, vboQuad = 0, cubo = 0;
    std::map<std::string, Model*> cache;
    std::vector<Elemento> elementos;
    std::deque<AnimacionKF> animaciones;   // KF creadas por la zona (deque: punteros estables)
    std::vector<Burbuja> burbujas;
    std::vector<MonedaVoladora> monedasVuelo;
    Model* modeloMoneda = nullptr;
    std::vector<glm::vec3> fuentes;
    std::string terrenoArchivo, terrenoCarpeta;
    Model* terreno = nullptr;
    Model* burbujaModelo = nullptr;
    Model* buzoModelo = nullptr;
    std::vector<float> alturas;
    int inspeccionIdx = -1;
    float giroInspeccion = 0.0f, inclinacionInspeccion = 0.0f, zoomInspeccion = 1.0f;
    static const int N = 111;
    float respiracion = 3.0f, fuenteTiempo = 0.0f;
public:
    // [CAMBIAR] velocidad de TODOS los animales (1.0 = normal, 0.5 = mitad, 2.0 = doble)
    float velFauna = 0.5f;             // multiplica la velocidad de nado de toda la fauna (mas lento = mas facil de observar)
    glm::vec3 buzoVel = glm::vec3(0.0f);   // velocidad del buzo (anima el modelo y el vaiven)
    bool buzoDePie = false;            // el buzo camina sobre el hielo (cuerpo vertical)
    float tiempoJuego = 0.0f;          // ultimo tiempo recibido en Actualizar (lo usan las mecanicas y la fisica)
private:
    glm::vec3 buzoFrente = glm::vec3(0.0f, 0.0f, -1.0f);   // orientacion suavizada del cuerpo del buzo
    float buzoFase = 0.0f, buzoUltimoT = 0.0f, buzoVelSuave = 0.0f;

    // ------------------------------------------------------------ utilidades
    static float Aleatorio(int a, int b)
    {
        float s = sinf((float)a * 12.9898f + (float)b * 78.233f) * 43758.5453f;
        return s - floorf(s);
    }
    static glm::vec3 Aleatorio3(int k) { return glm::vec3(Aleatorio(k, 1), Aleatorio(k, 2), Aleatorio(k, 4)) * 2.0f - 1.0f; }

    Model* Cargar(const std::string& archivo, const std::string& carpeta)
    {
        std::string ruta = "models/zonas/" + carpeta + "/" + archivo + ".fbx";
        if (!std::ifstream(ruta).good()) ruta = "models/zonas/" + carpeta + "/" + archivo + ".glb";
        auto it = cache.find(ruta);
        if (it != cache.end()) return it->second;
        Model* m = new Model(ruta);
        if (m->meshes.empty()) std::cout << "[Zonas] No se pudo cargar " << ruta << std::endl;
        cache[ruta] = m;
        return m;
    }

    static float RadioModelo(Model* m)
    {
        float r = 0.05f;
        for (auto& me : m->meshes) for (auto& v : me.vertices) { float l = glm::length(v.Position); r = l > r ? l : r; }
        return r;
    }
    static float EscalaMax(const glm::mat4& M)
    {
        float a = glm::length(glm::vec3(M[0])), b = glm::length(glm::vec3(M[1])), c = glm::length(glm::vec3(M[2]));
        return a > b ? (a > c ? a : c) : (b > c ? b : c);
    }
    // Descarte: no se dibuja lo que esta fuera del campo de vision o tan lejos que la niebla lo oculta
    glm::vec3 cullPos = glm::vec3(0.0f), cullFront = glm::vec3(0.0f, 0.0f, -1.0f);
    float cullAngulo = 1.0f, cullDist = 300.0f;
    bool EnVista(const glm::vec3& c, float r) const
    {
        glm::vec3 v = c - cullPos;
        float dist = glm::length(v);
        if (dist <= r) return true;
        if (dist - r > cullDist) return false;
        float ang = acosf(glm::clamp(glm::dot(v / dist, cullFront), -1.0f, 1.0f));
        return ang < cullAngulo + asinf(r / dist) + 0.05f;
    }

    static float MedioLargo(Model* m)
    {
        float mx = 0.01f;
        for (auto& me : m->meshes) for (auto& v : me.vertices) mx = fabsf(v.Position.x) > mx ? fabsf(v.Position.x) : mx;
        return mx;
    }

    void CalcularAlturas(Model* t)
    {
        alturas.assign(N * N, 0.0f);
        for (auto& me : t->meshes) for (auto& v : me.vertices) {
            int i = (int)roundf((v.Position.x + 55.0f) / 110.0f * (N - 1));
            int j = (int)roundf((v.Position.z + 55.0f) / 110.0f * (N - 1));
            if (i >= 0 && i < N && j >= 0 && j < N) alturas[j * N + i] = v.Position.y;
        }
    }

    static float Suavizar(float x) { x = x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); return x * x * (3.0f - 2.0f * x); }

    // Posicion de un miembro con movimiento propio. nado: 0 en tierra, 1 en el agua. dir (si no es cero) fuerza el rumbo.
    glm::vec3 PosMovil(const Elemento& e, int idx, int k, float t, float& nado, glm::vec3& dir) const;   // -> animacion/movimientos.h

    // Matriz de modelo de un miembro k del elemento en el tiempo t
    glm::mat4 Matriz(const Elemento& e, int idx, int k, float t) const;   // -> animacion/movimientos.h

    // Altura del hielo caminable bajo (x, z): el punto mas alto que no pase de maxY. Falso si no hay hielo.
    bool SueloHielo(float x, float z, float maxY, float t, float& yOut) const
    {
        bool hay = false; float mejor = -1e9f;
        for (size_t i = 0; i < elementos.size(); i++) {
            const Elemento& e = elementos[i];
            if (!e.caminable || !e.visible || !e.modelo) continue;
            glm::mat4 M = Matriz(e, (int)i, 0, t);
            glm::vec3 c = glm::vec3(M[3]);
            float rr = e.radioBase * e.escala * 1.2f + 1.0f;
            if ((x - c.x) * (x - c.x) + (z - c.z) * (z - c.z) > rr * rr) continue;
            glm::mat4 inv = glm::inverse(M);
            glm::vec3 o = glm::vec3(inv * glm::vec4(x, maxY + 50.0f, z, 1.0f));
            glm::vec3 dd = glm::normalize(glm::vec3(inv * glm::vec4(0.0f, -1.0f, 0.0f, 0.0f)));
            for (auto& me : e.modelo->meshes) {
                for (size_t q = 0; q + 2 < me.indices.size(); q += 3) {
                    const glm::vec3& a = me.vertices[me.indices[q]].Position;
                    const glm::vec3& b = me.vertices[me.indices[q + 1]].Position;
                    const glm::vec3& cc = me.vertices[me.indices[q + 2]].Position;
                    glm::vec3 e1 = b - a, e2 = cc - a, pv = glm::cross(dd, e2);
                    float det = glm::dot(e1, pv);
                    if (fabsf(det) < 1e-9f) continue;
                    float inv_d = 1.0f / det;
                    glm::vec3 tv = o - a;
                    float u = glm::dot(tv, pv) * inv_d;
                    if (u < 0.0f || u > 1.0f) continue;
                    glm::vec3 qv = glm::cross(tv, e1);
                    float v = glm::dot(dd, qv) * inv_d;
                    if (v < 0.0f || u + v > 1.0f) continue;
                    float th = glm::dot(e2, qv) * inv_d;
                    if (th < 0.0f) continue;
                    glm::vec3 hit = glm::vec3(M * glm::vec4(o + dd * th, 1.0f));
                    if (hit.y <= maxY && hit.y > mejor) { mejor = hit.y; hay = true; }
                }
            }
        }
        if (hay) yOut = mejor;
        return hay;
    }

    void ConfigurarShader(const glm::mat4& proj, const glm::mat4& view, const Camera& cam, float t, const Camera* camBuzo = nullptr);   // -> iluminacion/luces_y_materiales.h

    void Comunes(const glm::vec3& tinte, float ondula, float largo, float mecer, float fase, float alpha, float emision);   // -> iluminacion/luces_y_materiales.h

    static int Visibles(const Elemento& e)
    {
        if (e.miembros <= 1) return 1;
        int n = (int)ceilf(e.miembros * e.presencia);
        return n < 1 ? 1 : (n > e.miembros ? e.miembros : n);
    }

    void DibujarElemento(const Elemento& e, int idx, float t);   // -> escena/render.h

    // ------------------------------------------------------------ fabricas
    // Presets de material Phong (rúbrica: opaco, plástico, traslúcido, metálico)
    void MatOpaco(Elemento& e);   // -> iluminacion/luces_y_materiales.h
    void MatPlastico(Elemento& e);   // -> iluminacion/luces_y_materiales.h
    void MatTraslucido(Elemento& e);   // -> iluminacion/luces_y_materiales.h
    void MatMetalico(Elemento& e);   // -> iluminacion/luces_y_materiales.h

    Elemento Fijo(const char* archivo, const char* carpeta, glm::vec3 pos, float yaw, float escala);   // -> escena/colocar_modelos.h
    Elemento Nadador(const char* archivo, const char* carpeta, TipoMov mov, glm::vec3 centro, float radio, float vel, const char* etiqueta);   // -> escena/colocar_modelos.h
    // Esparce 'n' copias de un elemento fijo alrededor de un centro
    void Esparcir(Elemento base, int n, glm::vec2 centro, float rMin, float rMax, float escMin, float escMax, int semilla, const std::vector<glm::vec3>& tintes = {});   // -> escena/colocar_modelos.h

    // Ajustes exclusivos del arrecife, basados en las dimensiones de cada malla.
    void AjustarArrecife();   // -> zonas/arrecifes/arrecifes_escena.h

    // ------------------------------------------------------------ Zona 1
    void ConstruirArrecifes();   // -> zonas/arrecifes/arrecifes_escena.h

    // ------------------------------------------------------------ Zona 2
    void ConstruirAguasFrias();   // -> zonas/aguas_frias/aguas_frias_escena.h

    // ------------------------------------------------------------ Zona 3
    void ConstruirNaufragios();   // -> zonas/naufragios/naufragios_escena.h

};


// ============================================================================
// La clase Escenario esta repartida por tema. Aqui se incluyen sus partes:
// ============================================================================
#include <escena/colocar_modelos.h>
#include <escena/render.h>
#include <iluminacion/luces_y_materiales.h>
#include <animacion/movimientos.h>
#include <animacion/particulas_zona.h>
#include <interaccion/picking.h>
#include <interaccion/modo_edicion.h>
#include <zonas/arrecifes/arrecifes_escena.h>
#include <zonas/aguas_frias/aguas_frias_escena.h>
#include <zonas/naufragios/naufragios_escena.h>

#endif
