// ============================================================================================
// INTERFAZ · Paneles comunes: lista de especies (tecla L) y anuncio de "Zona completada".
// ============================================================================================
#ifndef HUD_COMUN_H
#define HUD_COMUN_H
/*
* Paneles del HUD que comparten las zonas:
*  - Lista de especies de la bitacora (tecla L): cuales ya registraste y cuales faltan.
*  - Anuncio de "Zona completada" con el resumen de lo que hizo el usuario.
* Se dibujan entre ui.Comenzar() y ui.Terminar().
*/
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <cstdio>
#include <cmath>
#include <interfaz/menu.h>
#include <interfaz/fichas.h>
#include <interfaz/estado_hud.h>

// Ordena las etiquetas por el nombre que se muestra al usuario
inline std::vector<std::string> OrdenarEspecies(const std::set<std::string>& etiquetas)
{
    std::vector<std::string> v(etiquetas.begin(), etiquetas.end());
    std::sort(v.begin(), v.end(), [](const std::string& a, const std::string& b) {
        const Ficha* fa = BuscarFicha(a); const Ficha* fb = BuscarFicha(b);
        return (fa ? fa->nombre : a) < (fb ? fb->nombre : b);
    });
    return v;
}

inline void HUDListaEspecies(MenuInicio& ui, const std::vector<std::string>& especies, const std::set<std::string>& registradas)
{
    float s = ui.Escala(), W = ui.Ancho(), H = ui.Alto();
    int n = (int)especies.size(); if (n == 0) return;
    int cols = n > 10 ? 2 : 1, filas = (n + cols - 1) / cols;
    float colW = 240.0f * s, lh = 22.0f * s, w = cols * colW + 40.0f * s, h = filas * lh + 92.0f * s;
    float x = (W - w) * 0.5f, y = (H - h) * 0.5f;
    int hechas = 0;
    for (const auto& e : especies) if (registradas.count(e)) hechas++;
    ui.Rect(glm::vec4(x, y, w, h), glm::vec4(0.01f, 0.07f, 0.12f, 0.90f), 14.0f * s);
    char b[96];
    snprintf(b, sizeof(b), "BITÁCORA DE ESPECIES   %d / %d", hechas, n);
    ui.Texto(b, x + w * 0.5f, y + 14.0f * s, 16.0f * s, glm::vec4(0.55f, 0.9f, 1.0f, 1.0f), 1);
    for (int i = 0; i < n; i++) {
        int c = i / filas, f = i % filas;
        float tx = x + 20.0f * s + c * colW, ty = y + 46.0f * s + f * lh;
        bool ok = registradas.count(especies[i]) > 0;
        const Ficha* fi = BuscarFicha(especies[i]);
        std::string nombre = fi ? fi->nombre : especies[i];
        glm::vec4 col = ok ? glm::vec4(0.5f, 1.0f, 0.6f, 1.0f) : glm::vec4(0.95f, 0.95f, 0.95f, 1.0f);
        ui.Texto(ok ? "✓" : "·", tx + 6.0f * s, ty - 2.0f * s, 16.0f * s, col, 1);
        ui.Texto(nombre, tx + 22.0f * s, ty, 15.0f * s, col, 0, !ok);
    }
    ui.Texto(hechas == n ? "¡Bitácora completa!   ·   L para cerrar" : "Busca las que faltan (en blanco) y apúntales con la mira   ·   L para cerrar",
             x + w * 0.5f, y + h - 30.0f * s, 13.0f * s, glm::vec4(1.0f, 0.85f, 0.5f, 1.0f), 1, false);
}

