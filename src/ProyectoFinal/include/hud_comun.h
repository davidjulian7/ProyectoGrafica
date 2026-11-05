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
#include <menu.h>
#include <fichas.h>

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

#endif
