// ============================================================================================
// ESCENA · Datos de cada objeto (struct Elemento) y de cada zona (struct ZonaConfig).
// Cada campo de Elemento es algo que puedes cambiarle a un objeto donde se crea, por ejemplo:
//   e.escala = 2.0f;  e.yaw = 90;  e.tinte = glm::vec3(1, 0.5, 0.5);  e.alpha = 0.6f;  e.emision = 0.3f;
//   e.vel = 0.3f (rapidez de nado);  e.interactivo = true;  e.etiqueta = "Nombre";
// ============================================================================================
#ifndef ESTRUCTURAS_H
#define ESTRUCTURAS_H
// Datos de la escena: tipos de movimiento, propiedades de cada objeto (Elemento) y configuracion de una zona.
#include <string>
#include <glm/glm.hpp>
#include <animacion/keyframes.h>

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
    // propios de cada zona (se asignan en zonas/<zona>/<zona>_escena.h)
    float nadoAceleracion = 3.5f;      // rapidez con la que acelera el buzo
    float nadoResistencia = 2.5f;      // resistencia del agua (mas alto = se detiene antes)
    float campoVision = 45.0f;         // grados de la camara (zoom)
    float velFauna = 0.5f;             // velocidad de los animales (1 = normal)
    // materiales: (intensidad del brillo, concentracion del brillo)
    glm::vec2 matOpaco = glm::vec2(0.08f, 24.0f), matPlastico = glm::vec2(0.60f, 32.0f);
    glm::vec2 matTraslucido = glm::vec2(0.30f, 64.0f), matMetalico = glm::vec2(1.00f, 96.0f);
};

struct Burbuja { glm::vec3 p; float vel; float r; float fase; };
struct MonedaVoladora { glm::vec3 p, v, eje; float ang, vAng, brillo; bool quieta; };   // lluvia de monedas del cofre

#endif