// segundos: tiempo desde que se completo la zona (el anuncio aparece y se queda 16 s)
inline void HUDZonaCompletada(MenuInicio& ui, float segundos, const std::string& zona,
                              const std::vector<std::string>& resumen, const std::string& mensaje,
                              const std::string& titulo = "¡ZONA COMPLETADA!", float duracion = 16.0f)
{
    if (segundos < 0.0f || segundos > duracion) return;
    float a = segundos < 0.6f ? segundos / 0.6f : (segundos > duracion - 1.0f ? duracion - segundos : 1.0f);
    float s = ui.Escala(), W = ui.Ancho(), H = ui.Alto();
    std::vector<std::string> ls = ui.Lineas(mensaje, 520.0f * s, 15.0f * s, false);
    float w = 580.0f * s, h = 128.0f * s + resumen.size() * 24.0f * s + ls.size() * 20.0f * s + 40.0f * s;
    float x = (W - w) * 0.5f, y = (H - h) * 0.42f;
    glm::vec3 oro(1.0f, 0.82f, 0.35f);
    float pulso = 0.35f + 0.15f * sinf(segundos * 3.0f);
    ui.Resplandor(glm::vec4(x - 22 * s, y - 22 * s, w + 44 * s, h + 44 * s), glm::vec4(oro, pulso * a), 20 * s, 22 * s);
    ui.Rect(glm::vec4(x, y, w, h), glm::vec4(0.01f, 0.06f, 0.10f, 0.92f * a), 18.0f * s);
    ui.Rect(glm::vec4(x, y, w, 5.0f * s), glm::vec4(oro, a), 2.0f * s);
    ui.Texto(titulo, W * 0.5f, y + 20 * s, 34 * s, glm::vec4(oro, a), 1);
    ui.Texto(zona, W * 0.5f, y + 66 * s, 17 * s, glm::vec4(0.6f, 0.92f, 1.0f, a), 1);
    float yy = y + 104 * s;
    for (const std::string& r : resumen) {
        ui.Texto("✓", x + 48 * s, yy - 2 * s, 17 * s, glm::vec4(0.5f, 1.0f, 0.6f, a), 1);
        ui.Texto(r, x + 66 * s, yy, 15 * s, glm::vec4(0.95f, 0.97f, 1.0f, a), 0, false);
        yy += 24 * s;
    }
    yy += 10 * s;
    for (const std::string& l : ls) { ui.Texto(l, W * 0.5f, yy, 15 * s, glm::vec4(1.0f, 0.9f, 0.65f, a), 1, false); yy += 20 * s; }
    ui.Texto("Esc: volver al menú   ·   o sigue explorando la zona", W * 0.5f, y + h - 28 * s, 13 * s, glm::vec4(0.75f, 0.85f, 0.9f, a), 1, false);
}


// ============================================================================================
// PANTALLA LIMPIA: elementos compactos que se muestran mientras no se presione H
// ============================================================================================

// Indicaciones al entrar a una zona: historia corta, objetivos y controles
struct IntroZona
{
    std::string titulo, subtitulo, historia;
    std::vector<std::string> objetivos, controles;
    glm::vec3 color = glm::vec3(0.45f, 0.85f, 1.0f);
};

inline void HUDIntro(MenuInicio& ui, const IntroZona& in, float segundos)
{
    float a = segundos < 0.5f ? segundos / 0.5f : 1.0f;
    float s = ui.Escala(), W = ui.Ancho(), H = ui.Alto();
    ui.Rect(glm::vec4(0, 0, W, H), glm::vec4(0.0f, 0.02f, 0.05f, 0.55f * a));
    float w = 600.0f * s;
    std::vector<std::string> hist = ui.Lineas(in.historia, w - 70.0f * s, 15.0f * s, false);
    float h = 150.0f * s + hist.size() * 21.0f * s + in.objetivos.size() * 25.0f * s + in.controles.size() * 20.0f * s + 70.0f * s;
    float x = (W - w) * 0.5f, y = (H - h) * 0.45f;
    ui.Rect(glm::vec4(x, y, w, h), glm::vec4(0.01f, 0.06f, 0.10f, 0.94f * a), 18.0f * s);
    ui.Rect(glm::vec4(x, y, w, 5.0f * s), glm::vec4(in.color, a), 2.0f * s);
    ui.Texto(in.titulo, W * 0.5f, y + 20 * s, 30 * s, glm::vec4(in.color, a), 1);
    ui.Texto(in.subtitulo, W * 0.5f, y + 60 * s, 16 * s, glm::vec4(0.85f, 0.92f, 1.0f, a), 1, false);
    float yy = y + 96 * s;
    for (const std::string& l : hist) { ui.Texto(l, W * 0.5f, yy, 15 * s, glm::vec4(0.92f, 0.95f, 1.0f, a), 1, false); yy += 21 * s; }
    yy += 12 * s;
    ui.Texto("OBJETIVOS", x + 35 * s, yy, 13 * s, glm::vec4(in.color, a)); yy += 24 * s;
    for (size_t i = 0; i < in.objetivos.size(); i++) {
        char n[8]; snprintf(n, sizeof(n), "%d.", (int)i + 1);
        ui.Texto(n, x + 40 * s, yy, 15 * s, glm::vec4(in.color, a), 0);
        ui.Texto(in.objetivos[i], x + 62 * s, yy, 15 * s, glm::vec4(1.0f, 1.0f, 1.0f, a), 0, false);
        yy += 25 * s;
    }
    yy += 10 * s;
    for (const std::string& c : in.controles) { ui.Texto(c, W * 0.5f, yy, 13 * s, glm::vec4(0.75f, 0.85f, 0.92f, a), 1, false); yy += 20 * s; }
    float pulso = 0.6f + 0.4f * sinf(segundos * 4.0f);
    ui.Texto("Clic o Enter para comenzar", W * 0.5f, y + h - 38 * s, 17 * s, glm::vec4(1.0f, 0.85f, 0.45f, a * pulso), 1);
}

