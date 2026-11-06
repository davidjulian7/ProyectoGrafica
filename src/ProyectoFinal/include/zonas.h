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
    MOV_KF          // animacion por cuadros clave (Elemento::kf)
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
        burbujaModelo = Cargar("burbuja", "comun");
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

    // Guarda una animacion por cuadros clave propia de la zona (direccion estable)
    const AnimacionKF* GuardarKF(const AnimacionKF& a) { animaciones.push_back(a); return &animaciones.back(); }

    // Devuelve true en el cuadro en que el buzo exhala (para el sonido de burbujas)
    bool Actualizar(float dt, float t, const Camera& cam)
    {
        if (dt > 0.1f) dt = 0.1f;
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
            unsigned int id = e.interactivo ? e.id : 0xFFFFFF;
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

    void Dibujar(const glm::mat4& proj, const glm::mat4& view, const Camera& cam, float t, int fbW, int fbH, const Camera* camBuzo = nullptr)
    {
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
            DibujarElemento(e, (int)i, t);
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
    std::vector<float> alturas;
    static const int N = 111;
    float respiracion = 3.0f, fuenteTiempo = 0.0f;
public:
    float velFauna = 0.5f;             // multiplica la velocidad de nado de toda la fauna (mas lento = mas facil de observar)
private:

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

    // Matriz de modelo de un miembro k del elemento en el tiempo t
    glm::mat4 Matriz(const Elemento& e, int idx, int k, float t) const
    {
        glm::vec3 p = e.pos, dir(1.0f, 0.0f, 0.0f);
        bool orientar = false;
        float signo = e.vel >= 0.0f ? 1.0f : -1.0f;
        glm::vec3 off = k > 0 ? Aleatorio3(idx * 97 + k) * e.dispersion : glm::vec3(0.0f);
        float yawUsar = e.yaw;
        glm::vec3 rotUsar = e.rotExtra;
        float escUsar = e.escala;
        switch (e.mov) {
        case MOV_FIJO:
            p.y += e.baseY + sinf(t * e.oscVel + e.fase) * e.oscAmp;
            if (k > 0) { p += glm::vec3(off.x, 0.0f, off.z); yawUsar += k * 137.5f; }   // copias esparcidas (monedas)
            break;
        case MOV_KF:
            if (e.kf) e.kf->PoseAt(t - e.kfInicio + (e.kf->loop ? e.fase : 0.0f), p, yawUsar, rotUsar, escUsar);
            break;
        case MOV_ORBITA:
        case MOV_CARDUMEN: {
            float a = t * e.vel * velFauna + e.fase - (k > 0 ? (0.15f + 0.25f * Aleatorio(idx, k)) * signo * (float)(k % 7) * 0.3f : 0.0f);
            float r = e.radio + off.x;
            p = e.pos + glm::vec3(cosf(a) * r, off.y + sinf(t * e.oscVel + e.fase + k) * e.oscAmp, sinf(a) * r);
            p.y += e.baseY;
            dir = glm::vec3(-sinf(a), 0.0f, cosf(a)) * signo;
            orientar = true;
            break; }
        case MOV_LIBRE: {
            float a = t * e.vel * velFauna + e.fase;
            p = e.pos + glm::vec3(e.radio * sinf(a), sinf(t * e.oscVel + e.fase) * e.oscAmp, e.radio * 0.6f * sinf(2.0f * a)) + off;
            dir = glm::vec3(e.radio * cosf(a), 0.0f, e.radio * 1.2f * cosf(2.0f * a)) * signo;
            orientar = true;
            break; }
        case MOV_DERIVA:
            p += glm::vec3(sinf(t * 0.21f + e.fase) * 0.6f, sinf(t * 0.37f + e.fase) * 0.5f, cosf(t * 0.17f + e.fase) * 0.6f);
            p.y += e.baseY;
            break;
        }
        glm::mat4 M = glm::translate(glm::mat4(1.0f), p);
        if (orientar) M = glm::rotate(M, atan2f(-dir.z, dir.x), glm::vec3(0.0f, 1.0f, 0.0f));
        else M = glm::rotate(M, glm::radians(yawUsar), glm::vec3(0.0f, 1.0f, 0.0f));
        if (e.mov == MOV_DERIVA) M = glm::rotate(M, t * 0.2f + e.fase, glm::normalize(glm::vec3(0.3f, 1.0f, 0.5f)));
        if (rotUsar.x != 0.0f) M = glm::rotate(M, glm::radians(rotUsar.x), glm::vec3(1, 0, 0));
        if (rotUsar.y != 0.0f) M = glm::rotate(M, glm::radians(rotUsar.y), glm::vec3(0, 1, 0));
        if (rotUsar.z != 0.0f) M = glm::rotate(M, glm::radians(rotUsar.z), glm::vec3(0, 0, 1));
        float s = escUsar * (k > 0 ? 0.85f + 0.3f * Aleatorio(idx, k + 50) : 1.0f) * (1.0f + 0.8f * e.inflado);
        glm::vec3 sv(s);
        if (e.pulsar) {   // la campana se contrae (mas angosta y alta) y se relaja
            float pu = sinf(t * 2.2f + e.fase + k * 1.3f);
            sv = glm::vec3(s * (1.0f - 0.10f * pu), s * (1.0f + 0.12f * pu), s * (1.0f - 0.10f * pu));
        }
        return glm::scale(M, sv);
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
            e.modelo->Draw(*sh);
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
        payasos.miembros = 6; payasos.dispersion = 0.5f; payasos.velNado = 12.0f; payasos.sobreSuelo = true; payasos.oscAmp = 0.2f;
        elementos.push_back(payasos);

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
        Elemento almeja = Fijo("almejagigante", c, glm::vec3(7.5f, 0.2f, -6.0f), 30, 1.0f);
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
        Elemento globo = Nadador("pez_globo", c, MOV_ORBITA, glm::vec3(-1.0f, 1.8f, 8.0f), 2.5f, 0.25f, "Pez globo");
        globo.inflable = true; globo.sobreSuelo = true; globo.ondula = 0.03f; globo.oscAmp = 0.3f; elementos.push_back(globo);
        globo.pos = glm::vec3(12.0f, 2.2f, 4.0f); globo.vel = -0.2f; globo.fase = 2.0f; elementos.push_back(globo);
        globo.pos = glm::vec3(1.0f, 2.5f, -1.0f); globo.radio = 4.0f; globo.vel = 0.15f; globo.fase = 4.0f; elementos.push_back(globo);
        Elemento tortuga = Nadador("tortuga", c, MOV_ORBITA, glm::vec3(0.0f, 4.5f, 0.0f), 11.0f, 0.09f, "Tortuga marina");
        tortuga.ondula = 0.0f; tortuga.oscAmp = 1.0f; tortuga.oscVel = 0.3f; elementos.push_back(tortuga);
        tortuga.pos = glm::vec3(-12.0f, 3.5f, 6.0f); tortuga.radio = 7.0f; tortuga.vel = -0.11f; tortuga.fase = 2.5f; tortuga.escala = 0.85f; elementos.push_back(tortuga);
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
        // plataformas de hielo flotando en la superficie
        for (int i = 0; i < 7; i++) {
            Elemento h = Fijo(i % 2 ? "hielo2" : "hielo1", c, glm::vec3(0.0f), 0, 1);
            float a = Aleatorio(70, i) * 6.28f, r = 6.0f + 24.0f * Aleatorio(71, i);
            h.pos = glm::vec3(cosf(a) * r, cfg.superficieY + 0.05f, sinf(a) * r);
            h.sobreSuelo = false; h.yaw = Aleatorio(72, i) * 360.0f; h.escala = 1.0f + 0.7f * Aleatorio(73, i);
            if (i == 0) h.pos = glm::vec3(5.0f, cfg.superficieY - 0.9f, 4.0f);
            h.oscAmp = 0.08f; h.oscVel = 0.5f; h.fase = (float)i;
            h.interactivo = true; h.etiqueta = "Plataforma de hielo";
            h.tinte = glm::vec3(0.85f, 0.95f, 1.05f); h.emision = 0.55f;   // luz que atraviesa el hielo
            MatTraslucido(h);
            elementos.push_back(h);
        }
        Elemento ice = Fijo("iceberg", c, glm::vec3(-30.0f, cfg.superficieY, -22.0f), 30, 1.6f); ice.sobreSuelo = false; ice.etiqueta = "Iceberg"; ice.interactivo = true;
        ice.tinte = glm::vec3(0.85f, 0.95f, 1.05f); ice.emision = 0.45f;
        MatTraslucido(ice);
        elementos.push_back(ice);
        ice.pos = glm::vec3(34.0f, cfg.superficieY, 10.0f); ice.yaw = 140; ice.escala = 1.3f; elementos.push_back(ice);

        Elemento ballena = Nadador("ballena", c, MOV_KF, glm::vec3(0.0f, 8.0f, -8.0f), 30.0f, 0.035f, "Ballena azul");
        ballena.kf = &KF_Ballena();   // KF #1: salto y reingreso
        ballena.ondula = 0.25f; ballena.velNado = 1.5f; ballena.oscAmp = 1.0f; ballena.oscVel = 0.15f;
        elementos.push_back(ballena);
        Elemento belugas = Nadador("beluga", c, MOV_CARDUMEN, glm::vec3(0.0f, 7.0f, 4.0f), 15.0f, -0.12f, "Beluga");
        belugas.miembros = 3; belugas.dispersion = 2.0f; belugas.ondula = 0.12f; belugas.velNado = 3.0f;
        elementos.push_back(belugas);
        Elemento focas = Nadador("foca", c, MOV_CARDUMEN, glm::vec3(4.0f, 9.0f, -4.0f), 9.0f, 0.35f, "Foca");
        focas.miembros = 3; focas.dispersion = 1.8f; focas.ondula = 0.1f; focas.velNado = 6.0f; focas.oscAmp = 1.2f;
        elementos.push_back(focas);
        Elemento pinguinos = Nadador("pinguino", c, MOV_CARDUMEN, glm::vec3(-6.0f, 12.5f, 2.0f), 7.0f, 0.7f, "Pinguino");
        pinguinos.miembros = 6; pinguinos.dispersion = 1.2f; pinguinos.rotExtra = glm::vec3(0.0f, 0.0f, -90.0f);
        pinguinos.ondula = 0.0f; pinguinos.oscAmp = 0.8f; pinguinos.oscVel = 1.5f;
        elementos.push_back(pinguinos);
        {
            Elemento oso = Fijo("osito", c, glm::vec3(5.0f, cfg.superficieY - 0.9f + 0.79f, 4.0f), 20.0f, 1.0f);
            oso.sobreSuelo = false; oso.interactivo = true; oso.etiqueta = "Oso polar";
            oso.oscAmp = 0.08f; oso.oscVel = 0.5f; oso.fase = 0.0f;
            elementos.push_back(oso);
        }
        Elemento morsa = Fijo("morsa", c, glm::vec3(-8.0f, 0.5f, -6.0f), 60, 1.0f); morsa.interactivo = true; morsa.etiqueta = "Morsa";
        elementos.push_back(morsa);
        morsa.pos = glm::vec3(-5.0f, 0.5f, -9.0f); morsa.yaw = 150; morsa.escala = 0.9f; elementos.push_back(morsa);
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
