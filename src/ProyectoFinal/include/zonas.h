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
#include <cmath>
#include <iostream>
#include <fstream>

#include <shader_m.h>
#include <model.h>
#include <camera.h>
#include <keyframes.h>
#include <animatedmodel.h>   // tras shader_m.h (guard SHADER_H compartido)

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
    // calculado al cargar
    Model* modelo = nullptr;
    unsigned int id = 0;
    float medioLargo = 0.5f;
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

class Escenario
{
public:
    ZonaConfig cfg;
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
        // Demo de skinning (rubrica A2b): solo se usa en Zona 1
        shSkin = new Shader("shaders/10_vertex_skinning-IT.vs", "shaders/10_fragment_skinning-IT.fs");
        shSkin->setBonesIDs(MAX_RIGGING_BONES);
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
        elementos.clear(); burbujas.clear(); seleccion = -1;
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
            e.id = 1000 + (unsigned int)i;
            e.baseY = e.sobreSuelo ? AlturaSuelo(e.pos.x, e.pos.z) : 0.0f;
            if (e.basura) basuraTotal++;
        }
        burbujaModelo = Cargar("burbuja", "comun");
        for (auto& f : fuentes) f.y += AlturaSuelo(f.x, f.z);   // las fuentes se miden desde el fondo
        linterna = cfg.linterna;

        // Demo de skinning: solo en Zona 1 (se carga una vez)
        if (z == 1) {
            if (!demoSkin) {
                demoSkin = new AnimatedModel("models/IllumModels/KAYA.fbx");
                if (demoSkin->meshes.empty()) {
                    delete demoSkin; demoSkin = nullptr;
                    std::cout << "[Zonas] No se pudo cargar KAYA.fbx (demo skinning)" << std::endl;
                }
            }
            if (demoSkin) {
                demoPos = glm::vec3(-4.0f, AlturaSuelo(-4.0f, 12.0f), 12.0f);
                demoYaw = 140.0f;
            }
        }

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
    void Actualizar(float dt, float t, const Camera& cam)
    {
        if (dt > 0.1f) dt = 0.1f;
        // respiracion del buzo: cada 4.5 s sale una nube de burbujas frente a la mascara
        respiracion += dt;
        if (respiracion > 4.5f) {
            respiracion = 0.0f;
            // salen por debajo de la mascara y suben frente a la vista
            glm::vec3 boca = cam.Position + cam.Front * 0.45f - cam.Up * 0.6f;
            for (int k = 0; k < 10; k++) burbujas.push_back({ boca + Aleatorio3(k + (int)(t * 10)) * 0.12f, 0.9f + Aleatorio(k, 3) * 0.6f, 0.012f + Aleatorio(k, 5) * 0.02f, Aleatorio(k, 9) * 6.0f });
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
        // Demo skinning (solo Zona 1)
        if (demoSkin && zona == 1) demoSkin->UpdateAnimation(dt);
    }

    // Pase de picking: dibuja los elementos interactivos con su color de ID y lee el centro.
    void Picking(const glm::mat4& proj, const glm::mat4& view, float t, int fbW, int fbH)
    {
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_BLEND);
        shPicking->use();
        shPicking->setMat4("projection", proj);
        shPicking->setMat4("view", view);
        // el terreno tapa lo que esta detras (ID 0xFFFFFF = nada)
        shPicking->setVec4("PickingColor", glm::vec4(1.0f));
        shPicking->setMat4("model", glm::mat4(1.0f));
        terreno->Draw(*shPicking);
        for (size_t i = 0; i < elementos.size(); i++) {
            Elemento& e = elementos[i];
            if (!e.visible) continue;
            unsigned int id = e.interactivo ? e.id : 0xFFFFFF;
            shPicking->setVec4("PickingColor", glm::vec4((id & 0xFF) / 255.0f, ((id >> 8) & 0xFF) / 255.0f, ((id >> 16) & 0xFF) / 255.0f, 1.0f));
            for (int k = 0; k < e.miembros; k++) {
                shPicking->setMat4("model", Matriz(e, (int)i, k, t));
                e.modelo->Draw(*shPicking);
            }
        }
        unsigned char px[4];
        glReadPixels(fbW / 2, fbH / 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px);
        unsigned int id = px[0] + px[1] * 256 + px[2] * 65536;
        int nuevo = (id >= 1000 && id < 1000 + elementos.size()) ? (int)(id - 1000) : -1;
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

        // 2b) Demo de skinning (Zona 1): esqueleto + shader de huesos
        if (demoSkin && zona == 1 && shSkin) {
            shSkin->use();
            shSkin->setMat4("projection", proj);
            shSkin->setMat4("view", view);
            glm::mat4 mDemo = glm::translate(glm::mat4(1.0f), demoPos);
            mDemo = glm::rotate(mDemo, glm::radians(demoYaw), glm::vec3(0.0f, 1.0f, 0.0f));
            mDemo = glm::scale(mDemo, glm::vec3(1.1f));
            shSkin->setMat4("model", mDemo);
            shSkin->setMat4("gBones", MAX_RIGGING_BONES, demoSkin->gBones);
            demoSkin->Draw(*shSkin);
            // restaurar shader opaco del resto del pase
            sh->use();
            ConfigurarShader(proj, view, cam, t, camBuzo);
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
    Shader* shSkin = nullptr;
    AnimatedModel* demoSkin = nullptr;   // demo A2b (KAYA), solo Zona 1
    glm::vec3 demoPos = glm::vec3(0.0f);
    float demoYaw = 0.0f;
    GLuint vaoVacio = 0, vaoQuad = 0, vboQuad = 0, cubo = 0;
    std::map<std::string, Model*> cache;
    std::vector<Elemento> elementos;
    std::vector<Burbuja> burbujas;
    std::vector<glm::vec3> fuentes;
    std::string terrenoArchivo, terrenoCarpeta;
    Model* terreno = nullptr;
    Model* burbujaModelo = nullptr;
    std::vector<float> alturas;
    static const int N = 111;
    float respiracion = 3.0f, fuenteTiempo = 0.0f;

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
            break;
        case MOV_KF:
            if (e.kf) e.kf->PoseAt(t + e.fase, p, yawUsar, rotUsar, escUsar);
            break;
        case MOV_ORBITA:
        case MOV_CARDUMEN: {
            float a = t * e.vel + e.fase - (k > 0 ? (0.15f + 0.25f * Aleatorio(idx, k)) * signo * (float)(k % 7) * 0.3f : 0.0f);
            float r = e.radio + off.x;
            p = e.pos + glm::vec3(cosf(a) * r, off.y + sinf(t * e.oscVel + e.fase + k) * e.oscAmp, sinf(a) * r);
            p.y += e.baseY;
            dir = glm::vec3(-sinf(a), 0.0f, cosf(a)) * signo;
            orientar = true;
            break; }
        case MOV_LIBRE: {
            float a = t * e.vel + e.fase;
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
        float s = escUsar * (k > 0 ? 0.85f + 0.3f * Aleatorio(idx, k + 50) : 1.0f);
        return glm::scale(M, glm::vec3(s));
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
        sh->setFloat("uFantasma", 0.0f);
        sh->setFloat("uSpecPower", 40.0f);
        sh->setFloat("uSpecInt", 0.25f);
        sh->setFloat("uMetalico", 0.0f);
    }

    void DibujarElemento(const Elemento& e, int idx, float t)
    {
        Comunes(e.tinte, e.ondula, e.medioLargo, e.mecer, e.fase, e.alpha, e.emision);
        sh->setFloat("uSpecPower", e.specPower);
        sh->setFloat("uSpecInt", e.specInt);
        sh->setFloat("uMetalico", e.metalico);
        sh->setFloat("uVelNado", e.velNado);
        sh->setFloat("uResaltado", idx == seleccion ? 0.25f + 0.15f * sinf(t * 6.0f) : 0.0f);
        for (int k = 0; k < e.miembros; k++) {
            sh->setFloat("uFase", e.fase + k * 1.3f);
            sh->setMat4("model", Matriz(e, idx, k, t));
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
        cfg.niebla = glm::vec3(0.08f, 0.40f, 0.50f);
        cfg.densidad = 0.020f;
        cfg.solColor = glm::vec3(1.0f, 0.96f, 0.86f) * 1.25f;
        cfg.solDir = glm::vec3(-0.3f, -1.0f, -0.25f);
        cfg.ambArriba = glm::vec3(0.36f, 0.56f, 0.62f);
        cfg.ambAbajo = glm::vec3(0.12f, 0.22f, 0.26f);
        cfg.causticas = 0.9f;
        cfg.superficieY = 18.0f;
        cfg.inicioCamara = glm::vec3(0.0f, 6.0f, 22.0f);
        terrenoArchivo = "terreno_arrecifes"; terrenoCarpeta = "arrecifes";
        const char* c = "arrecifes";

        std::vector<glm::vec3> vivos = { {1.0f,0.45f,0.60f}, {0.72f,0.42f,0.95f}, {1.0f,0.58f,0.28f}, {1.0f,0.85f,0.35f}, {0.30f,0.85f,0.80f}, {0.95f,0.35f,0.30f} };
        Elemento roca = Fijo("roca_arrecife", c, glm::vec3(0.0f, -0.4f, 0.0f), 0, 1); roca.etiqueta = "Roca";
        Esparcir(roca, 12, glm::vec2(0.0f), 4.0f, 32.0f, 0.8f, 2.6f, 11);
        // tres parches de arrecife
        glm::vec2 parches[] = { {-6.0f, -4.0f}, {7.0f, -10.0f}, {-2.0f, 9.0f} };
        for (int p = 0; p < 3; p++) {
            Elemento r1 = Fijo("coral_rama1", c, glm::vec3(0.0f, -0.1f, 0.0f), 0, 1); r1.interactivo = true; r1.etiqueta = "Coral";
            Elemento r2 = r1; r2.archivo = "coral_rama2";
            Elemento ce = Fijo("coral_cerebro", c, glm::vec3(0.0f, -0.15f, 0.0f), 0, 1); ce.interactivo = true; ce.etiqueta = "Coral";
            Elemento ab = Fijo("coral_abanico", c, glm::vec3(0.0f, -0.1f, 0.0f), 0, 1); ab.interactivo = true; ab.etiqueta = "Coral"; ab.mecer = 0.15f;
            Esparcir(r1, 8, parches[p], 0.0f, 5.0f, 1.0f, 2.4f, 20 + p * 10, vivos);
            Esparcir(r2, 7, parches[p], 0.0f, 5.5f, 1.0f, 2.2f, 21 + p * 10, vivos);
            Esparcir(ce, 5, parches[p], 0.5f, 5.0f, 0.8f, 1.8f, 22 + p * 10, vivos);
            Esparcir(ab, 3, parches[p], 1.0f, 5.0f, 1.2f, 2.0f, 23 + p * 10, { {0.85f,0.30f,0.75f}, {1.0f,0.55f,0.30f} });
        }
        Elemento alga = Fijo("alga", c, glm::vec3(0.0f, -0.1f, 0.0f), 0, 1); alga.mecer = 1.0f; alga.etiqueta = "Alga";
        Esparcir(alga, 26, glm::vec2(-14.0f, 8.0f), 0.0f, 7.0f, 0.7f, 1.6f, 50, { {0.9f,1.0f,0.8f}, {0.7f,0.9f,0.6f}, {1.0f,0.9f,0.7f} });
        Esparcir(alga, 18, glm::vec2(16.0f, 6.0f), 0.0f, 6.0f, 0.7f, 1.4f, 51);

        Elemento almeja = Fijo("almejagigante", c, glm::vec3(3.0f, 0.2f, -2.0f), 30, 1.0f); almeja.interactivo = true; almeja.etiqueta = "Almeja gigante";
        almeja.mov = MOV_KF; almeja.kf = &KF_Almeja();   // KF #4: abre/cierra la concha
        elementos.push_back(almeja);
        almeja.pos = glm::vec3(-9.0f, 0.2f, 3.0f); almeja.yaw = 200; almeja.escala = 0.8f; elementos.push_back(almeja);
        Elemento cangrejo = Fijo("cangrejoermitano", c, glm::vec3(1.5f, 0.15f, 4.0f), 120, 1.0f); cangrejo.interactivo = true; cangrejo.etiqueta = "Cangrejo ermitano";
        elementos.push_back(cangrejo);
        cangrejo.pos = glm::vec3(-3.0f, 0.15f, 12.0f); cangrejo.yaw = 40; elementos.push_back(cangrejo);
        Elemento caballito = Fijo("CaballitoMar", c, glm::vec3(-13.0f, 2.0f, 8.0f), 70, 1.0f); caballito.interactivo = true; caballito.etiqueta = "Caballito de mar";
        caballito.oscAmp = 0.25f; caballito.oscVel = 0.8f; caballito.sobreSuelo = true;
        elementos.push_back(caballito);
        caballito.pos = glm::vec3(-15.5f, 1.6f, 6.0f); caballito.yaw = 250; caballito.fase = 2.0f; elementos.push_back(caballito);

        Elemento payasos = Nadador("PezPayaso", c, MOV_CARDUMEN, glm::vec3(-6.0f, 2.2f, -4.0f), 3.0f, 0.5f, "Pez payaso");
        payasos.miembros = 7; payasos.dispersion = 0.8f; payasos.velNado = 12.0f; payasos.sobreSuelo = true;
        elementos.push_back(payasos);
        Elemento cirujanos = Nadador("Pezcirujano", c, MOV_CARDUMEN, glm::vec3(7.0f, 3.0f, -10.0f), 4.5f, -0.4f, "Pez cirujano azul");
        cirujanos.miembros = 6; cirujanos.dispersion = 1.0f; cirujanos.velNado = 11.0f; cirujanos.sobreSuelo = true;
        elementos.push_back(cirujanos);
        Elemento dragon = Nadador("pezdracoide", c, MOV_ORBITA, glm::vec3(-2.0f, 2.0f, 9.0f), 3.0f, 0.35f, "Pez dragon");
        dragon.sobreSuelo = true; elementos.push_back(dragon);
        Elemento atunes = Nadador("pescado", c, MOV_CARDUMEN, glm::vec3(0.0f, 9.0f, 0.0f), 24.0f, 0.18f, "Atun");
        atunes.miembros = 14; atunes.dispersion = 2.5f; atunes.velNado = 9.0f; atunes.ondula = 0.08f;
        elementos.push_back(atunes);
        Elemento tortuga = Nadador("tortuga", c, MOV_ORBITA, glm::vec3(0.0f, 6.0f, -2.0f), 13.0f, 0.10f, "Tortuga marina");
        tortuga.ondula = 0.0f; tortuga.oscAmp = 1.0f; tortuga.oscVel = 0.3f; elementos.push_back(tortuga);
        Elemento manta = Nadador("mantaraya", c, MOV_LIBRE, glm::vec3(0.0f, 10.0f, -6.0f), 22.0f, 0.06f, "Mantarraya");
        manta.ondula = 0.0f; manta.oscAmp = 1.5f; manta.oscVel = 0.25f; elementos.push_back(manta);
        Elemento manati = Nadador("manati", c, MOV_ORBITA, glm::vec3(4.0f, 2.8f, 4.0f), 20.0f, -0.05f, "Manati");
        manati.ondula = 0.05f; manati.velNado = 2.0f; manati.sobreSuelo = true; elementos.push_back(manati);

        fuentes = { glm::vec3(-6.0f, 0.4f, -4.0f), glm::vec3(7.0f, 0.4f, -10.0f) };
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
        Elemento oso = Nadador("osito", c, MOV_ORBITA, glm::vec3(8.0f, cfg.superficieY - 0.55f, 8.0f), 10.0f, 0.05f, "Oso polar");
        oso.ondula = 0.0f; oso.oscAmp = 0.05f; elementos.push_back(oso);
        Elemento morsa = Fijo("morsa", c, glm::vec3(-8.0f, 0.5f, -6.0f), 60, 1.0f); morsa.interactivo = true; morsa.etiqueta = "Morsa";
        elementos.push_back(morsa);
        morsa.pos = glm::vec3(-5.0f, 0.5f, -9.0f); morsa.yaw = 150; morsa.escala = 0.9f; elementos.push_back(morsa);
        Elemento krill = Nadador("Krill", c, MOV_CARDUMEN, glm::vec3(-4.0f, 6.0f, -6.0f), 5.0f, 0.25f, "Krill");
        krill.miembros = 90; krill.dispersion = 2.5f; krill.escala = 1.4f; krill.ondula = 0.0f; krill.oscAmp = 0.6f;
        elementos.push_back(krill);
        Elemento bacalaos = Nadador("bacalao", c, MOV_CARDUMEN, glm::vec3(6.0f, 3.5f, 6.0f), 8.0f, -0.3f, "Bacalao");
        bacalaos.miembros = 9; bacalaos.dispersion = 1.6f; bacalaos.sobreSuelo = true; elementos.push_back(bacalaos);
        fuentes.clear();
    }

    // ------------------------------------------------------------ Zona 3
    void ConstruirNaufragios()
    {
        cfg = ZonaConfig();
        cfg.nombre = "Naufragios";
        cfg.niebla = glm::vec3(0.025f, 0.075f, 0.075f);
        cfg.densidad = 0.040f;
        cfg.solColor = glm::vec3(0.40f, 0.55f, 0.50f) * 0.6f;
        cfg.solDir = glm::vec3(0.1f, -1.0f, 0.1f);
        cfg.ambArriba = glm::vec3(0.13f, 0.21f, 0.20f);
        cfg.ambAbajo = glm::vec3(0.02f, 0.04f, 0.05f);
        cfg.causticas = 0.06f;
        cfg.superficieY = 22.0f;
        cfg.linterna = true;
        cfg.inicioCamara = glm::vec3(0.0f, 5.0f, 18.0f);
        terrenoArchivo = "terreno_naufragios"; terrenoCarpeta = "naufragios";
        const char* c = "naufragios";

        Elemento sub = Fijo("submarino", c, glm::vec3(5.0f, 0.6f, -4.0f), 25, 1.0f);
        sub.interactivo = true; sub.etiqueta = "Submarino hundido";
        sub.mov = MOV_KF; sub.kf = &KF_Submarino();   // KF #2: se asienta con escora
        MatMetalico(sub);
        elementos.push_back(sub);
        // ruinas
        float colX[] = { -14.0f, -10.0f, -14.0f, -10.0f };
        float colZ[] = { -3.0f, -3.0f, 3.0f, 3.0f };
        for (int i = 0; i < 4; i++) {
            Elemento col = Fijo(i == 2 ? "columna_rota" : "columna", c, glm::vec3(colX[i], -0.3f, colZ[i]), (float)(i * 37), 1.0f);
            col.interactivo = true; col.etiqueta = "Ruinas"; elementos.push_back(col);
        }
        Elemento caida = Fijo("columna", c, glm::vec3(-6.0f, 0.3f, 6.0f), 75, 0.9f);
        caida.interactivo = true; caida.etiqueta = "Ruina caida";
        caida.mov = MOV_KF; caida.kf = &KF_Columna();   // KF #3: cae, rebota y se posa
        elementos.push_back(caida);
        Elemento bloque = Fijo("bloque", c, glm::vec3(0.0f, 0.2f, 0.0f), 0, 1); bloque.etiqueta = "Ruinas"; bloque.interactivo = true;
        Esparcir(bloque, 9, glm::vec2(-12.0f, 0.0f), 2.0f, 8.0f, 0.7f, 1.3f, 13);
        Elemento roca = Fijo("roca_naufragio", c, glm::vec3(0.0f, -0.4f, 0.0f), 0, 1); roca.etiqueta = "Roca";
        Esparcir(roca, 12, glm::vec2(0.0f), 6.0f, 30.0f, 0.8f, 2.4f, 14);

        // basura: en el fondo y a la deriva (se recoge con clic)
        auto basura = [&](const char* archivo, const char* etiqueta, int n, int semilla, bool deriva, float altura, glm::vec2 centro, float radio) {
            for (int i = 0; i < n; i++) {
                Elemento b = Fijo(archivo, c, glm::vec3(0.0f), 0, 1.0f);
                float a = Aleatorio(semilla, i) * 6.28f, r = radio * sqrtf(Aleatorio(semilla + 1, i));
                b.pos = glm::vec3(centro.x + cosf(a) * r, altura, centro.y + sinf(a) * r);
                b.yaw = Aleatorio(semilla + 2, i) * 360.0f;
                b.rotExtra = deriva ? glm::vec3(0.0f) : glm::vec3((Aleatorio(semilla + 3, i) - 0.5f) * 40.0f, 0.0f, (Aleatorio(semilla + 4, i) > 0.5f ? 90.0f : 0.0f));
                b.mov = deriva ? MOV_DERIVA : MOV_FIJO; b.fase = (float)i * 1.7f;
                b.interactivo = true; b.basura = true; b.etiqueta = etiqueta;
                if (std::string(archivo) == "bolsaplastico") MatPlastico(b);
                else MatMetalico(b);   // latas y llantas
                elementos.push_back(b);
            }
        };
        basura("lata", "Lata de aluminio", 7, 80, false, 0.08f, glm::vec2(2.0f, 2.0f), 14.0f);
        basura("lataaplastada", "Lata aplastada", 5, 81, false, 0.06f, glm::vec2(-4.0f, 0.0f), 12.0f);
        basura("Llanta", "Llanta", 3, 82, false, 0.15f, glm::vec2(4.0f, 6.0f), 10.0f);
        basura("bolsaplastico", "Bolsa de plastico", 7, 83, true, 3.0f, glm::vec2(0.0f, 0.0f), 16.0f);
        // KF #5: una bolsa recorriendo 5 puntos de flotación (además de las de deriva)
        Elemento bolsaKF = Fijo("bolsaplastico", c, glm::vec3(-2.0f, 2.5f, 4.0f), 0, 1.0f);
        bolsaKF.mov = MOV_KF; bolsaKF.kf = &KF_Bolsa();
        bolsaKF.interactivo = true; bolsaKF.basura = true; bolsaKF.etiqueta = "Bolsa de plastico";
        MatPlastico(bolsaKF);
        elementos.push_back(bolsaKF);

        Elemento leones = Nadador("pezleon", c, MOV_CARDUMEN, glm::vec3(-12.0f, 2.5f, 0.0f), 3.0f, 0.2f, "Pez leon (invasor)");
        leones.miembros = 4; leones.dispersion = 1.5f; leones.sobreSuelo = true; leones.velNado = 4.0f; elementos.push_back(leones);
        Elemento barracudas = Nadador("barracuda", c, MOV_CARDUMEN, glm::vec3(2.0f, 7.0f, -2.0f), 11.0f, -0.25f, "Barracuda");
        barracudas.miembros = 5; barracudas.dispersion = 1.5f; elementos.push_back(barracudas);
        Elemento tiburon = Nadador("Tiburon", c, MOV_ORBITA, glm::vec3(0.0f, 9.0f, 0.0f), 17.0f, 0.12f, "Tiburon");
        tiburon.ondula = 0.15f; tiburon.velNado = 3.0f; elementos.push_back(tiburon);
        Elemento vaquita = Nadador("vacamarina", c, MOV_ORBITA, glm::vec3(4.0f, 3.0f, 6.0f), 5.0f, 0.18f, "Vaquita marina");
        vaquita.ondula = 0.08f; vaquita.velNado = 4.0f; vaquita.sobreSuelo = true; elementos.push_back(vaquita);

        fuentes = { glm::vec3(3.0f, 1.8f, -3.0f), glm::vec3(8.0f, 2.5f, -5.5f) };
    }

};

#endif