// Objetivo actual en una sola linea (arriba a la izquierda)
// Nivel avanzado: solo el avance (hechos/total), sin decir que hacer ni como.
inline void HUDObjetivo(MenuInicio& ui, const std::string& textoObjetivo, int hechos, int total, const std::string& extraObjetivo = std::string())
{
    bool soloAvance = HUD().nivel == NIVEL_AVANZADO;
    std::string texto = soloAvance ? std::string(hechos >= total ? "¡Zona completa!" : "Avance de la zona") : textoObjetivo;
    std::string extra = soloAvance ? std::string() : extraObjetivo;
    float s = ui.Escala();
    char b[32]; snprintf(b, sizeof(b), "%d/%d", hechos, total);
    float tw = ui.MedirTexto(texto, 15.0f * s, true);
    float w = tw + 96.0f * s, h = extra.empty() ? 38.0f * s : 58.0f * s, x = 18.0f * s, y = 18.0f * s;
    ui.Rect(glm::vec4(x, y, w, h), glm::vec4(0.01f, 0.07f, 0.12f, 0.62f), 12.0f * s);
    ui.Texto(b, x + 14 * s, y + 10 * s, 15 * s, glm::vec4(0.55f, 0.9f, 1.0f, 1.0f), 0);
    ui.Texto(texto, x + 62 * s, y + 10 * s, 15 * s, glm::vec4(1.0f), 0, true);
    if (!extra.empty()) ui.Texto(extra, x + 62 * s, y + 34 * s, 12 * s, glm::vec4(0.75f, 0.88f, 0.95f, 0.95f), 0, false);
}

// Termometro compacto (derecha): barra delgada, temperatura y teclas
inline void HUDTermometroCompacto(MenuInicio& ui, float temp, float tMin, float tMax, float marca, glm::vec3 color, const char* teclas)
{
    float s = ui.Escala(), W = ui.Ancho();
    float w = 74.0f * s, h = 238.0f * s, x = W - w - 18.0f * s, y = 18.0f * s;
    ui.Rect(glm::vec4(x, y, w, h), glm::vec4(0.01f, 0.07f, 0.12f, 0.55f), 12.0f * s);
    char b[32]; snprintf(b, sizeof(b), "%.1f°", temp);
    ui.Texto(b, x + w * 0.5f, y + 10 * s, 19 * s, glm::vec4(color, 1.0f), 1);
    float bx = x + w * 0.5f - 6 * s, by = y + 42 * s, bh = h - 78 * s;
    float v = (temp - tMin) / (tMax - tMin); v = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
    float vm = (marca - tMin) / (tMax - tMin);
    ui.Rect(glm::vec4(bx, by, 12 * s, bh), glm::vec4(1, 1, 1, 0.15f), 6 * s);
    ui.Rect(glm::vec4(bx, by + bh * (1.0f - v), 12 * s, bh * v), glm::vec4(color, 1.0f), 6 * s);
    ui.Rect(glm::vec4(bx - 6 * s, by + bh * (1.0f - vm) - 1 * s, 24 * s, 2 * s), glm::vec4(1.0f, 0.75f, 0.4f, 1.0f));
    ui.Texto(teclas, x + w * 0.5f, y + h - 28 * s, 12 * s, glm::vec4(0.8f, 0.9f, 1.0f, 0.9f), 1, false);
}

