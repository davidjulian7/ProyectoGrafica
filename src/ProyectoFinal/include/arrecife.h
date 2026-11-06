#ifndef ARRECIFE_H
#define ARRECIFE_H
/*
* Zona 1 - Arrecifes: estres termico y blanqueamiento.
*
*  - Termometro (HUD): flecha abajo baja y flecha arriba sube la temperatura; T activa un
*    calentamiento automatico; R restaura el arrecife.
*  - Cada coral (y anemona y almeja) acumula estres termico cuando el agua pasa
*    del umbral de 29 grados: primero se blanquea por manchas y, si el estres dura
*    demasiado, muere y lo cubren las algas. Si el agua se enfria a tiempo, los
*    corales vivos recuperan su color.
*  - El ecosistema responde: los cardumenes se reducen y el agua se enturbia.
*  - Bitacora: al observar una especie con la mira se registra.
*/
#include <string>
#include <vector>
#include <set>
#include <cstdio>
#include <cmath>
#include <zonas.h>
#include <menu.h>
#include <fichas.h>
#include <hud_comun.h>

class MecanicaArrecife
{
public:
    const float T_MIN = 26.0f, T_MAX = 34.0f, T_UMBRAL = 29.0f, T_RECUPERA = 28.0f, T_INICIAL = 26.5f;
    float temperatura = 26.5f;
    bool automatico = false;
    float salud = 1.0f, fBlanqueo = 0.0f, fMuerte = 0.0f;
    int corales = 0, blanqueados = 0, muertos = 0;
    // eventos de sonido (ProyectoFinal los reproduce y los apaga)
    bool sonarAlerta = false, sonarDescubrir = false;
    bool verLista = false;                 // tecla L: lista de especies de la bitacora
    bool completado = false;

    void Reiniciar(Escenario& esc)
    {
        temperatura = T_INICIAL; automatico = false; hitos = 0; avisos.clear();
        registradas.clear(); ultimaMira.clear(); tiempoMira = 0.0f;
        std::set<std::string> especies;
        for (auto& e : esc.Elementos()) {
            e.estres = e.blanqueo = e.muerte = 0.0f; e.presencia = 1.0f;
            const Ficha* f = BuscarFicha(e.etiqueta);
            if (f && f->bitacora) especies.insert(e.etiqueta);
        }
        totalEspecies = (int)especies.size();
        especiesLista = OrdenarEspecies(especies);
        tMax = T_INICIAL; calento = recupero = completado = false; tCompleto = -1.0f; verLista = false;
        Aviso("Arrecife sano a 26.5 °C. Usa las flechas ↑ y ↓ para cambiar la temperatura del agua y observa qué pasa.", glm::vec3(0.55f, 0.9f, 1.0f), 7.0f);
    }

    void Tecla(int key, Escenario& esc)
    {
        if (key == GLFW_KEY_R) {
            if (calento) recupero = true;
            temperatura = T_INICIAL; automatico = false; hitos = 0;
            for (auto& e : esc.Elementos()) { e.estres = e.blanqueo = e.muerte = 0.0f; e.presencia = 1.0f; }
            Aviso("Arrecife restaurado. En la realidad, un arrecife tarda de 10 a 15 años en recuperarse de un blanqueamiento masivo.", glm::vec3(0.5f, 1.0f, 0.6f), 7.0f);
        }
        else if (key == GLFW_KEY_L) verLista = !verLista;
        else if (key == GLFW_KEY_T) {
            automatico = !automatico;
            if (automatico) Aviso("Simulación de calentamiento: la temperatura sube poco a poco, como en una ola de calor marina.", glm::vec3(1.0f, 0.75f, 0.4f), 6.0f);
            else Aviso("Calentamiento automático detenido.", glm::vec3(0.8f, 0.9f, 1.0f), 3.0f);
        }
    }

