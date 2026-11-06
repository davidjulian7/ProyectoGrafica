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

#include <shader_m.h>
#include <model.h>
#include <camera.h>
#include <keyframes.h>

enum TipoMov {
    MOV_FIJO,       // quieto (puede flotar suavemente con oscAmp)
    MOV_ORBITA,     // nada en circulo alrededor de pos
    MOV_CARDUMEN,   // grupo de 'miembros' que nadan juntos en circulo
    MOV_LIBRE,      // recorrido en forma de 8 (animales grandes)
    MOV_DERIVA,     // flota a la deriva girando lentamente (basura)
    MOV_KF,         // animacion por cuadros clave (Elemento::kf)
    MOV_ZAMBULLIDA, // vive sobre el hielo: descansa, salta al agua, nada en un lazo y vuelve a subir
    MOV_RUTA,       // recorrido amplio y suave que no se repite (ballena)
    MOV_BARCO       // cruza la zona en linea recta y vuelve a empezar (rompehielos)
};

struct Elemento {
    std::string archivo;               // modelo sin extension (models/zonas/<carpeta>/<archivo>.fbx)
    std::string carpeta;               // arrecifes, aguas_frias, naufragios o comun
    TipoMov mov = MOV_FIJO;
    glm::vec3 pos = glm::vec3(0.0f);   // posicion o centro de la orbita
    float yaw = 0.0f;                  // giro en Y (grados) para elementos fijos
    glm::vec3 rotExtra = glm::vec3(0.0f); // giro adicional en X, Y, Z (grados)
    float escala = 1.0f;
    glm::vec3 tinte = glm::vec3(1.0f); // se multiplica por la textura
    float radio = 0.0f, vel = 0.0f, fase = 0.0f;
    float oscAmp = 0.0f, oscVel = 1.0f; // oscilacion vertical
    float ondula = 0.0f, velNado = 6.0f; // ondulacion del cuerpo al nadar
    float mecer = 0.0f;                // vaiven de algas
    int miembros = 1;                  // para cardumenes
    float dispersion = 1.5f;
    bool sobreSuelo = false;           // pos.y se mide desde el terreno
    bool interactivo = false;
    bool basura = false;
    std::string etiqueta;              // nombre para el HUD (textures/menu/etiquetas/<etiqueta>.png)
    float alpha = 1.0f, emision = 0.0f;
    // material Phong diferenciado (opaco = defaults)
    float specInt = 0.25f, specPower = 40.0f, metalico = 0.0f;
    // animacion por cuadros clave (MOV_KF); apunta a un AnimacionKF con duracion estable
    const AnimacionKF* kf = nullptr;
    bool visible = true;
    // mecanicas
    bool esCoral = false;              // se blanquea con la temperatura (corales, anemonas, almejas)
    float sensibilidad = 1.0f;         // que tan rapido se estresa con el calor
    float estres = 0.0f;               // estres termico acumulado (0 = sano, 1 = blanqueado)
    float blanqueo = 0.0f;             // 0..1 visual
    float muerte = 0.0f;               // 0..1 visual (coral muerto cubierto de algas)
    float presencia = 1.0f;            // fraccion visible de un cardumen
    bool inflable = false;             // pez globo: se infla si el buzo se acerca
    float inflado = 0.0f;
    bool camuflaje = false;            // pulpo: cambia de color
    bool ondulaVertical = false;       // mamiferos: mueven la cola de arriba a abajo
    int residuo = 0;                   // Zona 3: 0 no es basura, 1 plastico, 2 metal, 3 toxico, 4 red
    float peso = 1.0f;                 // cuanto aporta a la limpieza del agua
    bool invasor = false;              // especie invasora que se puede retirar
    bool pulsar = false;               // medusa: la campana se contrae y se expande
    float fantasma = 0.0f;             // brillo de borde tipo Fresnel (traslucidos)
    float kfInicio = 0.0f;             // MOV_KF: segundo en que empieza la animacion
    float rumbo = 0.0f, alcance = 5.0f, periodo = 24.0f, profundidad = 3.0f, nivelAgua = 0.0f;   // MOV_ZAMBULLIDA
    glm::vec3 rotNado = glm::vec3(0.0f);   // giro extra mientras nada (en reposo se usa rotExtra)
    bool caminable = false;            // el buzo puede pararse encima (hielo)
    bool sobreHielo = false;           // al cargar se apoya en el hielo que tiene debajo
    float pieY = 0.0f;                 // distancia del origen a la base del modelo
    float reloj = 0.0f;                // MOV_BARCO: tiempo propio (se puede detener)
    // calculado al cargar
    Model* modelo = nullptr;
    unsigned int id = 0;
    float medioLargo = 0.5f;
    float radioBase = 1.0f;            // radio de la esfera envolvente del modelo (sin escala)
    float baseY = 0.0f;
};

// ---------------------------------------------------------- keyframes (rúbrica)
// Las 5 animaciones basadas en cuadros clave. Coordenadas relativas al 'base'
// de cada una; se construyen una sola vez (static).
inline const AnimacionKF& KF_Ballena()    { static AnimacionKF a = KF_BallenaSalto(glm::vec3(-8.0f, 2.0f, -6.0f)); return a; }
inline const AnimacionKF& KF_Submarino()  { static AnimacionKF a = KF_SubmarinoAsienta(glm::vec3(3.0f, 0.5f, -2.0f)); return a; }
inline const AnimacionKF& KF_Columna()    { static AnimacionKF a = KF_ColumnaCae(glm::vec3(-6.0f, 0.3f, 6.0f)); return a; }
inline const AnimacionKF& KF_Almeja()     { static AnimacionKF a = KF_AlmejaAbre(glm::vec3(3.0f, 0.2f, -2.0f)); return a; }
inline const AnimacionKF& KF_Bolsa()      { static AnimacionKF a = KF_BolsaFlota(glm::vec3(-2.0f, 2.5f, 4.0f)); return a; }

struct ZonaConfig {
    const char* nombre = "";
    glm::vec3 niebla = glm::vec3(0.1f, 0.4f, 0.5f);
    float densidad = 0.025f;
    glm::vec3 solColor = glm::vec3(1.0f);
    glm::vec3 solDir = glm::vec3(-0.3f, -1.0f, -0.2f);
    glm::vec3 ambArriba = glm::vec3(0.3f), ambAbajo = glm::vec3(0.1f);
    float causticas = 0.5f;
    float superficieY = 18.0f;
    bool linterna = false;
    glm::vec3 inicioCamara = glm::vec3(0.0f, 5.0f, 20.0f);
    float inicioYaw = -90.0f;
};