// Barra de progreso compacta (derecha)
inline void HUDBarraCompacta(MenuInicio& ui, const char* titulo, float valor, glm::vec3 color, const std::string& extra = std::string())
{
    float s = ui.Escala(), W = ui.Ancho();
    float w = 200.0f * s, h = extra.empty() ? 50.0f * s : 70.0f * s, x = W - w - 18.0f * s, y = 18.0f * s;
    ui.Rect(glm::vec4(x, y, w, h), glm::vec4(0.01f, 0.07f, 0.12f, 0.55f), 12.0f * s);
    char b[16]; snprintf(b, sizeof(b), "%d%%", (int)(valor * 100.0f + 0.5f));
    ui.Texto(titulo, x + 14 * s, y + 8 * s, 13 * s, glm::vec4(0.75f, 0.9f, 1.0f, 1.0f), 0);
    ui.Texto(b, x + w - 14 * s, y + 6 * s, 16 * s, glm::vec4(color, 1.0f), 2);
    ui.Rect(glm::vec4(x + 14 * s, y + 31 * s, w - 28 * s, 8 * s), glm::vec4(1, 1, 1, 0.15f), 4 * s);
    ui.Rect(glm::vec4(x + 14 * s, y + 31 * s, (w - 28 * s) * (valor < 0.02f ? 0.02f : valor), 8 * s), glm::vec4(color, 1.0f), 4 * s);
    if (!extra.empty()) ui.Texto(extra, x + 14 * s, y + 47 * s, 12 * s, glm::vec4(0.8f, 0.9f, 1.0f, 0.9f), 0, false);
}

// Aviso de que la zona termino: siguiente zona (abajo al centro)
inline void HUDSiguienteZona(MenuInicio& ui, const std::string& texto, float restante, float t)
{
    float s = ui.Escala(), W = ui.Ancho(), H = ui.Alto();
    char b[160]; snprintf(b, sizeof(b), "%s  ·  Enter para continuar (%d s)", texto.c_str(), (int)ceilf(restante > 0 ? restante : 0));
    float w = ui.MedirTexto(b, 17.0f * s, true) + 50.0f * s, h = 46.0f * s, x = (W - w) * 0.5f, y = H - h - 26.0f * s;
    float pulso = 0.35f + 0.25f * sinf(t * 4.0f);
    ui.Resplandor(glm::vec4(x - 10 * s, y - 10 * s, w + 20 * s, h + 20 * s), glm::vec4(1.0f, 0.82f, 0.35f, pulso), 16 * s, 12 * s);
    ui.Rect(glm::vec4(x, y, w, h), glm::vec4(0.01f, 0.06f, 0.10f, 0.92f), 14.0f * s);
    ui.Texto(b, W * 0.5f, y + 13 * s, 17 * s, glm::vec4(1.0f, 0.85f, 0.45f, 1.0f), 1);
}

// ============================================================================================
// DIFICULTAD POR TIEMPO (interfaz/estado_hud.h → TIEMPO_NIVEL)
// ============================================================================================
// Reloj de la zona (arriba a la derecha, a la izquierda del termometro o de la barra de limpieza)
//   restante: segundos que quedan · ocupado: ancho que ya usa otro panel en esa esquina
inline void HUDCronometro(MenuInicio& ui, float restante, float t, float ocupado)
{
    if (restante < 0.0f) restante = 0.0f;
    float s = ui.Escala(), W = ui.Ancho();
    int seg = (int)ceilf(restante);
    char b[16]; snprintf(b, sizeof(b), "%d:%02d", seg / 60, seg % 60);
    // [CAMBIAR] colores: blanco, amarillo en el ultimo minuto y rojo (con pulso) en los ultimos 30 s
    glm::vec3 c = restante <= 30.0f ? glm::vec3(1.0f, 0.45f, 0.4f) : (restante <= 60.0f ? glm::vec3(1.0f, 0.8f, 0.35f) : glm::vec3(0.92f, 0.97f, 1.0f));
    float w = 128.0f * s, h = 40.0f * s, y = 18.0f * s;
    float x = W - 18.0f * s - (ocupado > 0.0f ? ocupado + 12.0f * s : 0.0f) - w;
    if (restante <= 30.0f) ui.Resplandor(glm::vec4(x - 8 * s, y - 8 * s, w + 16 * s, h + 16 * s), glm::vec4(c, 0.25f + 0.2f * sinf(t * 6.0f)), 12 * s, 10 * s);
    ui.Rect(glm::vec4(x, y, w, h), glm::vec4(0.01f, 0.07f, 0.12f, 0.62f), 12.0f * s);
    ui.Texto("TIEMPO", x + 14 * s, y + 14 * s, 11 * s, glm::vec4(0.75f, 0.9f, 1.0f, 0.9f), 0);
    ui.Texto(b, x + w - 14 * s, y + 8 * s, 21 * s, glm::vec4(c, 1.0f), 2);
}