    void Actualizar(float dt, GLFWwindow* ventana, Escenario& esc, const Elemento* sel)
    {
        if (dt > 0.1f) dt = 0.1f;
        // --- termometro (no responde en modo edicion: ahi las teclas mueven objetos)
        if (!esc.modoEdicion) {
        if (glfwGetKey(ventana, GLFW_KEY_UP) == GLFW_PRESS) { temperatura += 1.2f * dt; automatico = false; }
        if (glfwGetKey(ventana, GLFW_KEY_DOWN) == GLFW_PRESS) { temperatura -= 1.2f * dt; automatico = false; }
        }
        if (automatico) temperatura += 0.12f * dt;
        temperatura = temperatura < T_MIN ? T_MIN : (temperatura > T_MAX ? T_MAX : temperatura);

        // --- estres termico de cada coral
        float sumB = 0.0f, sumM = 0.0f;
        corales = blanqueados = muertos = 0;
        float exceso = temperatura - T_UMBRAL;
        for (auto& e : esc.Elementos()) {
            if (!e.esCoral) continue;
            corales++;
            if (exceso > 0.0f) e.estres += exceso * e.sensibilidad * dt * 0.10f;
            else if (temperatura < T_RECUPERA && e.muerte < 0.5f) e.estres -= (T_RECUPERA - temperatura + 0.5f) * dt * 0.06f;
            e.estres = e.estres < 0.0f ? 0.0f : (e.estres > 2.5f ? 2.5f : e.estres);
            float objetivo = e.estres > 1.0f ? 1.0f : e.estres;
            if (e.muerte > 0.5f) objetivo = 1.0f;
            e.blanqueo += (objetivo - e.blanqueo) * (dt * 1.5f > 1.0f ? 1.0f : dt * 1.5f);
            if (e.estres > 1.0f && exceso > 0.0f) e.muerte += (e.estres - 1.0f) * dt * 0.07f;
            if (e.muerte > 1.0f) e.muerte = 1.0f;
            sumB += e.blanqueo; sumM += e.muerte;
            if (e.muerte > 0.5f) muertos++;
            else if (e.blanqueo > 0.5f) blanqueados++;
        }
        fBlanqueo = corales ? sumB / corales : 0.0f;
        fMuerte = corales ? sumM / corales : 0.0f;
        salud = 1.0f - (0.6f * fBlanqueo + 0.4f * fMuerte);

        // --- el ecosistema responde
        for (auto& e : esc.Elementos())
            if (e.mov == MOV_CARDUMEN) e.presencia = 0.25f + 0.75f * salud;
        float calor = (temperatura - T_MIN) / (T_MAX - T_MIN);
        esc.cfg.niebla = glm::mix(esc.cfgBase.niebla, glm::vec3(0.15f, 0.36f, 0.33f), 0.55f * fMuerte + 0.15f * fBlanqueo);
        esc.cfg.densidad = esc.cfgBase.densidad + 0.012f * fMuerte;
        esc.cfg.solColor = esc.cfgBase.solColor * glm::mix(glm::vec3(1.0f), glm::vec3(1.10f, 1.0f, 0.86f), calor);
        esc.cfg.causticas = esc.cfgBase.causticas * (1.0f + 0.3f * calor);

        // --- objetivos de la zona
        if (temperatura > tMax) tMax = temperatura;
        if (hitos & 2) calento = true;                                       // provoco un blanqueamiento masivo
        if (calento && temperatura < T_RECUPERA) recupero = true;            // y despues enfrio el agua
        if (completado) tCompleto += dt;
        if (!completado && calento && recupero && (int)registradas.size() >= totalEspecies) {
            completado = true; tCompleto = 0.0f; sonarDescubrir = true;
        }

        // --- avisos educativos
        if (!(hitos & 1) && temperatura >= T_UMBRAL) {
            hitos |= 1; sonarAlerta = true;
            Aviso("¡Alerta! El agua superó los 29 °C: los corales sufren estrés térmico y empiezan a expulsar las algas que les dan color y alimento.", glm::vec3(1.0f, 0.7f, 0.35f), 8.0f);
        }
        if (!(hitos & 2) && fBlanqueo > 0.5f) {
            hitos |= 2; sonarAlerta = true;
            Aviso("Blanqueamiento masivo: más de la mitad del arrecife perdió su color. El coral blanco sigue vivo, pero está muriendo de hambre.", glm::vec3(1.0f, 0.85f, 0.6f), 8.0f);
        }
        if (!(hitos & 4) && muertos > 0) {
            hitos |= 4; sonarAlerta = true;
            Aviso("Los corales con estrés prolongado comienzan a morir y las algas los cubren.", glm::vec3(1.0f, 0.5f, 0.45f), 7.0f);
        }
        if (!(hitos & 8) && fMuerte > 0.85f) {
            hitos |= 8; sonarAlerta = true;
            Aviso("El arrecife quedó inerte: tortugas y peces nadan entre corales muertos, sin refugio ni alimento.", glm::vec3(1.0f, 0.45f, 0.45f), 9.0f);
        }
        if ((hitos & 1) && !(hitos & 16) && temperatura < T_RECUPERA && fBlanqueo > 0.1f) {
            hitos |= 16;
            Aviso(fMuerte > 0.5f ? "El agua se enfrió, pero muchos corales ya murieron: el daño no se revierte rápido."
                                  : "El agua se enfría: los corales que siguen vivos pueden recuperar sus algas y su color.",
                  glm::vec3(0.5f, 1.0f, 0.6f), 7.0f);
        }

        // --- bitacora de especies
        if (sel) {
            if (sel->etiqueta == ultimaMira) tiempoMira += dt; else { ultimaMira = sel->etiqueta; tiempoMira = 0.0f; }
            const Ficha* f = BuscarFicha(sel->etiqueta);
            if (f && f->bitacora && tiempoMira > 0.25f && !registradas.count(sel->etiqueta)) {
                registradas.insert(sel->etiqueta); sonarDescubrir = true;
                Aviso("Nueva especie en tu bitácora: " + f->nombre, glm::vec3(0.55f, 0.95f, 1.0f), 3.5f);
                if ((int)registradas.size() == totalEspecies)
                    Aviso("¡Bitácora completa! Registraste todas las especies del arrecife.", glm::vec3(0.5f, 1.0f, 0.6f), 7.0f);
            }
        }
        else ultimaMira.clear();

        for (size_t i = 0; i < avisos.size(); ) {
            avisos[i].vida -= dt;
            if (avisos[i].vida <= 0.0f) avisos.erase(avisos.begin() + i); else i++;
        }
    }

