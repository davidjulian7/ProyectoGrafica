#ifndef ESTADO_HUD_H
#define ESTADO_HUD_H
// ============================================================================================
// INTERFAZ · Estado de la pantalla (que tanta informacion se muestra)
//   detallado    tecla H: muestra todos los paneles (termometro grande, objetivos, ayuda...)
//   fichaAbierta tecla I: muestra la ficha completa del animal u objeto que senala la mira
//   intro        tarjeta de indicaciones al entrar a una zona (se cierra con clic o Enter)
//   nivel        nivel de uso elegido en el menu (flechas arriba/abajo) o con la tecla U en una zona
//   quieto       segundos que el buzo lleva sin moverse (lo usa el nivel avanzado)
// [CAMBIAR] si quieres que el juego empiece con todos los paneles visibles: detallado = true
// ============================================================================================

// --------------------------------------------------------------------------------------------
// NIVELES DE USO (documento "Usuarios e interaccion": usuario novato, medio y experto)
//   BASICO      tarjeta de indicaciones al inicio de cada zona (que conseguir y como)
//               + pista amarilla en TODOS los objetos interactivos + teclas en el borde
//   INTERMEDIO  sin tarjeta: solo las teclas en el borde de la pantalla
//               + pista amarilla solo en ALGUNOS modelos (los interactivos cercanos)
//   AVANZADO    sin tarjeta ni pistas: solo el avance de la zona; las teclas aparecen en el
//               borde unicamente si el buzo se queda quieto un rato
// Donde se aplica cada cosa:
//   tarjeta inicial  -> ProyectoFinal.cpp, EntrarZona()
//   pista amarilla   -> escena/render.h, DibujarElemento() + uPista en bin/shaders/<zona>/objetos.fs
//   teclas en borde  -> interfaz/hud_comun.h, HUDTeclasBorde()
//   solo avance      -> interfaz/hud_comun.h, HUDObjetivo()
//   selector         -> interfaz/menu.h (Draw, OnKey, OnClick)
// --------------------------------------------------------------------------------------------
enum NivelUso { NIVEL_BASICO = 0, NIVEL_INTERMEDIO = 1, NIVEL_AVANZADO = 2 };
const int NUM_NIVELES = 3;

// [CAMBIAR] parametros de los niveles
const float PISTA_DISTANCIA_INTERMEDIO = 12.0f;   // intermedio: brillan los interactivos a menos de 12 m del buzo
const float AYUDA_QUIETO_AVANZADO = 8.0f;         // avanzado: segundos quieto para que aparezcan las teclas

// DIFICULTAD POR TIEMPO: segundos para completar cada zona segun el nivel (basico 6:00, intermedio 4:00, avanzado 2:30)
//   el reloj se ve arriba a la derecha (HUDCronometro) y al acabarse sale "Se acabó el tiempo" (HUDTiempoAgotado)
//   la cuenta esta en ProyectoFinal.cpp → Update() (tiempoUsado)
// [CAMBIAR] tiempos de cada nivel en segundos
const float TIEMPO_NIVEL[NUM_NIVELES] = { 360.0f, 240.0f, 150.0f };

struct EstadoHUD
{
    bool detallado = false;
    bool fichaAbierta = false;
    bool intro = false;
    float introTiempo = 0.0f;
    int nivel = NIVEL_BASICO;     // [CAMBIAR] nivel con el que arranca el juego
    float quieto = 0.0f;
};
inline EstadoHUD& HUD() { static EstadoHUD h; return h; }
inline float TiempoLimite() { return TIEMPO_NIVEL[HUD().nivel]; }

inline const char* NombreNivel(int n)
{
    return n == NIVEL_AVANZADO ? "AVANZADO" : (n == NIVEL_INTERMEDIO ? "INTERMEDIO" : "BÁSICO");
}
inline const char* DescripcionNivel(int n)
{
    return n == NIVEL_AVANZADO   ? "Sin guías: solo tu avance. Si te quedas quieto aparecen las teclas."
         : n == NIVEL_INTERMEDIO ? "Teclas en el borde de la pantalla y pistas en los objetos cercanos."
                                 : "Indicaciones al entrar a cada zona y pistas amarillas en todo lo interactivo.";
}

// Teclas en el borde: 0 = ocultas, 1 = visibles (en avanzado aparecen poco a poco al quedarse quieto)
inline float VisibilidadTeclas()
{
    if (HUD().nivel != NIVEL_AVANZADO) return 1.0f;
    float v = (HUD().quieto - AYUDA_QUIETO_AVANZADO) / 0.6f;
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}
#endif