// Se acabo el tiempo: Enter repite la zona, Esc vuelve al menu
inline void HUDTiempoAgotado(MenuInicio& ui, const std::string& zona, float segundos)
{
    float a = segundos < 0.5f ? segundos / 0.5f : 1.0f;
    float s = ui.Escala(), W = ui.Ancho(), H = ui.Alto();
    ui.Rect(glm::vec4(0, 0, W, H), glm::vec4(0.25f, 0.02f, 0.02f, 0.45f * a));
    float w = 540.0f * s, h = 190.0f * s, x = (W - w) * 0.5f, y = (H - h) * 0.42f;
    glm::vec3 rojo(1.0f, 0.5f, 0.45f);
    ui.Rect(glm::vec4(x, y, w, h), glm::vec4(0.01f, 0.06f, 0.10f, 0.94f * a), 18.0f * s);
    ui.Rect(glm::vec4(x, y, w, 5.0f * s), glm::vec4(rojo, a), 2.0f * s);
    ui.Texto("¡SE ACABÓ EL TIEMPO!", W * 0.5f, y + 22 * s, 34 * s, glm::vec4(rojo, a), 1);
    ui.Texto(zona, W * 0.5f, y + 70 * s, 17 * s, glm::vec4(0.6f, 0.92f, 1.0f, a), 1);
    int lim = (int)TiempoLimite();
    char b[96]; snprintf(b, sizeof(b), "Nivel %s: tenías %d:%02d para completar la zona", NombreNivel(HUD().nivel), lim / 60, lim % 60);
    ui.Texto(b, W * 0.5f, y + 100 * s, 15 * s, glm::vec4(0.92f, 0.95f, 1.0f, a), 1, false);
    float pulso = 0.6f + 0.4f * sinf(segundos * 4.0f);
    ui.Texto("Enter: intentar de nuevo   ·   Esc: volver al menú", W * 0.5f, y + h - 42 * s, 16 * s, glm::vec4(1.0f, 0.85f, 0.45f, a * pulso), 1);
}

// Pista discreta de teclas de informacion (abajo a la derecha)
// Nivel avanzado: solo aparece si el buzo se queda quieto (o con los paneles de H abiertos).
inline void HUDPistaTeclas(MenuInicio& ui)
{
    float v = HUD().detallado ? 1.0f : VisibilidadTeclas();
    if (v <= 0.0f) return;
    float s = ui.Escala(), W = ui.Ancho(), H = ui.Alto();
    ui.Texto(HUD().detallado ? "H: ocultar ayuda" : "H: ayuda  ·  I: ficha  ·  U: nivel", W - 18 * s, H - 30 * s, 12 * s, glm::vec4(0.8f, 0.9f, 1.0f, 0.55f * v), 2, false);
}

// ============================================================================================
// NIVELES DE USO · teclas en el borde de la pantalla (abajo a la izquierda)
//   basico e intermedio: siempre visibles · avanzado: solo si el buzo se queda quieto
//   (AYUDA_QUIETO_AVANZADO segundos en interfaz/estado_hud.h). Con H se ocultan porque ya
//   se ve la ayuda completa.
// [CAMBIAR] las lineas salen de TeclasZona() en ProyectoFinal.cpp
// ============================================================================================
inline void HUDTeclasBorde(MenuInicio& ui, const std::vector<std::string>& lineas)
{
    if (HUD().detallado || lineas.empty()) return;
    float v = VisibilidadTeclas();
    if (v <= 0.0f) return;
    float s = ui.Escala(), H = ui.Alto();
    float tam = 12.0f * s, lh = tam * 1.45f, w = 0.0f;
    for (const std::string& l : lineas) { float m = ui.MedirTexto(l, tam, false); w = m > w ? m : w; }
    w += 28.0f * s;
    float h = lineas.size() * lh + 34.0f * s, x = 18.0f * s, y = H - h - 18.0f * s;
    ui.Rect(glm::vec4(x, y, w, h), glm::vec4(0.01f, 0.07f, 0.12f, 0.55f * v), 10.0f * s);
    const char* titulo = "CONTROLES";   // [CAMBIAR] titulo del panel de teclas
    ui.Texto(titulo, x + 14 * s, y + 9 * s, 11 * s, glm::vec4(1.0f, 0.82f, 0.35f, 0.95f * v), 0);
    float yy = y + 28 * s;
    for (const std::string& l : lineas) { ui.Texto(l, x + 14 * s, yy, tam, glm::vec4(0.88f, 0.94f, 1.0f, 0.9f * v), 0, false); yy += lh; }
}

#endif