struct Burbuja { glm::vec3 p; float vel; float r; float fase; };
struct MonedaVoladora { glm::vec3 p, v, eje; float ang, vAng, brillo; bool quieta; };   // lluvia de monedas del cofre

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
    void LanzarMonedas(const glm::vec3& origen, int n)
    {
        if (!modeloMoneda) modeloMoneda = Cargar("moneda", "naufragios");
        for (int i = 0; i < n; i++) {
            glm::vec3 r = Aleatorio3(i * 7 + (int)monedasVuelo.size());
            float ang = Aleatorio(i, 31) * 6.2832f, abre = 0.6f + 1.8f * Aleatorio(i, 32);
            MonedaVoladora m;
            m.p = origen + glm::vec3(r.x * 0.25f, Aleatorio(i, 33) * 0.2f, r.z * 0.15f);
            m.v = glm::vec3(cosf(ang) * abre, 3.2f + 2.6f * Aleatorio(i, 34), sinf(ang) * abre + 0.6f);
            m.eje = glm::normalize(glm::vec3(r.x, 0.3f + fabsf(r.y), r.z) + glm::vec3(0.001f));
            m.ang = 0.0f; m.vAng = 6.0f + 10.0f * Aleatorio(i, 35); m.brillo = Aleatorio(i, 36) * 6.28f; m.quieta = false;
            monedasVuelo.push_back(m);
        }
    }

    // Posicion actual (centro del primer miembro) de un elemento
    glm::vec3 Posicion(int i, float t) const { return glm::vec3(Matriz(elementos[i], i, 0, t)[3]); }
    bool HieloBajo(float x, float z, float maxY, float t, float& y) const { return SueloHielo(x, z, maxY, t, y); }

    // Guarda una animacion por cuadros clave propia de la zona (direccion estable)
    const AnimacionKF* GuardarKF(const AnimacionKF& a) { animaciones.push_back(a); return &animaciones.back(); }

    // Devuelve true en el cuadro en que el buzo exhala (para el sonido de burbujas)
    bool Actualizar(float dt, float t, const Camera& cam)
    {
        if (dt > 0.1f) dt = 0.1f;
        tiempoJuego = t;
        bool exhalo = false;
        for (MonedaVoladora& m : monedasVuelo) {
            if (m.quieta) continue;
            m.v.y -= 2.4f * dt;                         // bajo el agua caen despacio
            m.v *= expf(-0.7f * dt);
            m.p += m.v * dt; m.ang += m.vAng * dt;
            float suelo = AlturaSuelo(m.p.x, m.p.z) + 0.02f;
            if (m.p.y <= suelo && m.v.y < 0.0f) {      // al tocar el fondo quedan acostadas
                m.p.y = suelo; m.quieta = true; m.eje = glm::vec3(0.0f, 1.0f, 0.0f);
            }
        }
        for (size_t i = 0; i < elementos.size(); i++) {
            Elemento& e = elementos[i];
            if (!e.inflable) continue;
            glm::vec3 p = glm::vec3(Matriz(e, (int)i, 0, t)[3]);
            float objetivo = glm::length(p - cam.Position) < 3.5f ? 1.0f : 0.0f;
            e.inflado += (objetivo - e.inflado) * (dt * (objetivo > e.inflado ? 4.0f : 0.8f));
        }
        // respiracion del buzo: cada 4.5 s sale una nube de burbujas frente a la mascara
        respiracion += dt;
        if (respiracion > 4.5f) {
            respiracion = 0.0f;
            exhalo = true;
            // salen por debajo de la mascara y suben frente a la vista
            // el regulador expulsa el aire a los costados de la mascara
            for (int k = 0; k < 12; k++) {
                float lado = (k % 2 == 0 ? 1.0f : -1.0f) * (0.45f + 0.25f * Aleatorio(k, 11));
                glm::vec3 p = cam.Position + cam.Front * (0.5f + 0.4f * Aleatorio(k, 13)) + cam.Right * lado - cam.Up * 0.5f
                            + Aleatorio3(k + (int)(t * 10)) * 0.08f;
                burbujas.push_back({ p, 0.9f + Aleatorio(k, 3) * 0.6f, 0.006f + Aleatorio(k, 5) * 0.010f, Aleatorio(k, 9) * 6.0f });
            }
        }
        // fuentes de burbujas de la zona (corales, fugas del submarino)
        fuenteTiempo += dt;
        if (fuenteTiempo > 0.25f) {
            fuenteTiempo = 0.0f;
            for (size_t f = 0; f < fuentes.size(); f++) {
                int k = (int)(t * 13.0f) + (int)f * 31;
                burbujas.push_back({ fuentes[f] + Aleatorio3(k) * 0.3f, 0.5f + Aleatorio(k, 3) * 0.5f, 0.04f + Aleatorio(k, 5) * 0.08f, Aleatorio(k, 7) * 6.0f });
            }
        }
        for (size_t i = 0; i < burbujas.size(); ) {
            Burbuja& b = burbujas[i];
            b.p.y += b.vel * dt;
            b.p.x += sinf(t * 2.3f + b.fase) * 0.15f * dt;
            b.p.z += cosf(t * 1.9f + b.fase) * 0.15f * dt;
            b.r *= 1.0f + 0.03f * dt;       // crecen al subir (menos presion)
            if (b.p.y > cfg.superficieY - 0.05f) { burbujas[i] = burbujas.back(); burbujas.pop_back(); }
            else i++;
        }
        return exhalo;
    }

    // Pase de picking: dibuja los elementos interactivos con su color de ID y lee el centro.
    // Pase de picking optimizado:
    //  - solo se dibujan los objetos cuya esfera envolvente cruza el rayo de la mira
    //  - solo se rasteriza un cuadrito alrededor del centro (glScissor)
    //  - asistencia de punteria: si el centro no toca nada, se toma el objeto mas cercano al centro
    void Picking(const glm::mat4& proj, const glm::mat4& view, float t, int fbW, int fbH)
    {
        if (InspeccionActiva()) return;
        glm::mat4 inv = glm::inverse(view);
        glm::vec3 o = glm::vec3(inv[3]), d = -glm::normalize(glm::vec3(inv[2]));
        int R = (int)(16.0f * fbH / 768.0f); if (R < 5) R = 5;
        float tanMitad = 1.0f / proj[1][1];
        float holgura = (R + 2) * 2.0f * tanMitad / (float)fbH;     // radio angular del cuadrito
        int cx = fbW / 2, cy = fbH / 2;

        glEnable(GL_SCISSOR_TEST);
        glScissor(cx - R, cy - R, 2 * R + 1, 2 * R + 1);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_BLEND);
        shPicking->use();
        shPicking->setMat4("projection", proj);
        shPicking->setMat4("view", view);
        shPicking->setVec4("PickingColor", glm::vec4(1.0f));       // el terreno tapa lo que esta detras
        shPicking->setMat4("model", glm::mat4(1.0f));
        terreno->Draw(*shPicking);
        for (size_t i = 0; i < elementos.size(); i++) {
            Elemento& e = elementos[i];
            if (!e.visible || e.alpha < 0.35f && !e.interactivo) continue;
            unsigned int id = (zona == 1 || e.interactivo) ? e.id : 0xFFFFFF;
            bool colorPuesto = false;
            for (int k = 0; k < Visibles(e); k++) {
                glm::mat4 M = Matriz(e, (int)i, k, t);
                glm::vec3 c = glm::vec3(M[3]);
                float r = e.radioBase * EscalaMax(M) * 1.1f;
                glm::vec3 v = c - o;
                float tca = glm::dot(v, d);
                if (tca < -r) continue;
                float rr = r + (tca > 0.0f ? tca : 0.0f) * holgura;
                if (glm::dot(v, v) - tca * tca > rr * rr) continue;   // la esfera no toca el rayo de la mira
                if (!colorPuesto) {
                    shPicking->setVec4("PickingColor", glm::vec4((id & 0xFF) / 255.0f, ((id >> 8) & 0xFF) / 255.0f, ((id >> 16) & 0xFF) / 255.0f, 1.0f));
                    colorPuesto = true;
                }
                shPicking->setMat4("model", M);
                e.modelo->Draw(*shPicking);
            }
        }
        int lado = 2 * R + 1;
        std::vector<unsigned char> px(lado * lado * 4);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(cx - R, cy - R, lado, lado, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
        glDisable(GL_SCISSOR_TEST);
        int nuevo = -1, mejor = 1 << 30;
        for (int y = 0; y < lado; y++)
            for (int x = 0; x < lado; x++) {
                const unsigned char* q = &px[(y * lado + x) * 4];
                unsigned int id = q[0] + q[1] * 256 + q[2] * 65536;
                if (id < 1000 || id >= 1000 + elementos.size()) continue;
                int d2 = (x - R) * (x - R) + (y - R) * (y - R);
                if (d2 < mejor) { mejor = d2; nuevo = (int)(id - 1000); }
            }
        if (nuevo != seleccion) {
            seleccion = nuevo;
            if (seleccion >= 0) std::cout << "  Apuntando a: " << elementos[seleccion].etiqueta << std::endl;
        }
    }

    // Clic sobre lo seleccionado: la basura se recoge, la fauna muestra su nombre.
    void Interactuar()
    {
        if (seleccion < 0) return;
        Elemento& e = elementos[seleccion];
        if (e.basura) {
            e.visible = false; basuraRecogida++;
            std::cout << ">> Basura recogida: " << e.etiqueta << "  (" << basuraRecogida << "/" << basuraTotal << ")" << std::endl;
            seleccion = -1;
        }
        else std::cout << ">> " << e.etiqueta << std::endl;
    }

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

    Elemento* Editable()
    {
        if (editIdx < 0 || editIdx >= (int)elementos.size()) return nullptr;
        Elemento& e = elementos[editIdx];
        if (!e.visible) return nullptr;
        // Solo fijos / basura / KF: no cardúmenes ni animales de recorrido libre
        if (e.mov == MOV_CARDUMEN || e.mov == MOV_LIBRE || e.mov == MOV_ORBITA) return nullptr;
        return &e;
    }

    // Cicla al siguiente objeto editable (Tab)
    void CiclarEditable()
    {
        int n = (int)elementos.size();
        if (n == 0) return;
        for (int step = 1; step <= n; step++) {
            int i = (editIdx + step) % n;
            Elemento& e = elementos[i];
            if (!e.visible) continue;
            if (e.mov == MOV_CARDUMEN || e.mov == MOV_LIBRE || e.mov == MOV_ORBITA) continue;
            editIdx = i;
            seleccion = i;
            std::cout << ">> Editando: " << (e.etiqueta.empty() ? e.archivo : e.etiqueta)
                      << "  (pos " << e.pos.x << "," << e.pos.y << "," << e.pos.z
                      << "  esc " << e.escala << "  yaw " << e.yaw << ")" << std::endl;
            return;
        }
    }

    // Al entrar en edición: usa la selección actual si es editable
    void FijarEditableDesdeSeleccion()
    {
        if (seleccion < 0 || seleccion >= (int)elementos.size()) { CiclarEditable(); return; }
        Elemento& e = elementos[seleccion];
        if (e.mov == MOV_CARDUMEN || e.mov == MOV_LIBRE || e.mov == MOV_ORBITA) { CiclarEditable(); return; }
        editIdx = seleccion;
        std::cout << ">> Editando: " << (e.etiqueta.empty() ? e.archivo : e.etiqueta)
                  << "  (pos " << e.pos.x << "," << e.pos.y << "," << e.pos.z
                  << "  esc " << e.escala << "  yaw " << e.yaw << ")" << std::endl;
    }

    // Guarda la pose del objeto editable: imprime el snippet C++ y escribe poses_editadas.json
    void GuardarPoseActual()
    {
        Elemento* pe = Editable();
        if (!pe) { std::cout << ">> No hay objeto editable seleccionado" << std::endl; return; }
        Elemento& e = *pe;

        std::cout << "// Pose de " << e.archivo << " (carpeta " << e.carpeta << "):\n"
                  << "  Elemento p = Fijo(\"" << e.archivo << "\", \"" << e.carpeta << "\",\n"
                  << "      glm::vec3(" << e.pos.x << "f, " << e.pos.y << "f, " << e.pos.z << "f),\n"
                  << "      " << e.yaw << "f, " << e.escala << "f);\n"
                  << "  p.rotExtra = glm::vec3(" << e.rotExtra.x << "f, " << e.rotExtra.y << "f, " << e.rotExtra.z << "f);\n"
                  << "  p.tinte = glm::vec3(" << e.tinte.r << "f, " << e.tinte.g << "f, " << e.tinte.b << "f);\n";

        std::ofstream out("poses_editadas.json", std::ios::app);
        if (out) {
            out << "{\n"
                << "  \"archivo\": \"" << e.archivo << "\",\n"
                << "  \"carpeta\": \"" << e.carpeta << "\",\n"
                << "  \"pos\": [" << e.pos.x << ", " << e.pos.y << ", " << e.pos.z << "],\n"
                << "  \"yaw\": " << e.yaw << ",\n"
                << "  \"escala\": " << e.escala << ",\n"
                << "  \"rotExtra\": [" << e.rotExtra.x << ", " << e.rotExtra.y << ", " << e.rotExtra.z << "],\n"
                << "  \"tinte\": [" << e.tinte.r << ", " << e.tinte.g << ", " << e.tinte.b << "]\n"
                << "},\n";
            std::cout << ">> Pose anexada a poses_editadas.json" << std::endl;
        }
    }

    void Dibujar(const glm::mat4& proj, const glm::mat4& view, const Camera& cam, float t, int fbW, int fbH, const Camera* camBuzo = nullptr, bool mostrarBuzo = false)
    {
        ZonaConfig cfgGuardada = cfg;
        bool arriba = zona == 2 && cam.Position.y > cfg.superficieY;   // sobre el hielo: aire, cielo y mar
        if (arriba) {
            cfg.niebla = glm::vec3(0.66f, 0.80f, 0.90f); cfg.densidad = 0.0045f; cfg.causticas = 0.0f;
            cfg.ambArriba = glm::vec3(0.62f, 0.70f, 0.80f); cfg.ambAbajo = glm::vec3(0.34f, 0.38f, 0.44f);
            cfg.solColor = glm::vec3(1.05f, 1.0f, 0.92f);
        }
        glClearColor(cfg.niebla.r, cfg.niebla.g, cfg.niebla.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        float profundidad = cfg.superficieY - cam.Position.y;

        // 1) Agua de fondo (direccion de vista -> color del agua, sol y rayos)
        glDepthMask(GL_FALSE);
        shFondo->use();
        glm::mat4 vistaRot = glm::mat4(glm::mat3(view));
        shFondo->setMat4("uInvVP", glm::inverse(proj * vistaRot));
        shFondo->setVec2("uRes", glm::vec2((float)fbW, (float)fbH));
        shFondo->setVec3("uNiebla", cfg.niebla);
        shFondo->setVec3("uSolDir", glm::normalize(cfg.solDir));
        shFondo->setVec3("uSolColor", cfg.solColor);
        shFondo->setFloat("uTiempo", t);
        shFondo->setFloat("uProfCam", profundidad);
        shFondo->setInt("uArriba", arriba ? 1 : 0);
        glBindVertexArray(vaoVacio); glDrawArrays(GL_TRIANGLES, 0, 3); glBindVertexArray(0);
        glDepthMask(GL_TRUE);

        // parametros del descarte por campo de vision y niebla
        cullPos = cam.Position; cullFront = cam.Front;
        float tanY = tanf(glm::radians(cam.Zoom) * 0.5f), aspecto = (float)fbW / (float)(fbH > 0 ? fbH : 1);
        cullAngulo = atanf(tanY * sqrtf(1.0f + aspecto * aspecto));
        cullDist = 3.3f / (cfg.densidad > 0.001f ? cfg.densidad : 0.001f);

        // 2) Terreno y elementos opacos
        glDisable(GL_BLEND);
        sh->use();
        ConfigurarShader(proj, view, cam, t, camBuzo);
        Comunes(glm::vec3(1.0f), 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
        sh->setMat4("model", glm::mat4(1.0f));
        terreno->Draw(*sh);
        for (size_t i = 0; i < elementos.size(); i++) {
            Elemento& e = elementos[i];
            if (!e.visible || e.alpha < 1.0f) continue;
            if (InspeccionActiva() && (int)i == inspeccionIdx) continue;
            DibujarElemento(e, (int)i, t);
        }

        // El jugador comparte la pose de la camara de primera persona en todas las zonas.
        // Se oculta desde sus ojos para evitar que la mascara tape la vista.
        if (mostrarBuzo && camBuzo && buzoModelo) {
            Comunes(glm::vec3(1.0f), 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
            float dtB = t - buzoUltimoT; buzoUltimoT = t;
            if (dtB < 0.0f || dtB > 0.2f) { dtB = 0.016f; buzoFrente = camBuzo->Front; }
            // el cuerpo sigue a la mirada con retraso, asi no gira de golpe
            glm::vec3 objF = camBuzo->Front;
            if (buzoDePie) { objF.y = 0.0f; objF = glm::length(objF) > 0.01f ? glm::normalize(objF) : buzoFrente; }
            buzoFrente = glm::normalize(glm::mix(buzoFrente, objF, 1.0f - expf(-5.0f * dtB)));
            float vel = glm::length(buzoVel);
            buzoVelSuave += (vel - buzoVelSuave) * (1.0f - expf(-4.0f * dtB));
            buzoFase += dtB * (1.6f + buzoVelSuave * 0.9f);
            float mov = buzoVelSuave > 4.0f ? 1.0f : buzoVelSuave * 0.25f;
            glm::vec3 der = glm::normalize(glm::cross(buzoFrente, glm::vec3(0.0f, 1.0f, 0.0f)));
            glm::vec3 arr = glm::cross(der, buzoFrente);
            glm::mat4 pose(1.0f);
            pose[0] = glm::vec4(der, 0.0f);
            pose[1] = glm::vec4(arr, 0.0f);
            pose[2] = glm::vec4(-buzoFrente, 0.0f);
            pose[3] = glm::vec4(camBuzo->Position + camBuzo->BobOffset + arr * (0.05f * sinf(buzoFase * 2.0f) * (0.4f + mov)), 1.0f);
            if (!buzoDePie) pose = glm::rotate(pose, -0.85f * mov, glm::vec3(1.0f, 0.0f, 0.0f));   // se inclina hacia adelante al nadar
            pose = glm::rotate(pose, 0.10f * sinf(buzoFase) * (0.3f + mov), glm::vec3(0.0f, 0.0f, 1.0f));   // balanceo de aleteo
            pose = glm::rotate(pose, 0.06f * sinf(buzoFase * 2.0f + 1.0f) * mov, glm::vec3(1.0f, 0.0f, 0.0f));
            // Modelo original: Z arriba, cara hacia -Y; la mascara queda en el origen del jugador.
            pose = glm::rotate(pose, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            pose = glm::rotate(pose, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            pose = glm::translate(pose, glm::vec3(0.0f, 0.25f, -0.72f));
            sh->setMat4("model", pose);
            buzoModelo->Draw(*sh);
        }

        // monedas voladoras: oro metalico con destellos
        if (modeloMoneda && !monedasVuelo.empty()) {
            Comunes(glm::vec3(1.15f, 0.92f, 0.45f), 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
            sh->setFloat("uSpecPower", 90.0f); sh->setFloat("uSpecInt", 1.6f); sh->setFloat("uMetalico", 1.0f);
            for (const MonedaVoladora& m : monedasVuelo) {
                if (!EnVista(m.p, 0.2f)) continue;
                sh->setFloat("uEmision", 0.25f + 0.35f * (0.5f + 0.5f * sinf(t * 7.0f + m.brillo)));
                glm::mat4 mm = glm::translate(glm::mat4(1.0f), m.p);
                if (m.quieta) mm = glm::rotate(mm, m.brillo, glm::vec3(0.0f, 1.0f, 0.0f));
                else mm = glm::rotate(mm, m.ang, m.eje);
                sh->setMat4("model", glm::scale(mm, glm::vec3(2.4f)));
                modeloMoneda->Draw(*sh);
            }
        }

        // 3) Superficie del agua vista desde abajo (Fresnel + ventana de Snell)
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_CULL_FACE);
        shSuperficie->use();
        glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(cam.Position.x, cfg.superficieY, cam.Position.z));
        m = glm::scale(m, glm::vec3(400.0f, 1.0f, 400.0f));
        shSuperficie->setMat4("model", m);
        shSuperficie->setMat4("view", view);
        shSuperficie->setMat4("projection", proj);
        shSuperficie->setVec3("uCamPos", cam.Position);
        shSuperficie->setVec3("uNiebla", cfg.niebla);
        shSuperficie->setFloat("uDensidad", cfg.densidad);
        shSuperficie->setVec3("uSolDir", glm::normalize(cfg.solDir));
        shSuperficie->setVec3("uSolColor", cfg.solColor);
        shSuperficie->setFloat("uTiempo", t);
        shSuperficie->setInt("uCubo", 1);
        glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_CUBE_MAP, cubo); glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(vaoQuad); glDrawArrays(GL_TRIANGLES, 0, 6); glBindVertexArray(0);

        // 4) Transparentes: elementos semitransparentes y burbujas
        sh->use();
        glDepthMask(GL_FALSE);
        for (size_t i = 0; i < elementos.size(); i++) {
            Elemento& e = elementos[i];
            if (InspeccionActiva() && (int)i == inspeccionIdx) continue;
            if (e.visible && e.alpha < 1.0f) DibujarElemento(e, (int)i, t);
        }
        Comunes(glm::vec3(0.85f, 0.95f, 1.0f), 0.0f, 0.0f, 0.0f, 0.0f, 0.45f, 0.35f);
        sh->setFloat("uSpecPower", 64.0f);
        sh->setFloat("uSpecInt", 0.45f);
        sh->setFloat("uMetalico", 0.0f);
        for (const Burbuja& b : burbujas) {
            if (!EnVista(b.p, b.r * 2.0f)) continue;
            glm::mat4 mb = glm::translate(glm::mat4(1.0f), b.p);
            mb = glm::scale(mb, glm::vec3(b.r * 2.0f));
            sh->setMat4("model", mb);
            burbujaModelo->Draw(*sh);
        }
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        cfg = cfgGuardada;
    }

    // Presenta el modelo seleccionado delante de la camara sin alterar su pose en la escena.
    void DibujarInspeccion(const glm::mat4& proj, const glm::mat4& view, const Camera& cam, float t)
    {
        if (!InspeccionActiva()) return;
        Elemento& e = elementos[inspeccionIdx];
        glClear(GL_DEPTH_BUFFER_BIT);
        glDisable(GL_BLEND);
        sh->use();
        ConfigurarShader(proj, view, cam, t, &cam);
        Comunes(e.tinte, e.ondula, e.medioLargo, e.mecer, e.fase, e.alpha, e.emision);
        sh->setFloat("uSpecPower", e.specPower);
        sh->setFloat("uSpecInt", e.specInt);
        sh->setFloat("uMetalico", e.metalico);
        sh->setFloat("uBlanqueo", e.blanqueo);
        sh->setFloat("uMuerte", e.muerte);
        sh->setInt("uOndulaEje", e.ondulaVertical ? 1 : 0);
        sh->setFloat("uFantasma", e.fantasma);
        glm::mat4 pose = Matriz(e, inspeccionIdx, 0, t);
        glm::mat4 rotacion(1.0f);
        rotacion = glm::rotate(rotacion, glm::radians(giroInspeccion), cam.Up);
        rotacion = glm::rotate(rotacion, glm::radians(inclinacionInspeccion), cam.Right);
        float diametro = 2.0f * e.radioBase * e.escala;
        float ajuste = 2.2f / (diametro > 0.05f ? diametro : 0.05f);
        glm::vec3 centro = cam.Position + cam.Front * 3.8f + cam.Up * 0.05f;
        glm::mat4 modelo = glm::translate(glm::mat4(1.0f), centro) * rotacion
                         * glm::mat4(glm::mat3(pose)) * glm::scale(glm::mat4(1.0f), glm::vec3(ajuste * zoomInspeccion));
        sh->setMat4("model", modelo);
        e.modelo->Draw(*sh, e.modelo->TieneAnimacion() ? t : -1.0f);
    }

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
    glm::vec3 PosMovil(const Elemento& e, int idx, int k, float t, float& nado, glm::vec3& dir) const
    {
        nado = 1.0f; dir = glm::vec3(0.0f);
        float signo = e.vel >= 0.0f ? 1.0f : -1.0f;
        glm::vec3 off = k > 0 ? Aleatorio3(idx * 97 + k) * e.dispersion : glm::vec3(0.0f);
        glm::vec3 p = e.pos;
        switch (e.mov) {
        case MOV_ORBITA:
        case MOV_CARDUMEN: {
            float a = t * e.vel * velFauna + e.fase - (k > 0 ? (0.15f + 0.25f * Aleatorio(idx, k)) * signo * (float)(k % 7) * 0.3f : 0.0f);
            float r = e.radio + off.x;
            p = e.pos + glm::vec3(cosf(a) * r, off.y + sinf(t * e.oscVel + e.fase + k) * e.oscAmp, sinf(a) * r);
            p.y += e.baseY;
            break; }
        case MOV_LIBRE: {
            float a = t * e.vel * velFauna + e.fase;
            p = e.pos + glm::vec3(e.radio * sinf(a), sinf(t * e.oscVel + e.fase) * e.oscAmp, e.radio * 0.6f * sinf(2.0f * a)) + off;
            break; }
        case MOV_RUTA: {   // curva con frecuencias que no coinciden: casi nunca repite el mismo recorrido
            float a = t * fabsf(e.vel) * velFauna, R = e.radio;
            p = e.pos + glm::vec3(R * (0.85f * sinf(a + e.fase) + 0.35f * sinf(0.37f * a + 1.3f)),
                                  e.oscAmp * (sinf(0.43f * a + 0.5f) + 0.35f * sinf(0.97f * a + 2.0f)),
                                  R * (0.60f * cosf(0.71f * a + 1.7f * e.fase) + 0.30f * sinf(0.53f * a + 0.4f))) + off;
            break; }
        case MOV_BARCO: {
            float x = -110.0f + fmodf(e.reloj * e.vel, 220.0f);
            p = glm::vec3(x, e.pos.y + 0.12f * sinf(t * 0.8f + e.fase), e.pos.z);
            dir = glm::vec3(1.0f, 0.0f, 0.0f);
            break; }
        case MOV_ZAMBULLIDA: {
            const float Tj = e.vel > 0.1f ? e.vel : 1.3f, Tn = 8.0f, R = e.radio, h = e.oscAmp;
            float P = e.periodo < 2.0f * Tj + Tn + 4.0f ? 2.0f * Tj + Tn + 4.0f : e.periodo;
            float Ti = P - 2.0f * Tj - Tn;
            float tt = t + (float)k * P * 0.23f + e.fase * P / 6.2832f;
            float c = floorf(tt / P), s = tt - c * P;
            bool par = ((long)c % 2) == 0;
            float sg = par ? 1.0f : -1.0f;
            glm::vec3 d(cosf(e.rumbo), 0.0f, sinf(e.rumbo)), lat(-d.z, 0.0f, d.x);
            glm::vec3 A = e.pos + lat * ((float)k - 0.5f * (float)(e.miembros - 1)) * e.dispersion - lat * R;
            glm::vec3 base = par ? A : A + lat * (2.0f * R);
            float yAgua = e.nivelAgua - 0.35f;
            glm::vec3 W = base + d * e.alcance; W.y = yAgua;
            if (s < Ti) {
                float w = Suavizar(s / (0.4f * Ti));
                float ang = 3.14159f * w;
                dir = d * (-cosf(ang)) + lat * sg * sinf(ang);
                p = base; nado = 0.0f;
            } else if (s < Ti + Tj) {
                float u = (s - Ti) / Tj;
                p = base + d * (e.alcance * u);
                p.y = e.pos.y + (yAgua - e.pos.y) * u + h * 4.0f * u * (1.0f - u);
                nado = Suavizar(u * 1.6f);
            } else if (s < Ti + Tj + Tn) {
                float u = (s - Ti - Tj) / Tn, th = 3.14159f * u;
                p = W + lat * (sg * R * (1.0f - cosf(th))) + d * (R * sinf(th));
                p.y = yAgua - e.profundidad * sinf(th);
            } else {
                float u = (s - Ti - Tj - Tn) / Tj;
                glm::vec3 W2 = W + lat * (sg * 2.0f * R), B = base + lat * (sg * 2.0f * R);
                p = W2 + (B - W2) * u; p.y = yAgua + (e.pos.y - yAgua) * u + h * 4.0f * u * (1.0f - u);
                nado = 1.0f - Suavizar((u - 0.4f) * 1.8f);
            }
            break; }
        default: break;
        }
        return p;
    }

    // Matriz de modelo de un miembro k del elemento en el tiempo t
    glm::mat4 Matriz(const Elemento& e, int idx, int k, float t) const
    {
        glm::vec3 p = e.pos;
        float yawUsar = e.yaw, escUsar = e.escala, pitch = 0.0f, roll = 0.0f, nado = 0.0f, yawMov = 0.0f;
        glm::vec3 rotUsar = e.rotExtra;
        bool orientar = false;
        glm::vec3 off = k > 0 ? Aleatorio3(idx * 97 + k) * e.dispersion : glm::vec3(0.0f);
        switch (e.mov) {
        case MOV_FIJO:
            p.y += e.baseY + sinf(t * e.oscVel + e.fase) * e.oscAmp;
            if (k > 0) { p += glm::vec3(off.x, 0.0f, off.z); yawUsar += k * 137.5f; }   // copias esparcidas (monedas)
            break;
        case MOV_KF:
            if (e.kf) e.kf->PoseAt(t - e.kfInicio + (e.kf->loop ? e.fase : 0.0f), p, yawUsar, rotUsar, escUsar);
            break;
        case MOV_DERIVA:
            p += glm::vec3(sinf(t * 0.21f + e.fase) * 0.6f, sinf(t * 0.37f + e.fase) * 0.5f, cosf(t * 0.17f + e.fase) * 0.6f);
            p.y += e.baseY;
            break;
        case MOV_ORBITA: case MOV_CARDUMEN: case MOV_LIBRE: case MOV_RUTA: case MOV_BARCO: case MOV_ZAMBULLIDA: {
            glm::vec3 d0, d1, d2, q1, q2;
            float n1, n2;
            p = PosMovil(e, idx, k, t, nado, d0);
            q1 = PosMovil(e, idx, k, t + 0.12f, n1, d1);
            q2 = PosMovil(e, idx, k, t + 0.24f, n2, d2);
            glm::vec3 v1 = q1 - p, v2 = q2 - q1;
            float h1 = sqrtf(v1.x * v1.x + v1.z * v1.z), h2 = sqrtf(v2.x * v2.x + v2.z * v2.z);
            glm::vec3 dir = glm::dot(d0, d0) > 0.0f ? d0 : v1;
            if (glm::dot(dir, dir) < 1e-8f) dir = glm::vec3(1.0f, 0.0f, 0.0f);
            yawMov = atan2f(-dir.z, dir.x);
            if (e.mov == MOV_BARCO) {
                pitch = 0.015f * sinf(t * 0.7f + e.fase); roll = 0.025f * sinf(t * 0.9f);
            } else if (!e.pulsar && glm::dot(d0, d0) == 0.0f) {
                if (h1 > 1e-4f && (glm::dot(d0, d0) == 0.0f)) {
                    pitch = atan2f(v1.y, h1); pitch = pitch > 0.6f ? 0.6f : (pitch < -0.6f ? -0.6f : pitch);
                    if (e.mov != MOV_ZAMBULLIDA && e.mov != MOV_RUTA) pitch *= 0.6f;
                    if (h2 > 1e-4f) {
                        float dy = atan2f(-v2.z, v2.x) - atan2f(-v1.z, v1.x);
                        while (dy > 3.14159f) dy -= 6.28318f;
                        while (dy < -3.14159f) dy += 6.28318f;
                        roll = -dy / 0.12f * 0.45f; roll = roll > 0.4f ? 0.4f : (roll < -0.4f ? -0.4f : roll);
                    }
                }
                if (nado > 0.5f && e.mov != MOV_ZAMBULLIDA) yawMov += 0.07f * sinf(t * e.velNado * 0.6f + e.fase + k);
            }
            orientar = true;
            break; }
        }
        glm::mat4 M = glm::translate(glm::mat4(1.0f), p);
        if (orientar) {
            M = glm::rotate(M, yawMov, glm::vec3(0.0f, 1.0f, 0.0f));
            if (pitch != 0.0f) M = glm::rotate(M, pitch, glm::vec3(0, 0, 1));
            if (roll != 0.0f) M = glm::rotate(M, roll, glm::vec3(1, 0, 0));
            if (e.mov == MOV_ZAMBULLIDA) rotUsar = glm::mix(e.rotExtra, e.rotNado, nado);
        }
        else M = glm::rotate(M, glm::radians(yawUsar), glm::vec3(0.0f, 1.0f, 0.0f));
        if (rotUsar.x != 0.0f) M = glm::rotate(M, glm::radians(rotUsar.x), glm::vec3(1, 0, 0));
        if (rotUsar.y != 0.0f) M = glm::rotate(M, glm::radians(rotUsar.y), glm::vec3(0, 1, 0));
        if (rotUsar.z != 0.0f) M = glm::rotate(M, glm::radians(rotUsar.z), glm::vec3(0, 0, 1));
        if (e.mov == MOV_DERIVA) M = glm::rotate(M, t * 0.2f + e.fase, glm::normalize(glm::vec3(0.3f, 1.0f, 0.5f)));
        float sc = escUsar * (k > 0 ? 0.85f + 0.3f * Aleatorio(idx, k + 50) : 1.0f) * (1.0f + 0.8f * e.inflado);
        glm::vec3 sv(sc);
        if (e.pulsar) {   // la campana se contrae (mas angosta y alta) y se relaja
            float pu = sinf(t * 2.2f + e.fase + k * 1.3f);
            sv = glm::vec3(sc * (1.0f - 0.10f * pu), sc * (1.0f + 0.12f * pu), sc * (1.0f - 0.10f * pu));
        }
        return glm::scale(M, sv);
    }

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

    void ConfigurarShader(const glm::mat4& proj, const glm::mat4& view, const Camera& cam, float t, const Camera* camBuzo = nullptr)
    {
        sh->setMat4("projection", proj);
        sh->setMat4("view", view);
        sh->setVec3("uCamPos", cam.Position);
        sh->setVec3("uSolDir", glm::normalize(cfg.solDir));
        sh->setVec3("uSolColor", cfg.solColor);
        sh->setVec3("uAmbArriba", cfg.ambArriba);
        sh->setVec3("uAmbAbajo", cfg.ambAbajo);
        sh->setVec3("uNiebla", cfg.niebla);
        sh->setFloat("uDensidad", cfg.densidad);
        sh->setFloat("uCausticas", cfg.causticas);
        sh->setFloat("uSuperficieY", cfg.superficieY);
        sh->setFloat("uTiempo", t);
        sh->setInt("uLinterna", linterna ? 1 : 0);
        // La linterna sale siempre del buzo (1a persona), aunque se mire en órbita
        const Camera& luz = camBuzo ? *camBuzo : cam;
        sh->setVec3("uLinternaPos", luz.Position - luz.Up * 0.25f + luz.Right * 0.2f);
        sh->setVec3("uLinternaDir", luz.Front);
    }

    void Comunes(const glm::vec3& tinte, float ondula, float largo, float mecer, float fase, float alpha, float emision)
    {
        sh->setVec3("uTinte", tinte);
        sh->setFloat("uOndula", ondula);
        sh->setFloat("uLargo", largo);
        sh->setFloat("uVelNado", 6.0f);
        sh->setFloat("uMecer", mecer);
        sh->setFloat("uFase", fase);
        sh->setFloat("uAlpha", alpha);
        sh->setFloat("uEmision", emision);
        sh->setFloat("uResaltado", 0.0f);
        sh->setFloat("uBlanqueo", 0.0f);
        sh->setFloat("uMuerte", 0.0f);
        sh->setFloat("uFantasma", 0.0f);
        sh->setInt("uOndulaEje", 0);
        sh->setInt("uSkinning", 0);
        sh->setFloat("uSpecPower", 40.0f);
        sh->setFloat("uSpecInt", 0.25f);
        sh->setFloat("uMetalico", 0.0f);
    }

    static int Visibles(const Elemento& e)
    {
        if (e.miembros <= 1) return 1;
        int n = (int)ceilf(e.miembros * e.presencia);
        return n < 1 ? 1 : (n > e.miembros ? e.miembros : n);
    }

    void DibujarElemento(const Elemento& e, int idx, float t)
    {
        glm::vec3 tinte = e.tinte;
        if (e.camuflaje) {   // el pulpo alterna entre rojo, arena y moteado oscuro
            float c = 0.5f + 0.5f * sinf(t * 0.35f + e.fase);
            tinte = glm::mix(glm::vec3(1.25f, 0.95f, 0.85f), glm::vec3(0.75f, 0.55f, 0.45f), c);
        }
        Comunes(tinte, e.ondula, e.medioLargo, e.mecer, e.fase, e.alpha, e.emision);
        sh->setFloat("uSpecPower", e.specPower);
        sh->setFloat("uSpecInt", e.specInt);
        sh->setFloat("uMetalico", e.metalico);
        sh->setFloat("uVelNado", e.velNado);
        sh->setFloat("uBlanqueo", e.blanqueo);
        sh->setFloat("uMuerte", e.muerte);
        sh->setInt("uOndulaEje", e.ondulaVertical ? 1 : 0);
        sh->setFloat("uFantasma", e.fantasma);
        sh->setFloat("uResaltado", idx == seleccion ? 0.25f + 0.15f * sinf(t * 6.0f) : 0.0f);
        sh->setFloat("uVelNado", e.velNado * (0.4f + 0.6f * velFauna));
        for (int k = 0; k < Visibles(e); k++) {
            glm::mat4 M = Matriz(e, idx, k, t);
            if (!EnVista(glm::vec3(M[3]), e.radioBase * EscalaMax(M) * 1.15f)) continue;
            sh->setFloat("uFase", e.fase + k * 1.3f);
            sh->setMat4("model", M);
            e.modelo->Draw(*sh, e.modelo->TieneAnimacion() ? t + e.fase + k * 0.37f : -1.0f);
        }
    }

    // ------------------------------------------------------------ fabricas
    // Presets de material Phong (rúbrica: opaco, plástico, traslúcido, metálico)
    void MatOpaco(Elemento& e)   { e.specInt = 0.08f; e.specPower = 24.0f; e.metalico = 0.0f; }
    void MatPlastico(Elemento& e){ e.specInt = 0.60f; e.specPower = 32.0f; e.metalico = 0.0f; }
    void MatTraslucido(Elemento& e) { e.specInt = 0.30f; e.specPower = 64.0f; e.metalico = 0.0f; if (e.alpha >= 1.0f) e.alpha = 0.75f; }
    void MatMetalico(Elemento& e){ e.specInt = 1.00f; e.specPower = 96.0f; e.metalico = 1.0f; }

    Elemento Fijo(const char* archivo, const char* carpeta, glm::vec3 pos, float yaw, float escala)
    {
        Elemento e; e.archivo = archivo; e.carpeta = carpeta; e.pos = pos; e.yaw = yaw; e.escala = escala;
        e.sobreSuelo = true; e.mov = MOV_FIJO;
        return e;
    }
    Elemento Nadador(const char* archivo, const char* carpeta, TipoMov mov, glm::vec3 centro, float radio, float vel, const char* etiqueta)
    {
        Elemento e; e.archivo = archivo; e.carpeta = carpeta; e.mov = mov; e.pos = centro; e.radio = radio; e.vel = vel;
        e.oscAmp = 0.4f; e.oscVel = 0.7f; e.ondula = 0.06f; e.interactivo = true; e.etiqueta = etiqueta;
        return e;
    }
    // Esparce 'n' copias de un elemento fijo alrededor de un centro
    void Esparcir(Elemento base, int n, glm::vec2 centro, float rMin, float rMax, float escMin, float escMax, int semilla, const std::vector<glm::vec3>& tintes = {})
    {
        for (int i = 0; i < n; i++) {
            Elemento e = base;
            float a = Aleatorio(semilla, i) * 6.2832f;
            float r = rMin + (rMax - rMin) * sqrtf(Aleatorio(semilla + 1, i));
            e.pos = glm::vec3(centro.x + cosf(a) * r, base.pos.y, centro.y + sinf(a) * r);
            e.yaw = Aleatorio(semilla + 2, i) * 360.0f;
            e.escala = escMin + (escMax - escMin) * Aleatorio(semilla + 3, i);
            e.fase = Aleatorio(semilla + 4, i) * 6.28f;
            if (!tintes.empty()) e.tinte = tintes[i % tintes.size()];
            elementos.push_back(e);
        }
    }

    // Ajustes exclusivos del arrecife, basados en las dimensiones de cada malla.
    void AjustarArrecife()
    {
        for (auto& e : elementos) {
            float largo = 0.0f;
            if (e.archivo == "CaballitoMar") largo = 0.20f;
            else if (e.archivo == "PezPayaso") largo = 0.12f;
            else if (e.archivo == "Pezcirujano") largo = 0.30f;
            else if (e.archivo == "PezGlobo") largo = 0.45f;
            else if (e.archivo == "langosta") largo = 0.50f;
            else if (e.archivo == "PepinoMar") largo = 0.35f;
            else if (e.archivo == "EstrellaCorona") largo = 0.45f;
            else if (e.archivo == "manati") largo = 3.0f;
            else if (e.archivo == "delfin") largo = 2.8f;
            else if (e.archivo == "barracuda") largo = 1.4f;
            else if (e.archivo == "vacamarina") largo = 1.5f;
            else if (e.archivo == "mantaraya") largo = 4.0f;
            if (largo == 0.0f || e.modelo->meshes.empty()) continue;
            glm::vec3 minimo(1e30f), maximo(-1e30f);
            for (auto& m : e.modelo->meshes) for (auto& v : m.vertices) {
                minimo = glm::min(minimo, v.Position);
                maximo = glm::max(maximo, v.Position);
            }
            glm::vec3 dim = maximo - minimo;
            float mayor = glm::max(dim.x, glm::max(dim.y, dim.z));
            if (mayor > 0.001f) e.escala = largo / mayor;
        }
        // Separar colonias entre si y de los objetos fijos del fondo.
        for (size_t i = 0; i < elementos.size(); ++i) {
            Elemento& e = elementos[i];
            bool colonia = e.archivo.find("coral_") == 0 || e.archivo == "esponja_tubo";
            if (!colonia) continue;
            glm::vec3 original = e.pos;
            for (int intento = 0; intento < 500; ++intento) {
                if (intento) {
                    float angulo = intento * 2.399963f;
                    float distancia = 0.45f * sqrtf((float)intento);
                    e.pos.x = original.x + cosf(angulo) * distancia;
                    e.pos.z = original.z + sinf(angulo) * distancia;
                }
                bool libre = true;
                for (size_t j = 0; j < elementos.size(); ++j) {
                    const Elemento& otro = elementos[j];
                    bool otraColonia = otro.archivo.find("coral_") == 0 || otro.archivo == "esponja_tubo";
                    if (j == i || (otraColonia && j > i) || otro.mov != MOV_FIJO || otro.archivo == "pasto_marino") continue;
                    float distancia = glm::length(glm::vec2(e.pos.x - otro.pos.x, e.pos.z - otro.pos.z));
                    if (distancia < e.radioBase * e.escala + otro.radioBase * otro.escala + 0.15f) { libre = false; break; }
                }
                if (libre) break;
            }
            e.baseY = e.sobreSuelo ? AlturaSuelo(e.pos.x, e.pos.z) : 0.0f;
        }
    }

    // ------------------------------------------------------------ Zona 1
    void ConstruirArrecifes()
    {
        cfg = ZonaConfig();
        cfg.nombre = "Arrecifes";
        cfg.niebla = glm::vec3(0.07f, 0.42f, 0.52f);
        cfg.densidad = 0.018f;
        cfg.solColor = glm::vec3(1.0f, 0.96f, 0.86f) * 1.35f;
        cfg.solDir = glm::vec3(-0.3f, -1.0f, -0.25f);
        cfg.ambArriba = glm::vec3(0.38f, 0.58f, 0.64f);
        cfg.ambAbajo = glm::vec3(0.13f, 0.23f, 0.27f);
        cfg.causticas = 1.0f;
        cfg.superficieY = 14.0f;                       // arrecife somero, lleno de luz
        cfg.inicioCamara = glm::vec3(0.0f, 8.0f, 20.0f);
        terrenoArchivo = "terreno_arrecifes"; terrenoCarpeta = "arrecifes";
        const char* c = "arrecifes";

        std::vector<glm::vec3> vivos = { {1.0f,0.42f,0.58f}, {0.72f,0.40f,0.95f}, {1.0f,0.56f,0.26f}, {1.0f,0.84f,0.32f}, {0.28f,0.85f,0.78f}, {0.95f,0.33f,0.30f} };
        Elemento roca = Fijo("roca_arrecife", c, glm::vec3(0.0f, -0.4f, 0.0f), 0, 1); roca.etiqueta = "Roca"; roca.interactivo = true;
        Esparcir(roca, 14, glm::vec2(0.0f), 4.0f, 32.0f, 0.8f, 2.6f, 11);

        // 22 Fondo + 10/11/12 corales en cuatro parches de arrecife
        glm::vec2 parches[] = { {-6.0f, -4.0f}, {7.0f, -9.0f}, {-1.0f, 8.0f}, {12.0f, 4.0f} };
        for (int p = 0; p < 4; p++) {
            Elemento r1 = Fijo("coral_rama1", c, glm::vec3(0.0f, -0.1f, 0.0f), 0, 1);
            r1.interactivo = true; r1.etiqueta = "Coral ramificado"; r1.esCoral = true;
            Elemento r2 = r1; r2.archivo = "coral_rama2";
            Elemento ce = Fijo("coral_cerebro", c, glm::vec3(0.0f, -0.15f, 0.0f), 0, 1);
            ce.interactivo = true; ce.etiqueta = "Coral cerebro"; ce.esCoral = true;
            Elemento ab = Fijo("coral_abanico", c, glm::vec3(0.0f, -0.1f, 0.0f), 0, 1);
            ab.interactivo = true; ab.etiqueta = "Abanico de mar"; ab.mecer = 0.15f; ab.esCoral = true;
            Elemento es = Fijo("esponja_tubo", c, glm::vec3(0.0f, -0.05f, 0.0f), 0, 1);
            es.interactivo = true; es.etiqueta = "Esponja de tubo";
            Esparcir(r1, 7, parches[p], 0.0f, 5.0f, 1.0f, 2.4f, 20 + p * 10, vivos);
            Esparcir(r2, 6, parches[p], 0.0f, 5.5f, 1.0f, 2.2f, 21 + p * 10, vivos);
            Esparcir(ce, 5, parches[p], 0.5f, 5.0f, 0.8f, 1.8f, 22 + p * 10, vivos);
            Esparcir(ab, 3, parches[p], 1.0f, 5.0f, 1.2f, 2.0f, 23 + p * 10, { {0.85f,0.30f,0.75f}, {1.0f,0.55f,0.30f} });
            Esparcir(es, 2, parches[p], 1.5f, 4.5f, 0.8f, 1.3f, 24 + p * 10, { {0.75f,0.45f,0.95f}, {1.0f,0.78f,0.30f}, {0.95f,0.50f,0.35f} });
            Elemento rs = r1; rs.archivo = "coral_rama_santiago";        // coral ramificado modelado por Santiago
            Esparcir(rs, 4, parches[p], 0.5f, 5.0f, 0.9f, 1.5f, 25 + p * 10, vivos);
        }
        for (size_t i = 0; i < elementos.size(); i++)
            if (elementos[i].esCoral) elementos[i].sensibilidad = 0.6f + 0.9f * Aleatorio(91, (int)i);

        // 16 Anemonas con 18 peces payaso
        glm::vec3 anem[] = { {-5.4f, 0.0f, -3.0f}, {-6.9f, 0.0f, -4.6f}, {-5.7f, 0.0f, -5.6f} };
        glm::vec3 tAnem[] = { {1.0f, 0.55f, 0.75f}, {0.6f, 1.0f, 0.65f}, {0.85f, 0.6f, 1.0f} };
        for (int i = 0; i < 3; i++) {
            Elemento a = Fijo("anemona", c, anem[i], (float)(i * 70), 1.3f);
            a.mecer = 0.8f; a.tinte = tAnem[i]; a.interactivo = true; a.etiqueta = "Anemona"; a.esCoral = true; a.sensibilidad = 0.9f; a.fase = (float)i;
            elementos.push_back(a);
        }
        Elemento payasos = Nadador("PezPayaso", c, MOV_CARDUMEN, glm::vec3(-6.0f, 0.9f, -4.4f), 1.1f, 0.6f, "Pez payaso");
		payasos.rotExtra = glm::vec3(0.0f, 180.0f, 0.0f);
        payasos.miembros = 6; payasos.dispersion = 0.5f; payasos.velNado = 12.0f; payasos.sobreSuelo = true; payasos.oscAmp = 0.2f;
        elementos.push_back(payasos);

        // Pez loro recorre el claro central con seis cuadros clave.
        Elemento pezLoro = Nadador("PezLoro", c, MOV_KF, glm::vec3(0.0f, 4.2f, 0.0f), 0.0f, 0.0f, "Pez loro");
        pezLoro.kf = GuardarKF(KF_PezLoro(pezLoro.pos));
        pezLoro.ondula = 0.05f; pezLoro.velNado = 4.0f; pezLoro.fase = 1.5f;
        elementos.push_back(pezLoro);

        // Estrella de corona de espinas (cerca de los parches de coral pero sin chocar)
        Elemento corona1 = Fijo("EstrellaCorona", c, glm::vec3(0.0f, 0.00f, 0.0f), 45, 1.0f);
        corona1.rotExtra = glm::vec3(-90.0f, 0.0f, 0.0f); // Acuesta la estrella horizontalmente
        corona1.interactivo = true;
        corona1.etiqueta = "Estrella corona de espinas";
		MatOpaco(corona1);
        elementos.push_back(corona1);

        Elemento corona2 = Fijo("EstrellaCorona", c, glm::vec3(-2.5f, 0.05f, 6.5f), 120, 0.9f);
        corona2.rotExtra = glm::vec3(-90.0f, 0.0f, 0.0f); // Acuesta la estrella horizontalmente
        corona2.interactivo = true;
        corona2.etiqueta = "Estrella corona de espinas";
		MatOpaco(corona2);
        elementos.push_back(corona2);

        // Pepino de mar (en zonas más arenosas / cerca del pasto marino)
        Elemento pepino1 = Fijo("PepinoMar", c, glm::vec3(-12.0f, 0.0f, 8.5f), 75, 1.0f);
		pepino1.rotExtra = glm::vec3(-90.0f, 0.0f, 0.0f); // Acuesta el pepino horizontalmente
        pepino1.interactivo = true;
        pepino1.etiqueta = "Pepino de mar";
		MatPlastico(pepino1);
        elementos.push_back(pepino1);

        Elemento pepino2 = Fijo("PepinoMar", c, glm::vec3(3.0f, 0.0f, 10.0f), 210, 1.1f);
		pepino2.rotExtra = glm::vec3(-90.0f, 0.0f, 0.0f); // Acuesta el pepino horizontalmente
        pepino2.interactivo = true;
        pepino2.etiqueta = "Pepino de mar";
		MatPlastico(pepino2);
        elementos.push_back(pepino2);

        // 17 Pradera de pasto marino con 08 manati y 03 caballitos de mar
        Elemento pasto = Fijo("pasto_marino", c, glm::vec3(0.0f, -0.05f, 0.0f), 0, 1);
        pasto.mecer = 1.6f; pasto.interactivo = true; pasto.etiqueta = "Pasto marino";
        Esparcir(pasto, 70, glm::vec2(-17.0f, 10.0f), 0.0f, 9.0f, 0.8f, 1.6f, 60, { {1.0f,1.0f,1.0f}, {0.85f,1.0f,0.8f}, {1.1f,1.05f,0.85f} });
        Elemento manati = Nadador("manati", c, MOV_ORBITA, glm::vec3(-17.0f, 1.8f, 10.0f), 6.0f, -0.07f, "Manati");
        manati.ondula = 0.05f; manati.velNado = 2.0f; manati.sobreSuelo = true; manati.ondulaVertical = true; elementos.push_back(manati);
        Elemento caballito = Fijo("CaballitoMar", c, glm::vec3(-14.0f, 0.7f, 7.5f), 70, 1.0f);
        caballito.interactivo = true; caballito.etiqueta = "Caballito de mar"; caballito.oscAmp = 0.2f; caballito.oscVel = 0.8f;
        elementos.push_back(caballito);
        caballito.pos = glm::vec3(-19.5f, 0.9f, 12.0f); caballito.yaw = 250; caballito.fase = 2.0f; elementos.push_back(caballito);
        caballito.pos = glm::vec3(-16.0f, 0.6f, 14.0f); caballito.yaw = 130; caballito.fase = 4.0f; elementos.push_back(caballito);

        // 07 Almejas, 05 pulpo y 15 langostas en el fondo
        Elemento almeja = Fijo("Almeja", c, glm::vec3(7.5f, 0.2f, -6.0f), 30, 1.0f);
        almeja.interactivo = true; almeja.etiqueta = "Almeja gigante"; almeja.esCoral = true; almeja.sensibilidad = 0.7f;
        almeja.tinte = glm::vec3(0.75f, 0.85f, 1.15f);
        {   // KF: la almeja abre y cierra la concha (animacion por cuadros clave de Julian)
            Elemento ak = almeja;
            ak.mov = MOV_KF; ak.kf = GuardarKF(KF_AlmejaAbre(glm::vec3(7.5f, AlturaSuelo(7.5f, -6.0f) + 0.2f, -6.0f)));
            elementos.push_back(ak);
        }
        almeja.pos = glm::vec3(-2.5f, 0.2f, 11.5f); almeja.yaw = 200; almeja.escala = 0.8f; elementos.push_back(almeja);
        Elemento pulpo = Fijo("pulpo", c, glm::vec3(2.0f, 0.0f, 4.0f), 220, 1.0f);
        pulpo.interactivo = true; pulpo.etiqueta = "Pulpo"; pulpo.camuflaje = true; pulpo.mecer = 0.3f; pulpo.fase = 1.0f;
        elementos.push_back(pulpo);
        Elemento langosta = Fijo("langosta", c, glm::vec3(9.0f, 0.02f, -6.5f), 160, 1.0f);
		langosta.rotExtra = glm::vec3(-90.0f, 0.0f, 0.0f);
        langosta.interactivo = true; langosta.etiqueta = "Langosta";
        elementos.push_back(langosta);
        langosta.pos = glm::vec3(-3.5f, 0.02f, 10.5f); langosta.yaw = 20; elementos.push_back(langosta);
        langosta.pos = glm::vec3(13.5f, 0.02f, 2.0f); langosta.yaw = 300; elementos.push_back(langosta);

        // 21 Respiradero hidrotermal
        Elemento resp = Fijo("respiradero", c, glm::vec3(18.0f, -0.3f, -15.0f), 20, 1.2f);
        resp.interactivo = true; resp.etiqueta = "Respiradero hidrotermal";
        elementos.push_back(resp);

        // Fauna que nada
        Elemento cirujanos = Nadador("Pezcirujano", c, MOV_CARDUMEN, glm::vec3(7.0f, 2.6f, -9.0f), 4.0f, -0.4f, "Pez cirujano azul");
        cirujanos.miembros = 7; cirujanos.dispersion = 1.0f; cirujanos.velNado = 11.0f; cirujanos.sobreSuelo = true;
        elementos.push_back(cirujanos);
        Elemento globo = Nadador("PezGlobo", c, MOV_ORBITA, glm::vec3(-1.0f, 1.8f, 8.0f), 2.5f, 0.25f, "Pez globo");
		globo.rotExtra = glm::vec3(-90.0f, 0.0f, 0.0f);
        globo.inflable = true; globo.sobreSuelo = true; globo.ondula = 0.03f; globo.oscAmp = 0.3f; elementos.push_back(globo);
        globo.pos = glm::vec3(12.0f, 2.2f, 4.0f); globo.vel = -0.2f; globo.fase = 2.0f; elementos.push_back(globo);
        globo.pos = glm::vec3(1.0f, 2.5f, -1.0f); globo.radio = 4.0f; globo.vel = 0.15f; globo.fase = 4.0f; elementos.push_back(globo);
        Elemento tortuga = Nadador("tortuga", c, MOV_KF, glm::vec3(0.0f, 4.5f, 0.0f), 10.5f, 0.09f, "Tortuga marina");
        tortuga.ondula = 0.0f; tortuga.oscAmp = 0.0f; tortuga.oscVel = 1.0f;
        tortuga.kf = GuardarKF(KF_TortugaArrecife(tortuga.pos, 10.5f, 1.0f));
        elementos.push_back(tortuga);
        tortuga.pos = glm::vec3(-12.0f, 3.5f, 6.0f); tortuga.fase = 2.5f; tortuga.escala = 0.85f;
        tortuga.kf = GuardarKF(KF_TortugaArrecife(tortuga.pos, 6.0f, 0.85f));
        elementos.push_back(tortuga);
        Elemento manta = Nadador("mantaraya", c, MOV_LIBRE, glm::vec3(0.0f, 8.0f, -6.0f), 22.0f, 0.06f, "Mantarraya");
        manta.ondula = 0.0f; manta.oscAmp = 1.5f; manta.oscVel = 0.25f; elementos.push_back(manta);
        Elemento delfines = Nadador("delfin", c, MOV_CARDUMEN, glm::vec3(0.0f, 10.5f, 0.0f), 24.0f, 0.22f, "Delfin nariz de botella");
        delfines.miembros = 3; delfines.dispersion = 2.0f; delfines.ondula = 0.12f; delfines.velNado = 5.0f; delfines.ondulaVertical = true;
        delfines.oscAmp = 1.2f; delfines.oscVel = 0.9f; elementos.push_back(delfines);
        Elemento barracudas = Nadador("barracuda", "naufragios", MOV_CARDUMEN, glm::vec3(4.0f, 6.0f, 2.0f), 16.0f, -0.16f, "Barracuda");
        barracudas.miembros = 4; barracudas.dispersion = 1.5f; elementos.push_back(barracudas);
        Elemento vaquita = Nadador("vacamarina", "naufragios", MOV_ORBITA, glm::vec3(6.0f, 3.5f, 10.0f), 9.0f, 0.16f, "Vaquita marina");
        vaquita.ondula = 0.08f; vaquita.velNado = 4.0f; vaquita.sobreSuelo = true; vaquita.ondulaVertical = true; elementos.push_back(vaquita);

        // 23 Burbujas: respiradero y grietas del arrecife
        fuentes = { glm::vec3(18.0f, 3.6f, -15.0f), glm::vec3(18.4f, 2.4f, -14.4f), glm::vec3(-6.0f, 0.4f, -4.0f) };
    }

    // ------------------------------------------------------------ Zona 2
    void ConstruirAguasFrias()
    {
        cfg = ZonaConfig();
        cfg.nombre = "Aguas Frias";
        cfg.niebla = glm::vec3(0.20f, 0.34f, 0.44f);
        cfg.densidad = 0.028f;
        cfg.solColor = glm::vec3(0.85f, 0.93f, 1.0f) * 0.95f;
        cfg.solDir = glm::vec3(0.2f, -1.0f, 0.35f);
        cfg.ambArriba = glm::vec3(0.40f, 0.50f, 0.60f);
        cfg.ambAbajo = glm::vec3(0.10f, 0.14f, 0.20f);
        cfg.causticas = 0.3f;
        cfg.superficieY = 16.0f;
        cfg.inicioCamara = glm::vec3(0.0f, 8.0f, 24.0f);
        terrenoArchivo = "terreno_aguas_frias"; terrenoCarpeta = "aguas_frias";
        const char* c = "aguas_frias";

        Elemento roca = Fijo("roca_fria", c, glm::vec3(0.0f, -0.5f, 0.0f), 0, 1); roca.etiqueta = "Roca";
        Esparcir(roca, 16, glm::vec2(0.0f), 3.0f, 34.0f, 0.8f, 2.8f, 12);
        // plataformas de hielo: el buzo puede subirse; los animales descansan, se lanzan al agua y regresan
        const float sy = cfg.superficieY;
        auto hacerPlataforma = [&](const char* archivo, float x, float z, float esc, float yaw) {
            Elemento h = Fijo(archivo, c, glm::vec3(x, sy + 0.7f, z), yaw, esc);
            h.sobreSuelo = false; h.interactivo = true; h.etiqueta = "Plataforma de hielo"; h.caminable = true;
            h.tinte = glm::vec3(0.85f, 0.95f, 1.05f); h.emision = 0.55f;   // luz que atraviesa el hielo
            MatTraslucido(h); h.alpha = 0.9f;
            elementos.push_back(h);
        };
        // habitante de una plataforma: n individuos que se turnan para zambullirse hacia el centro de la zona
        auto habitante = [&](const char* archivo, const char* etiqueta, float x, float z, float esc, int n, float disp, float radio, float periodo, float fase, float salto, float altura) {
            Elemento a = Nadador(archivo, c, MOV_ZAMBULLIDA, glm::vec3(x, sy + 0.7f, z), radio, salto, etiqueta);
            a.sobreSuelo = false; a.sobreHielo = true; a.miembros = n; a.dispersion = disp; a.periodo = periodo; a.fase = fase;
            a.rumbo = atan2f(-z, -x); a.alcance = 5.0f * esc + 1.5f; a.profundidad = 2.6f; a.nivelAgua = sy; a.oscAmp = altura;
            a.ondula = 0.0f; a.velNado = 3.0f;
            elementos.push_back(a);
            return elementos.size() - 1;
        };
        hacerPlataforma("hielo1", 18.0f, 24.0f, 1.4f, 20.0f);
        hacerPlataforma("hielo2", -6.0f, 26.0f, 1.2f, 100.0f);
        hacerPlataforma("hielo1", -30.0f, 18.0f, 1.4f, 200.0f);
        hacerPlataforma("hielo2", 28.0f, -24.0f, 1.2f, 40.0f);
        hacerPlataforma("hielo1", -10.0f, -26.0f, 1.3f, 300.0f);
        hacerPlataforma("hielo2", 8.0f, -32.0f, 1.0f, 150.0f);

        // la cueva de hielo: un oso polar vigila desde la cima
        {
            Elemento cueva = Fijo("cueva_meshy", c, glm::vec3(-36.0f, sy + 2.6f, -26.0f), 30.0f, 8.0f);
            cueva.sobreSuelo = false; cueva.interactivo = true; cueva.etiqueta = "Cueva de hielo"; cueva.caminable = true;
            cueva.specInt = 0.35f; cueva.specPower = 48.0f;
            elementos.push_back(cueva);
            Elemento oso2 = Fijo("osito", c, glm::vec3(-36.0f, sy + 6.0f, -26.0f + 2.3f), 200.0f, 1.0f);
            oso2.sobreSuelo = false; oso2.sobreHielo = true; oso2.interactivo = true; oso2.etiqueta = "Oso polar";
            elementos.push_back(oso2);
        }
        // icebergs de Meshy (la mayor parte queda bajo el agua)
        {
            Elemento ice = Fijo("iceberg_meshy", c, glm::vec3(38.0f, sy, 30.0f), 30.0f, 11.0f);
            ice.sobreSuelo = false; ice.etiqueta = "Iceberg"; ice.interactivo = true;
            ice.tinte = glm::vec3(0.9f, 0.98f, 1.05f); ice.emision = 0.3f; ice.specInt = 0.35f; ice.specPower = 64.0f;
            elementos.push_back(ice);
            ice.pos = glm::vec3(-12.0f, sy, -48.0f); ice.yaw = 140.0f; ice.escala = 9.0f; elementos.push_back(ice);
        }

        // rompehielos: cruza la zona de lado a lado por un canal libre de hielo
        {
            Elemento barco = Fijo("rompehielos", c, glm::vec3(0.0f, sy + 0.31f * 11.0f, 0.0f), 0.0f, 11.0f);
            barco.sobreSuelo = false; barco.interactivo = true; barco.etiqueta = "Rompehielos";
            barco.mov = MOV_BARCO; barco.vel = 3.2f; barco.reloj = 25.0f; barco.rotExtra = glm::vec3(0.0f, 180.0f, 0.0f);
            barco.specInt = 0.5f; barco.specPower = 40.0f; barco.metalico = 0.3f;
            elementos.push_back(barco);
        }

        Elemento ballena = Nadador("ballena", c, MOV_RUTA, glm::vec3(0.0f, 9.0f, -4.0f), 30.0f, 0.06f, "Ballena azul");
        ballena.ondula = 0.25f; ballena.velNado = 1.5f; ballena.oscAmp = 3.5f; ballena.ondulaVertical = true;
        elementos.push_back(ballena);
        Elemento belugas = Nadador("beluga", c, MOV_CARDUMEN, glm::vec3(0.0f, 7.0f, 4.0f), 15.0f, -0.12f, "Beluga");
        belugas.miembros = 3; belugas.dispersion = 2.0f; belugas.ondula = 0.12f; belugas.velNado = 3.0f;
        elementos.push_back(belugas);

        // animales del hielo (cada plataforma tiene su especie)
        {
            size_t i0 = habitante("osito", "Oso polar", 18.0f, 24.0f, 1.4f, 1, 0.0f, 2.8f, 40.0f, 0.0f, 2.4f, 0.7f);
            elementos[i0].escala = 1.1f;
            size_t i1 = habitante("pinguino", "Pinguino", -6.0f, 26.0f, 1.2f, 5, 1.1f, 2.4f, 22.0f, 1.0f, 1.2f, 1.9f);
            elementos[i1].rotNado = glm::vec3(0.0f, 0.0f, -90.0f); elementos[i1].escala = 1.5f;
            habitante("foca", "Foca", -30.0f, 18.0f, 1.4f, 2, 1.5f, 2.6f, 30.0f, 2.2f, 1.6f, 0.9f);
            size_t i3 = habitante("morsa", "Morsa", 28.0f, -24.0f, 1.2f, 2, 2.0f, 2.8f, 46.0f, 3.1f, 2.4f, 0.5f);
            elementos[i3].escala = 1.1f;
            habitante("LeonMarino2", "Leon marino", -10.0f, -26.0f, 1.3f, 2, 1.8f, 2.6f, 26.0f, 4.0f, 1.3f, 1.4f);
            size_t i5 = habitante("pinguino", "Pinguino", 8.0f, -32.0f, 1.0f, 4, 1.0f, 2.2f, 24.0f, 5.0f, 1.2f, 1.9f);
            elementos[i5].rotNado = glm::vec3(0.0f, 0.0f, -90.0f); elementos[i5].escala = 1.5f;
        }
        // focas que nadan en mar abierto, lentas
        Elemento focas = Nadador("foca", c, MOV_CARDUMEN, glm::vec3(4.0f, 9.0f, -4.0f), 11.0f, 0.13f, "Foca");
        focas.miembros = 3; focas.dispersion = 1.8f; focas.ondula = 0.1f; focas.velNado = 2.5f; focas.oscAmp = 0.9f;
        elementos.push_back(focas);
        Elemento krill = Nadador("Krill", c, MOV_CARDUMEN, glm::vec3(-4.0f, 6.0f, -6.0f), 5.0f, 0.25f, "Krill");
        krill.miembros = 60; krill.dispersion = 2.5f; krill.escala = 1.4f; krill.ondula = 0.0f; krill.oscAmp = 0.6f;
        elementos.push_back(krill);
        Elemento bacalaos = Nadador("bacalao", c, MOV_CARDUMEN, glm::vec3(6.0f, 3.5f, 6.0f), 8.0f, -0.3f, "Bacalao");
        bacalaos.miembros = 9; bacalaos.dispersion = 1.6f; bacalaos.sobreSuelo = true; elementos.push_back(bacalaos);
        Elemento pescados = Nadador("pescado", "arrecifes", MOV_CARDUMEN, glm::vec3(-10.0f, 5.0f, 8.0f), 6.0f, 0.3f, "Pescado");
        pescados.miembros = 5; pescados.dispersion = 1.4f; elementos.push_back(pescados);

        // Orca2, Narwhal2, Cachalote2 y LeonMarino2 ya vienen con Y arriba (exporta_al_juego.py): sin rotExtra
        Elemento orcas = Nadador("Orca2", c, MOV_CARDUMEN, glm::vec3(-12.0f, 7.0f, -14.0f), 18.0f, 0.12f, "Orca");
        orcas.miembros = 3; orcas.dispersion = 3.0f; orcas.ondula = 0.08f; orcas.velNado = 4.0f; orcas.oscAmp = 0.8f;
        elementos.push_back(orcas);
        Elemento narvales = Nadador("Narwhal2", c, MOV_CARDUMEN, glm::vec3(14.0f, 9.0f, -18.0f), 12.0f, -0.10f, "Narval");
        narvales.miembros = 2; narvales.dispersion = 2.5f; narvales.ondula = 0.08f; narvales.velNado = 4.0f; narvales.oscAmp = 0.6f;
        elementos.push_back(narvales);
        Elemento cachalote = Nadador("Cachalote2", c, MOV_LIBRE, glm::vec3(0.0f, 5.0f, -22.0f), 22.0f, 0.05f, "Cachalote");
        cachalote.tinte = glm::vec3(0.55f, 0.65f, 0.8f); cachalote.ondula = 0.10f; cachalote.velNado = 2.5f; cachalote.oscAmp = 1.2f; cachalote.oscVel = 0.2f;
        elementos.push_back(cachalote);
        Elemento calamar = Nadador("Calamar2", c, MOV_ORBITA, glm::vec3(-14.0f, 3.0f, 10.0f), 8.0f, 0.08f, "Calamar colosal");
        calamar.ondula = 0.15f; calamar.velNado = 3.0f; calamar.oscAmp = 1.0f;
        elementos.push_back(calamar);
        Elemento leones = Nadador("LeonMarino2", c, MOV_CARDUMEN, glm::vec3(10.0f, 10.0f, 14.0f), 9.0f, 0.30f, "Leon marino");
        leones.miembros = 3; leones.dispersion = 1.8f; leones.ondula = 0.10f; leones.velNado = 6.0f; leones.oscAmp = 1.0f;
        elementos.push_back(leones);
        Elemento dracoides = Nadador("pezdracoide", "arrecifes", MOV_CARDUMEN, glm::vec3(-2.0f, 4.0f, 14.0f), 6.0f, 0.35f, "Pez dracoide");
        dracoides.miembros = 6; dracoides.dispersion = 1.4f; dracoides.tinte = glm::vec3(0.75f, 0.9f, 1.1f);
        elementos.push_back(dracoides);
        fuentes.clear();
    }

    // ------------------------------------------------------------ Zona 3
    void ConstruirNaufragios()
    {
        cfg = ZonaConfig();
        cfg.nombre = "Naufragios";
        cfg.niebla = glm::vec3(0.035f, 0.075f, 0.060f);   // agua turbia: empieza oscura y se aclara al limpiar
        cfg.densidad = 0.045f;
        cfg.solColor = glm::vec3(0.40f, 0.52f, 0.46f) * 0.6f;
        cfg.solDir = glm::vec3(0.1f, -1.0f, 0.1f);
        cfg.ambArriba = glm::vec3(0.12f, 0.18f, 0.16f);
        cfg.ambAbajo = glm::vec3(0.02f, 0.04f, 0.04f);
        cfg.causticas = 0.05f;
        cfg.superficieY = 22.0f;
        cfg.linterna = true;
        cfg.inicioCamara = glm::vec3(0.0f, 5.0f, 22.0f);
        terrenoArchivo = "terreno_naufragios"; terrenoCarpeta = "naufragios";
        const char* c = "naufragios";
        auto suelo = [&](float x, float z) { return AlturaSuelo(x, z); };
        auto fijo = [&](const char* archivo, glm::vec3 p, float yaw, float esc, const char* etiqueta) {
            Elemento e = Fijo(archivo, c, p, yaw, esc); e.interactivo = etiqueta != nullptr; if (etiqueta) e.etiqueta = etiqueta; return e;
        };

        // ---------------- 57 fondo de grava (terreno) + rocas
        Elemento roca = fijo("roca_naufragio", glm::vec3(0.0f, -0.4f, 0.0f), 0, 1, "Roca"); MatOpaco(roca);
        Esparcir(roca, 12, glm::vec2(0.0f), 7.0f, 32.0f, 0.8f, 2.4f, 14);

        // ---------------- letreros informativos
        struct L { const char* a; glm::vec3 p; float yaw; const char* et; };
        L letreros[] = { { "letrero_bienvenida", {0.0f, 0.0f, 15.0f}, 0.0f, "Letrero: como limpiar" },
                         { "letrero_plasticos", {6.5f, 0.0f, 9.0f}, -35.0f, "Letrero: plasticos" },
                         { "letrero_redes", {0.5f, 0.0f, -8.5f}, -15.0f, "Letrero: redes fantasma" },
                         { "letrero_invasor", {-8.0f, 0.0f, 3.5f}, 60.0f, "Letrero: pez leon" },
                         { "letrero_toxicos", {12.5f, 0.0f, 5.0f}, -45.0f, "Letrero: residuos toxicos" } };
        for (auto& l : letreros) { Elemento e = fijo(l.a, l.p, l.yaw, 1.15f, l.et); MatOpaco(e); e.emision = 0.25f; elementos.push_back(e); }

        // ---------------- 52 submarino hundido: se asienta en el fondo al llegar (KF, una vez)
        Elemento sub = fijo("submarino", glm::vec3(0.0f), 25, 1.0f, "Submarino hundido");
        sub.mov = MOV_KF; sub.kf = GuardarKF([&]() { AnimacionKF a = KF_SubmarinoAsienta(glm::vec3(3.0f, suelo(5.4f, -4.4f) + 0.6f, -2.0f)); a.loop = false; return a; }());
        sub.kfInicio = 1.5f; MatMetalico(sub); elementos.push_back(sub);

        // ---------------- 53/54 ruinas (modelos de Meshy): arcadas, balcones y columnas clasicas
        // el marmol es casi blanco: el tinte lo envejece con sedimento y algas
        const glm::vec3 marmolHundido(0.52f, 0.58f, 0.50f);
        Elemento arcada = fijo("ruinas_arcada", glm::vec3(-16.5f, -0.3f, -1.5f), 90, 1.0f, "Ruinas"); MatOpaco(arcada); arcada.tinte = marmolHundido;
        elementos.push_back(arcada);                                         // muro principal de arcos al fondo
        arcada.pos = glm::vec3(-13.0f, -0.4f, 13.5f); arcada.yaw = 15; arcada.escala = 0.75f;
        arcada.rotExtra = glm::vec3(0.0f, 0.0f, -4.0f); elementos.push_back(arcada);   // arcada menor, ligeramente inclinada
        Elemento balcon = fijo("ruinas_balcon", glm::vec3(-20.5f, -0.4f, 7.0f), 60, 1.0f, "Ruinas"); MatOpaco(balcon); balcon.tinte = marmolHundido;
        elementos.push_back(balcon);
        balcon.pos = glm::vec3(-14.5f, -0.4f, -15.5f); balcon.yaw = 200; balcon.escala = 0.85f; elementos.push_back(balcon);
        // pasillo de columnas que lleva hacia la arcada
        float colX[] = { -12.5f, -12.5f, -9.5f, -9.5f }, colZ[] = { -6.0f, 1.0f, -6.0f, 1.0f };
        float inclina[] = { 2.0f, -3.0f, 0.0f, 4.0f };
        for (int i = 0; i < 4; i++) {
            Elemento col = fijo("columna", glm::vec3(colX[i], -0.3f, colZ[i]), (float)(i * 23), 1.0f, "Columna");
            col.rotExtra = glm::vec3(inclina[i], 0.0f, inclina[3 - i]); MatOpaco(col); col.tinte = marmolHundido; elementos.push_back(col);
        }
        Elemento tumbada = fijo("columna", glm::vec3(-11.0f, 0.55f, 7.5f), 30, 0.95f, "Columna");   // ya derrumbada
        tumbada.rotExtra = glm::vec3(0.0f, 0.0f, 88.0f); MatOpaco(tumbada); tumbada.tinte = marmolHundido * 0.9f; elementos.push_back(tumbada);
        Elemento caida = fijo("columna", glm::vec3(0.0f), 0, 0.9f, "Columna");   // se derrumba cuando te acercas (KF)
        caida.mov = MOV_KF; caida.kf = GuardarKF(KF_ColumnaDerrumbe(glm::vec3(-5.5f, suelo(-5.5f, 7.0f) - 0.2f, 7.0f), 75.0f));
        caida.kfInicio = 1e6f; MatOpaco(caida); caida.tinte = marmolHundido; elementos.push_back(caida);
        Elemento bloque = fijo("bloque", glm::vec3(0.0f, 0.2f, 0.0f), 0, 1, "Ruinas"); MatOpaco(bloque);
        Esparcir(bloque, 8, glm::vec2(-13.0f, 1.0f), 2.0f, 8.0f, 0.7f, 1.3f, 13);

        // ---------------- 51 ancla, 66 timon, 74 senal de transito, 49 dron
        Elemento ancla = fijo("ancla", glm::vec3(2.0f, -0.35f, -10.0f), 60, 1.0f, "Ancla oxidada"); ancla.rotExtra = glm::vec3(0.0f, 0.0f, -38.0f); MatMetalico(ancla); elementos.push_back(ancla);
        Elemento barco = fijo("BarcoRuinas", glm::vec3(-8.0f, 2.0f, -21.0f), 20, 1.0f, "Barco hundido");   // modelo de Julian
        MatOpaco(barco); elementos.push_back(barco);
        Elemento timon = fijo("timon", glm::vec3(-13.6f, 0.45f, -6.5f), 30, 1.0f, "Timon"); timon.rotExtra = glm::vec3(72.0f, 0.0f, 0.0f); MatOpaco(timon); elementos.push_back(timon);
        Elemento senal = fijo("senal", glm::vec3(-6.5f, -0.25f, 11.0f), 200, 1.0f, "Senal de transito"); senal.rotExtra = glm::vec3(0.0f, 0.0f, 24.0f); MatMetalico(senal); elementos.push_back(senal);
        Elemento dron = fijo("dron", cfg.inicioCamara + glm::vec3(-1.5f, 0.0f, -2.6f), 0, 0.55f, "Dron de exploracion");
        dron.sobreSuelo = false; dron.emision = 0.3f; MatPlastico(dron); dron.oscAmp = 0.08f; dron.oscVel = 1.6f; elementos.push_back(dron);

        // ---------------- 55/56 cofre del tesoro (se abre al limpiar todo) y monedas
        glm::vec3 pc(1.5f, suelo(1.5f, 5.0f) - 0.05f, 5.0f);
        Elemento cofre = fijo("cofre", glm::vec3(pc.x, -0.05f, pc.z), 0, 1.0f, "Cofre del tesoro"); MatOpaco(cofre); elementos.push_back(cofre);
        Elemento tapa = fijo("cofre_tapa", glm::vec3(0.0f), 0, 1.0f, "Cofre del tesoro");
        tapa.mov = MOV_KF; tapa.kf = GuardarKF(KF_TapaCofre(pc + glm::vec3(0.0f, 0.507f, -0.315f), 0.0f)); tapa.kfInicio = 1e6f; MatOpaco(tapa);   // bisagra del cofre de Meshy
        elementos.push_back(tapa);
        Elemento monedas = fijo("moneda", glm::vec3(pc.x + 0.7f, 0.0f, pc.z + 0.5f), 0, 1.0f, nullptr);
        monedas.miembros = 30; monedas.dispersion = 0.9f; monedas.presencia = 0.25f; monedas.emision = 0.35f; MatMetalico(monedas); elementos.push_back(monedas);

        // ---------------- residuos (se recogen con clic)
        auto residuo = [&](const char* archivo, const char* etiqueta, int tipo, float peso, int n, int semilla, bool deriva, float altura, glm::vec2 centro, float radio, float escala) {
            for (int i = 0; i < n; i++) {
                float a = Aleatorio(semilla, i) * 6.28f, r = 2.0f + radio * sqrtf(Aleatorio(semilla + 1, i));
                Elemento b = fijo(archivo, glm::vec3(centro.x + cosf(a) * r, altura, centro.y + sinf(a) * r), Aleatorio(semilla + 2, i) * 360.0f, 1.0f, etiqueta);
                if (!deriva) b.rotExtra = glm::vec3((Aleatorio(semilla + 3, i) - 0.5f) * 30.0f, 0.0f, Aleatorio(semilla + 4, i) > 0.4f ? 88.0f : 0.0f);
                b.mov = deriva ? MOV_DERIVA : MOV_FIJO; b.fase = (float)i * 1.7f;
                b.basura = true; b.residuo = tipo; b.peso = peso; b.escala = escala;   // mas grandes que en la realidad: faciles de ver y tomar
                elementos.push_back(b);
            }
        };
        size_t ini = elementos.size();
        residuo("bolsaplastico", "Bolsa de plastico", 1, 1.0f, 6, 83, true, 3.0f, glm::vec2(0.0f, 0.0f), 16.0f, 1.5f);
        residuo("botella", "Botella PET", 1, 1.0f, 4, 84, true, 2.5f, glm::vec2(2.0f, 3.0f), 14.0f, 1.8f);
        residuo("botella", "Botella PET", 1, 1.0f, 3, 85, false, 0.07f, glm::vec2(-3.0f, -2.0f), 12.0f, 1.8f);
        residuo("bota", "Bota de hule", 1, 1.0f, 3, 86, false, 0.08f, glm::vec2(-10.0f, 4.0f), 8.0f, 1.5f);
        residuo("Llanta", "Llanta", 1, 2.0f, 3, 82, false, 0.15f, glm::vec2(4.0f, 6.0f), 10.0f, 1.0f);
        residuo("lata", "Lata de aluminio", 2, 1.0f, 7, 80, false, 0.14f, glm::vec2(2.0f, 2.0f), 14.0f, 1.9f);
        residuo("lataaplastada", "Lata aplastada", 2, 1.0f, 5, 81, false, 0.10f, glm::vec2(-4.0f, 0.0f), 12.0f, 2.0f);
        residuo("pila", "Pila", 3, 1.0f, 6, 87, false, 0.06f, glm::vec2(6.0f, -2.0f), 10.0f, 2.5f);
        for (size_t i = ini; i < elementos.size(); i++) {
            Elemento& b = elementos[i];
            if (b.archivo == "lata" || b.archivo == "lataaplastada" || b.archivo == "pila") MatMetalico(b);
            else if (b.archivo == "bolsaplastico") { b.alpha = 0.55f; MatTraslucido(b); }
            else if (b.archivo == "botella") { b.alpha = 0.65f; MatTraslucido(b); b.fantasma = 0.15f; }
            else MatPlastico(b);
        }
        Elemento bolsaKF = fijo("bolsaplastico", glm::vec3(0.0f), 0, 1.0f, "Bolsa de plastico");   // KF de Julian: recorre puntos de flotacion
        bolsaKF.mov = MOV_KF; bolsaKF.kf = &KF_Bolsa(); bolsaKF.escala = 1.5f; bolsaKF.basura = true; bolsaKF.residuo = 1; bolsaKF.alpha = 0.55f; MatTraslucido(bolsaKF);
        elementos.push_back(bolsaKF);

        // 64 barril toxico con su nube verde
        Elemento barril = fijo("barril", glm::vec3(9.5f, -0.1f, 1.5f), 20, 1.0f, "Barril de residuos toxicos");
        barril.rotExtra = glm::vec3(8.0f, 0.0f, -12.0f); barril.basura = true; barril.residuo = 3; barril.peso = 5.0f; MatMetalico(barril);
        elementos.push_back(barril);
        Elemento nube = fijo("burbuja", glm::vec3(9.5f, 0.9f, 1.5f), 0, 3.2f, nullptr);
        nube.carpeta = "comun"; nube.tinte = glm::vec3(0.45f, 1.0f, 0.35f); nube.alpha = 0.30f; nube.emision = 0.7f; nube.pulsar = true; nube.fantasma = 0.4f;
        elementos.push_back(nube);

        // 63 red de pesca fantasma con una tortuga atrapada
        Elemento red = fijo("red_fantasma", glm::vec3(-3.5f, -0.2f, -13.0f), 15, 1.0f, "Red fantasma");
        red.residuo = 4; red.peso = 6.0f; MatOpaco(red); elementos.push_back(red);
        Elemento tortuga = Fijo("tortuga", "arrecifes", glm::vec3(-3.5f, 1.7f, -13.0f), 15, 1.0f);
        tortuga.interactivo = true; tortuga.etiqueta = "Tortuga atrapada"; elementos.push_back(tortuga);

        // ---------------- 67/68 cangrejos ermitanos (el de la lata cambia de concha al limpiar)
        Elemento cLata = fijo("cangrejo_lata", glm::vec3(4.0f, 0.12f, 7.5f), 200, 1.0f, "Cangrejo con lata"); MatMetalico(cLata); cLata.specInt = 0.6f; elementos.push_back(cLata);
        Elemento cNat = Fijo("cangrejoermitano", "arrecifes", glm::vec3(4.0f, 0.15f, 7.5f), 200, 1.0f); cNat.interactivo = true; cNat.etiqueta = "Cangrejo ermitano"; cNat.visible = false;
        elementos.push_back(cNat);
        cNat.pos = glm::vec3(-10.5f, 0.15f, -1.0f); cNat.yaw = 40; cNat.visible = true; elementos.push_back(cNat);

        // ---------------- 71 erizos y 73 estrellas de mar
        Elemento erizo = fijo("erizo", glm::vec3(0.0f, 0.0f, 0.0f), 0, 1, "Erizo de mar"); MatOpaco(erizo);
        Esparcir(erizo, 7, glm::vec2(-8.0f, -2.0f), 1.0f, 9.0f, 0.8f, 1.3f, 15);
        Elemento estrella = fijo("estrella_mar", glm::vec3(0.0f, 0.01f, 0.0f), 0, 1, "Estrella de mar"); MatOpaco(estrella);
        Esparcir(estrella, 6, glm::vec2(3.0f, 2.0f), 2.0f, 12.0f, 0.8f, 1.4f, 16, { {1.0f,1.0f,1.0f}, {1.0f,0.7f,0.75f}, {0.9f,0.6f,1.0f} });

        // ---------------- 65 peces leon (invasores, se retiran con clic)
        glm::vec4 leones[] = { {-12.0f, 2.5f, 0.0f, 2.5f}, {-9.0f, 2.0f, -5.0f, 2.0f}, {6.0f, 2.5f, -6.0f, 3.0f}, {-3.0f, 3.0f, 8.0f, 2.5f}, {10.0f, 2.0f, 3.0f, 2.0f}, {-15.0f, 3.0f, 6.0f, 3.0f} };
        for (int i = 0; i < 6; i++) {
            Elemento l = Nadador("pezleon", c, MOV_ORBITA, glm::vec3(leones[i]), leones[i].w, (i % 2 ? -0.25f : 0.22f), "Pez leon (invasor)");
            l.sobreSuelo = true; l.velNado = 4.0f; l.fase = i * 1.1f; l.invasor = true; elementos.push_back(l);
        }
        // peces nativos: pocos al inicio, aumentan al retirar invasores y limpiar
        Elemento roncadores = Nadador("roncador", c, MOV_CARDUMEN, glm::vec3(1.0f, 3.5f, 1.0f), 7.0f, 0.3f, "Roncador");
        roncadores.miembros = 18; roncadores.dispersion = 1.4f; roncadores.velNado = 12.0f; roncadores.sobreSuelo = true; roncadores.presencia = 0.15f;
        elementos.push_back(roncadores);
        Elemento cirujanos = Nadador("Pezcirujano", "arrecifes", MOV_CARDUMEN, glm::vec3(-6.0f, 4.5f, -1.0f), 9.0f, -0.25f, "Pez cirujano azul");
        cirujanos.miembros = 10; cirujanos.dispersion = 1.2f; cirujanos.velNado = 11.0f; cirujanos.sobreSuelo = true; cirujanos.presencia = 0.15f;
        elementos.push_back(cirujanos);

        // ---------------- 48 tiburon blanco y 69 medusas
        Elemento tiburon = Nadador("Tiburon", c, MOV_ORBITA, glm::vec3(0.0f, 9.0f, 0.0f), 20.0f, 0.10f, "Tiburon");
        tiburon.ondula = 0.15f; tiburon.velNado = 3.0f; elementos.push_back(tiburon);
        for (int i = 0; i < 6; i++) {
            Elemento m = fijo("medusa", glm::vec3(-12.0f + i * 5.0f, 5.0f + (i % 3) * 2.5f, -6.0f + (i % 2) * 12.0f), 0, 0.8f + 0.1f * i, "Medusa");
            m.sobreSuelo = false; m.mov = MOV_DERIVA; m.fase = i * 1.9f; m.pulsar = true; m.alpha = 0.78f; m.emision = 0.30f; m.fantasma = 0.25f;
            m.mecer = 0.5f; MatTraslucido(m); elementos.push_back(m);
        }

        // ---------------- 23 burbujas: fugas del submarino
        fuentes = { glm::vec3(5.0f, 1.6f, -4.5f), glm::vec3(7.5f, 2.3f, -6.0f) };
    }

};

#endif