    // Texto de estado para la ficha de un coral
    std::string Detalle(const Elemento& e, glm::vec3& color) const
    {
        if (!e.esCoral) return "";
        char b[64];
        if (e.muerte > 0.5f) { color = glm::vec3(1.0f, 0.5f, 0.45f); return "Estado: muerto, cubierto de algas"; }
        if (e.blanqueo > 0.15f) { color = glm::vec3(1.0f, 0.8f, 0.5f); snprintf(b, sizeof(b), "Estado: blanqueado %d%%", (int)(e.blanqueo * 100.0f + 0.5f)); return b; }
        color = glm::vec3(0.5f, 1.0f, 0.6f); return "Estado: sano";
    }

    void DibujarHUD(MenuInicio& ui, GLFWwindow* ventana, float t, float tiempoEnZona)
    {
        ui.Comenzar(ventana);
        float s = ui.Escala(), W = ui.Ancho(), H = ui.Alto();
        char b[96];
        glm::vec3 cTemp = ColorTemperatura(temperatura);

        // ---------------- termometro (derecha)
        float pw = 150.0f * s, ph = 430.0f * s, px = W - pw - 18.0f * s, py = 96.0f * s;
        if (temperatura >= T_UMBRAL) ui.Resplandor(glm::vec4(px - 14 * s, py - 14 * s, pw + 28 * s, ph + 28 * s), glm::vec4(cTemp, 0.25f + 0.2f * sinf(t * 5.0f)), 18 * s, 14 * s);
        ui.Rect(glm::vec4(px, py, pw, ph), glm::vec4(0.01f, 0.07f, 0.12f, 0.75f), 16.0f * s);
        ui.Texto("TEMPERATURA", px + pw * 0.5f, py + 12 * s, 13 * s, glm::vec4(0.55f, 0.9f, 1.0f, 1.0f), 1);
        snprintf(b, sizeof(b), "%.1f °C", temperatura);
        ui.Texto(b, px + pw * 0.5f, py + 30 * s, 32 * s, glm::vec4(cTemp, 1.0f), 1);
        float tx = px + pw * 0.40f, tw = 20.0f * s, ty = py + 88 * s, th = 250.0f * s;
        ui.Rect(glm::vec4(tx - tw * 0.5f - 3 * s, ty - 3 * s, tw + 6 * s, th + 6 * s), glm::vec4(1, 1, 1, 0.30f), (tw + 6 * s) * 0.5f);
        ui.Rect(glm::vec4(tx - tw * 0.5f, ty, tw, th), glm::vec4(0.03f, 0.10f, 0.15f, 0.95f), tw * 0.5f);
        float frac = (temperatura - T_MIN) / (T_MAX - T_MIN);
        float fh = th * frac;
        ui.Rect(glm::vec4(tx - tw * 0.5f + 4 * s, ty + th - fh, tw - 8 * s, fh + 6 * s), glm::vec4(cTemp, 1.0f), (tw - 8 * s) * 0.5f);
        float br = 21.0f * s;
        ui.Rect(glm::vec4(tx - br - 3 * s, ty + th - 3 * s, 2 * br + 6 * s, 2 * br + 6 * s), glm::vec4(1, 1, 1, 0.30f), br + 3 * s);
        ui.Rect(glm::vec4(tx - br, ty + th, 2 * br, 2 * br), glm::vec4(cTemp, 1.0f), br);
        ui.Rect(glm::vec4(tx - br * 0.45f, ty + th + br * 0.35f, br * 0.5f, br * 0.5f), glm::vec4(1, 1, 1, 0.45f), br * 0.25f);
        for (int k = 26; k <= 34; k += 2) {
            float y = ty + th * (1.0f - (k - T_MIN) / (T_MAX - T_MIN));
            ui.Rect(glm::vec4(tx - tw * 0.5f - 14 * s, y - 1 * s, 9 * s, 2 * s), glm::vec4(1, 1, 1, 0.7f));
            snprintf(b, sizeof(b), "%d", k);
            ui.Texto(b, tx - tw * 0.5f - 18 * s, y - 8 * s, 13 * s, glm::vec4(0.85f, 0.92f, 1.0f, 0.9f), 2, false);
        }
        float yu = ty + th * (1.0f - (T_UMBRAL - T_MIN) / (T_MAX - T_MIN));
        ui.Rect(glm::vec4(tx - tw * 0.5f - 5 * s, yu - 1.5f * s, tw + 10 * s, 3 * s), glm::vec4(1.0f, 0.55f, 0.3f, 1.0f));
        ui.Texto("Umbral", tx + tw * 0.5f + 9 * s, yu - 16 * s, 12 * s, glm::vec4(1.0f, 0.65f, 0.4f, 1.0f));
        ui.Texto("29 °C", tx + tw * 0.5f + 9 * s, yu - 2 * s, 12 * s, glm::vec4(1.0f, 0.65f, 0.4f, 1.0f));
        ui.Texto("↑  subir   ↓  bajar", px + pw * 0.5f, py + ph - 42 * s, 12 * s, glm::vec4(0.85f, 0.92f, 1.0f, 0.95f), 1, false);
        ui.Texto(automatico ? "T  auto (activo)" : "T  auto   R  reiniciar", px + pw * 0.5f, py + ph - 24 * s, 12 * s,
                 automatico ? glm::vec4(1.0f, 0.75f, 0.4f, 1.0f) : glm::vec4(0.85f, 0.92f, 1.0f, 0.95f), 1, false);

        // ---------------- estado del arrecife (abajo a la izquierda)
        float bw = 330.0f * s, bh = 222.0f * s, bx = 18.0f * s, by = H - bh - 18.0f * s;
        ui.Rect(glm::vec4(bx, by, bw, bh), glm::vec4(0.01f, 0.07f, 0.12f, 0.75f), 14.0f * s);
        ui.Texto("SALUD DEL ARRECIFE", bx + 16 * s, by + 12 * s, 13 * s, glm::vec4(0.55f, 0.9f, 1.0f, 1.0f));
        glm::vec3 cSalud = salud > 0.66f ? glm::vec3(0.4f, 0.95f, 0.55f) : (salud > 0.33f ? glm::vec3(1.0f, 0.8f, 0.35f) : glm::vec3(1.0f, 0.45f, 0.4f));
        float barW = bw - 32 * s - 52 * s;
        ui.Rect(glm::vec4(bx + 16 * s, by + 36 * s, barW, 14 * s), glm::vec4(1, 1, 1, 0.15f), 7 * s);
        ui.Rect(glm::vec4(bx + 16 * s, by + 36 * s, barW * (salud < 0.02f ? 0.02f : salud), 14 * s), glm::vec4(cSalud, 1.0f), 7 * s);
        snprintf(b, sizeof(b), "%d%%", (int)(salud * 100.0f + 0.5f));
        ui.Texto(b, bx + bw - 16 * s, by + 31 * s, 19 * s, glm::vec4(cSalud, 1.0f), 2);
        snprintf(b, sizeof(b), "Corales blanqueados: %d de %d", blanqueados, corales);
        ui.Texto(b, bx + 16 * s, by + 60 * s, 14 * s, glm::vec4(0.95f, 0.95f, 0.95f, 1.0f), 0, false);
        snprintf(b, sizeof(b), "Corales muertos: %d", muertos);
        ui.Texto(b, bx + 16 * s, by + 82 * s, 14 * s, glm::vec4(muertos ? glm::vec3(1.0f, 0.6f, 0.55f) : glm::vec3(0.95f), 1.0f), 0, false);
        snprintf(b, sizeof(b), "Especies registradas: %d de %d  (L: lista)", (int)registradas.size(), totalEspecies);
        ui.Texto(b, bx + 16 * s, by + 104 * s, 14 * s, glm::vec4(0.6f, 0.95f, 1.0f, 1.0f), 0, false);
        ui.Texto("OBJETIVOS", bx + 16 * s, by + 132 * s, 13 * s, glm::vec4(0.55f, 0.9f, 1.0f, 1.0f));
        bool hecho[3] = { calento, recupero, (int)registradas.size() >= totalEspecies };
        const char* obj[3] = { "Sube la temperatura hasta blanquear el arrecife", "Enfría el agua (↓) o restaura (R)", "Registra todas las especies" };
        for (int k = 0; k < 3; k++) {
            glm::vec4 c = hecho[k] ? glm::vec4(0.5f, 1.0f, 0.6f, 1.0f) : glm::vec4(0.92f, 0.95f, 1.0f, 1.0f);
            ui.Texto(hecho[k] ? "✓" : "·", bx + 22 * s, by + 152 * s + k * 22 * s, 16 * s, c, 1);
            ui.Texto(obj[k], bx + 36 * s, by + 154 * s + k * 22 * s, 13 * s, c, 0, false);
        }

        // ---------------- avisos (arriba al centro)
        float ay = 92.0f * s;
        for (int i = (int)avisos.size() - 1, n = 0; i >= 0 && n < 2; i--, n++) {
            const AvisoHUD& a = avisos[i];
            float alfa = a.vida < 0.6f ? a.vida / 0.6f : (a.total - a.vida < 0.3f ? (a.total - a.vida) / 0.3f : 1.0f);
            std::vector<std::string> ls = ui.Lineas(a.texto, 600.0f * s, 16.0f * s, true);
            float aw = 0.0f;
            for (auto& l : ls) { float m = ui.MedirTexto(l, 16.0f * s, true); aw = m > aw ? m : aw; }
            aw += 36 * s; float ah = ls.size() * 16.0f * s * 1.3f + 22 * s;
            float ax = (W - aw) * 0.5f;
            ui.Rect(glm::vec4(ax, ay, aw, ah), glm::vec4(0.01f, 0.07f, 0.12f, 0.80f * alfa), 12 * s);
            ui.Rect(glm::vec4(ax, ay, 4 * s, ah), glm::vec4(a.color, alfa), 2 * s);
            for (size_t k = 0; k < ls.size(); k++)
                ui.Texto(ls[k], W * 0.5f, ay + 11 * s + k * 16.0f * s * 1.3f, 16.0f * s, glm::vec4(a.color, alfa), 1);
            ay += ah + 8 * s;
        }

        // ---------------- ayuda de controles (primeros 20 segundos), entre el panel y el borde derecho
        float ayuda = tiempoEnZona < 18.0f ? 1.0f : (tiempoEnZona < 20.0f ? (20.0f - tiempoEnZona) / 2.0f : 0.0f);
        if (ayuda > 0.0f) {
            float x0 = bx + bw + 18 * s, x1 = W - 18 * s, aw = x1 - x0;
            const char* l1 = "WASD nadar  ·  Espacio / Ctrl subir y bajar  ·  Shift aletear  ·  Clic inspeccionar objeto  ·  Esc menú";
            const char* l2 = "↑ / ↓ temperatura  ·  T calentamiento automático  ·  R restaurar  ·  Apunta a las especies para registrarlas";
            std::vector<std::string> a1 = ui.Lineas(l1, aw - 28 * s, 14 * s, false), a2 = ui.Lineas(l2, aw - 28 * s, 14 * s, true);
            float lh = 14 * s * 1.35f, ah = (a1.size() + a2.size()) * lh + 20 * s, ay2 = H - 18 * s - ah;
            ui.Rect(glm::vec4(x0, ay2, aw, ah), glm::vec4(0.01f, 0.07f, 0.12f, 0.72f * ayuda), 12 * s);
            float yy = ay2 + 10 * s;
            for (auto& l : a1) { ui.Texto(l, x0 + aw * 0.5f, yy, 14 * s, glm::vec4(0.88f, 0.94f, 1.0f, ayuda), 1, false); yy += lh; }
            for (auto& l : a2) { ui.Texto(l, x0 + aw * 0.5f, yy, 14 * s, glm::vec4(1.0f, 0.8f, 0.5f, ayuda), 1, true); yy += lh; }
        }
        if (verLista) HUDListaEspecies(ui, especiesLista, registradas);
        if (completado) {
            char r1[64], r2[64], r3[64];
            snprintf(r1, sizeof(r1), "Blanqueaste el arrecife (máximo: %.1f °C)", tMax);
            snprintf(r2, sizeof(r2), "Enfriaste el agua: %d corales murieron en el proceso", muertos);
            snprintf(r3, sizeof(r3), "Registraste las %d especies del arrecife", totalEspecies);
            HUDZonaCompletada(ui, tCompleto, "Zona 1 · Arrecifes", { r1, r2, r3 },
                "Con 1.5 °C de calentamiento global se perdería entre el 70% y el 90% de los arrecifes de coral del planeta (IPCC, 2018).");
        }
        ui.Terminar();
    }

private:
    struct AvisoHUD { std::string texto; glm::vec3 color; float vida, total; };
    std::vector<AvisoHUD> avisos;
    int hitos = 0;
    std::set<std::string> registradas;
    int totalEspecies = 0;
    std::vector<std::string> especiesLista;
    float tMax = 26.5f, tCompleto = -1.0f;
    bool calento = false, recupero = false;
    std::string ultimaMira;
    float tiempoMira = 0.0f;

    void Aviso(const std::string& texto, glm::vec3 color, float segundos)
    {
        avisos.push_back({ texto, color, segundos, segundos });
        if (avisos.size() > 4) avisos.erase(avisos.begin());
    }

    glm::vec3 ColorTemperatura(float T) const
    {
        glm::vec3 frio(0.35f, 0.75f, 1.0f), templado(0.45f, 0.95f, 0.6f), calido(1.0f, 0.75f, 0.3f), caliente(1.0f, 0.35f, 0.3f);
        if (T < 28.0f) return glm::mix(frio, templado, (T - 26.0f) / 2.0f);
        if (T < 30.0f) return glm::mix(templado, calido, (T - 28.0f) / 2.0f);
        return glm::mix(calido, caliente, (T - 30.0f) / 2.5f > 1.0f ? 1.0f : (T - 30.0f) / 2.5f);
    }
};

#endif
