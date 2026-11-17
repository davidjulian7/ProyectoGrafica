#ifndef ESTADO_HUD_H
#define ESTADO_HUD_H
// ============================================================================================
// INTERFAZ · Estado de la pantalla (que tanta informacion se muestra)
//   detallado    tecla H: muestra todos los paneles (termometro grande, objetivos, ayuda...)
//   fichaAbierta tecla I: muestra la ficha completa del animal u objeto que senala la mira
//   intro        tarjeta de indicaciones al entrar a una zona (se cierra con clic o Enter)
// [CAMBIAR] si quieres que el juego empiece con todos los paneles visibles: detallado = true
// ============================================================================================
struct EstadoHUD
{
    bool detallado = false;
    bool fichaAbierta = false;
    bool intro = false;
    float introTiempo = 0.0f;
};
inline EstadoHUD& HUD() { static EstadoHUD h; return h; }
#endif
