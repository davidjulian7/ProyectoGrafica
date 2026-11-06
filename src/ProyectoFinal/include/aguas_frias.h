#ifndef AGUAS_FRIAS_H
#define AGUAS_FRIAS_H
/*
* Zona 2 - Aguas Frias: deshielo, fantasmas y cadena trofica.
*
*  - Termometro (HUD): T calienta y G enfria el agua; el hielo se encoge al calentar.
*  - Con poco hielo, foca, pinguino, ballena y oso polar se vuelven fantasmas.
*  - X toma una foto al animal bajo la mira (14 especies).
*  - Clic en krill o pescado para recogerlos y en un animal para alimentarlo.
*  - Si el hielo llega a 0 se pierde el habitat; con los 5 objetivos y el hielo sano, la zona se restaura.
*/
#include <string>
#include <vector>
#include <set>
#include <cstdio>
#include <cmath>
#include <zonas.h>
#include <menu.h>
#include <fichas.h>

class MecanicaAguasFrias
{
public:
    // ---- termometro (grados centigrados del agua)
    const float T_MIN = -2.0f, T_MAX = 4.0f, T_CONGELA = 0.0f, T_INICIAL = -1.5f;
    float temperatura = -1.5f;
    float hielo = 1.0f;
    int estadoFinal = 0;
    bool sonarAlerta = false, sonarDescubrir = false, sonarRecoger = false;

    // ------------------------------------------------------------------ inicio
    void Reiniciar(Escenario& esc)
    {
        temperatura = T_INICIAL; hielo = 1.0f; estadoFinal = 0; hitos = 0; objCalor = false; objFantasma = false;
        avisos.clear(); fotos.clear(); alimentados.clear(); barcoParado = 0.0f; barcoXPrev = -999.0f;
        krillMochila = 0; pescadoMochila = 0; flash = 0.0f; marco = 0.0f;
        for (int i = 0; i < 4; i++) { pob[i] = 1.0f; apoyo[i] = 0.0f; }

        base.clear();
        for (auto& e : esc.Elementos()) {
            Base b;
            b.escala = e.escala; b.alpha = e.alpha; b.emision = e.emision; b.tinte = e.tinte; b.posY = e.pos.y;
            b.tipo = 0;
            if (e.archivo == "hielo1" || e.archivo == "hielo2" || e.archivo == "iceberg" || e.archivo == "iceberg_meshy") b.tipo = 1;
            else if (EsFantasma(e.archivo)) b.tipo = 2;
            base.push_back(b);
            e.visible = true; e.presencia = 1.0f; e.fantasma = 0.0f;
        }
        Aviso("Aguas frías a -1.5 °C. Usa T para calentar el agua y G para enfriarla, y observa qué le pasa al hielo.", glm::vec3(0.55f, 0.9f, 1.0f), 7.0f);
    }

    // ------------------------------------------------------------------ teclas sueltas (se llama desde key_callback)
    void Tecla(int key, Escenario& esc)
    {
        if (key == GLFW_KEY_R) {
            Reiniciar(esc);
            Aviso("Zona reiniciada. En la realidad, el hielo perdido tarda décadas en volver.", glm::vec3(0.5f, 1.0f, 0.6f), 6.0f);
        }
        else if (key == GLFW_KEY_X) TomarFoto(esc);
    }

    // ------------------------------------------------------------------ clic izquierdo: recoger y alimentar
    void Clic(Escenario& esc)
    {
        if (esc.seleccion < 0) return;
        const Elemento& e = esc.Elementos()[esc.seleccion];
        const std::string& n = e.etiqueta;

        if (n == "Krill") {
            if (krillMochila < 5) { krillMochila++; sonarRecoger = true; Aviso("Recogiste krill (" + std::to_string(krillMochila) + "/5). Es la base de toda la cadena.", glm::vec3(1.0f, 0.8f, 0.6f), 3.5f); }
            else Aviso("Tu bolsa de krill está llena. Dáselo a un animal.", glm::vec3(0.8f, 0.9f, 1.0f), 3.0f);
        }
        else if (n == "Pescado") {
            if (pescadoMochila < 5) { pescadoMochila++; sonarRecoger = true; Aviso("Recogiste pescado (" + std::to_string(pescadoMochila) + "/5).", glm::vec3(1.0f, 0.8f, 0.6f), 3.5f); }
            else Aviso("Tu bolsa de pescado está llena. Dáselo a un animal.", glm::vec3(0.8f, 0.9f, 1.0f), 3.0f);
        }
        else if (n == "Rompehielos") {
            barcoParado = 10.0f; sonarAlerta = true;
            Aviso("El rompehielos se detiene a observar. Sus motores y su hollín aceleran el deshielo: más barcos en el Ártico, menos hielo.", glm::vec3(1.0f, 0.85f, 0.55f), 6.0f);
        }
        else if (n == "Cueva de hielo") {
            Aviso("Las cuevas de hielo son refugios de osos polares. Si el hielo se derrite, pierden su guarida y su zona de caza.", glm::vec3(0.6f, 0.95f, 1.0f), 5.5f);
        }
        else if (n == "Plataforma de hielo" || n == "Iceberg") {
            temperatura = temperatura + 0.5f > T_MAX ? T_MAX : temperatura + 0.5f;
            Aviso("Calentaste el agua 0.5 °C: el hielo empieza a ceder. Así actúa el calentamiento global.", glm::vec3(1.0f, 0.8f, 0.5f), 4.5f);
        }
        else if (n == "Bacalao" || n == "Ballena azul") {
            Alimentar(e, n, true);
        }
        else if (n == "Foca" || n == "Pinguino" || n == "Morsa" || n == "Oso polar" || n == "Orca" || n == "Leon marino") {
            Alimentar(e, n, false);
        }
    }

    // ------------------------------------------------------------------ cada cuadro
    void Actualizar(float dt, GLFWwindow* ventana, Escenario& esc, const Elemento* sel)
    {
        if (dt > 0.1f) dt = 0.1f;

        // ===== ACTIVIDAD 1: termometro (no responde en modo edicion)
        if (!esc.modoEdicion) {
            if (glfwGetKey(ventana, GLFW_KEY_T) == GLFW_PRESS) temperatura += 1.0f * dt;
            if (glfwGetKey(ventana, GLFW_KEY_G) == GLFW_PRESS) temperatura -= 1.0f * dt;
        }
        temperatura = temperatura < T_MIN ? T_MIN : (temperatura > T_MAX ? T_MAX : temperatura);

        float objetivo = 1.0f - (temperatura + 1.0f) / 3.5f;
        objetivo = objetivo < 0.0f ? 0.0f : (objetivo > 1.0f ? 1.0f : objetivo);
        float vel = objetivo < hielo ? 0.5f : 0.3f;
        hielo += (objetivo - hielo) * (dt * vel > 1.0f ? 1.0f : dt * vel);
        if (hielo < 0.01f) hielo = 0.0f;
        if (hielo > 0.99f) hielo = 1.0f;

        // ===== ACTIVIDAD 2: fantasmas. g va de 0 (sano) a 1 (fantasma total) cuando el hielo baja de 65% a 15%
        float g = Suave((0.65f - hielo) / 0.5f);

        pob[0] += (hielo - pob[0]) * (dt * 0.5f > 1.0f ? 1.0f : dt * 0.5f);
        for (int i = 1; i < 4; i++) pob[i] += (pob[i - 1] - pob[i]) * (dt * 0.25f > 1.0f ? 1.0f : dt * 0.25f);

        float brillo = estadoFinal == 2 ? 0.25f + 0.15f * sinf(tReloj * 2.0f) : 0.0f;
        tReloj += dt;

        auto& elems = esc.Elementos();
        barcoParado = barcoParado > dt ? barcoParado - dt : 0.0f;
        for (size_t i = 0; i < elems.size() && i < base.size(); i++) {
            Elemento& e = elems[i];
            const Base& b = base[i];
            if (e.mov == MOV_BARCO) {
                if (barcoParado <= 0.0f) e.reloj += dt;
                float bx = esc.Posicion((int)i, esc.tiempoJuego).x;
                if (barcoXPrev > -500.0f && barcoXPrev < 0.0f && bx >= 0.0f) {   // cruzo el centro de la zona
                    float antes = temperatura;
                    temperatura = temperatura + 0.35f > T_MAX ? T_MAX : temperatura + 0.35f;
                    sonarAlerta = true;
                    Aviso(temperatura > antes ? "Pasó el rompehielos: abrió un canal y calentó el agua 0.35 °C. Haz clic en el barco para detenerlo." : "Pasó el rompehielos por el canal.", glm::vec3(1.0f, 0.85f, 0.55f), 6.5f);
                }
                barcoXPrev = bx;
            }
            if (b.tipo == 1) {
                e.escala = b.escala * (0.05f + 0.95f * hielo);
                e.visible = hielo > 0.03f;
                e.emision = b.emision + brillo;
            }
            else if (b.tipo == 2) {
                e.fantasma = 0.9f * g;
                e.alpha = b.alpha + (0.45f - b.alpha) * g;
                e.tinte = glm::mix(b.tinte, glm::vec3(0.65f, 0.85f, 1.25f), g);
                e.emision = b.emision + 0.35f * g;
                if (e.archivo == "osito") e.pos.y = b.posY - Suave((0.25f - hielo) / 0.25f) * 2.2f;
            }
            if (e.archivo == "Krill") e.presencia = 0.2f + 0.8f * Nivel(0);
            else if (e.archivo == "bacalao") e.presencia = 0.25f + 0.75f * Nivel(1);
            else if (e.archivo == "foca" || e.archivo == "pinguino") e.presencia = 0.34f + 0.66f * Nivel(2);
        }

        float derr = 1.0f - hielo;
        esc.cfg.niebla = glm::mix(esc.cfgBase.niebla, glm::vec3(0.27f, 0.33f, 0.36f), 0.5f * derr);
        esc.cfg.densidad = esc.cfgBase.densidad + 0.010f * derr;
        esc.cfg.solColor = esc.cfgBase.solColor * glm::mix(glm::vec3(1.0f), glm::vec3(1.08f, 0.98f, 0.88f), derr);
        esc.cfg.causticas = esc.cfgBase.causticas * (1.0f + 0.4f * derr);

        // ===== avisos educativos (cada uno sale una sola vez)
        if (!(hitos & 1) && temperatura >= T_CONGELA) {
            hitos |= 1; sonarAlerta = true;
            Aviso("¡Alerta! El agua superó los 0 °C: el hielo marino empieza a derretirse.", glm::vec3(1.0f, 0.75f, 0.4f), 7.0f);
        }
        if (hielo < 0.5f) objCalor = true;
        if (!(hitos & 2) && hielo < 0.5f) {
            hitos |= 2; sonarAlerta = true;
            Aviso("Se perdió la mitad del hielo. Las focas y los osos polares ya no tienen dónde descansar y cazar.", glm::vec3(1.0f, 0.8f, 0.5f), 8.0f);
        }
        if (!(hitos & 4) && g > 0.3f) {
            hitos |= 4; sonarAlerta = true;
            Aviso("Los animales que dependen del hielo se vuelven fantasmas: así se verá su extinción. Si enfrías el agua a tiempo, regresan.", glm::vec3(0.7f, 0.9f, 1.0f), 9.0f);
        }
        if (!(hitos & 8) && hielo < 0.4f && pob[0] < 0.5f) {
            hitos |= 8;
            Aviso("El krill necesita el hielo para crecer. Sin krill, caen los peces y después focas, pingüinos y ballenas: mira el panel de la cadena trófica.", glm::vec3(1.0f, 0.7f, 0.5f), 9.0f);
        }

        // ===== ACTIVIDAD 5: gran momento (hielo perdido / zona restaurada)
        if (estadoFinal == 0 && hielo <= 0.0f) {
            estadoFinal = 1; sonarAlerta = true;
            Aviso("¡El iceberg se parte y las plataformas desaparecen! Perdimos el hábitat. Enfría el agua (G) para intentar recuperarlo.", glm::vec3(1.0f, 0.45f, 0.45f), 10.0f);
        }
        else if (estadoFinal == 1 && hielo > 0.25f) {
            estadoFinal = 0;
            Aviso("El hielo se recupera. Aún hay esperanza si enfriamos a tiempo.", glm::vec3(0.5f, 1.0f, 0.6f), 6.0f);
        }
        if (estadoFinal != 2 && objCalor && objFantasma && (int)fotos.size() >= TOTAL_FOTOS && (int)alimentados.size() >= TOTAL_ALIMENTADOS && hielo >= 0.95f) {
            estadoFinal = 2; sonarDescubrir = true;
            Aviso("¡Aguas Frías restauradas! Completaste los 5 objetivos y el hielo volvió a brillar.", glm::vec3(0.5f, 1.0f, 0.6f), 10.0f);
        }
        if (!(hitos & 16) && (int)fotos.size() >= TOTAL_FOTOS) {
            hitos |= 16; sonarDescubrir = true;
            Aviso("¡Álbum completo! Registraste todas las especies de las aguas frías.", glm::vec3(0.55f, 0.95f, 1.0f), 6.0f);
        }

        flash -= dt * 2.5f; if (flash < 0.0f) flash = 0.0f;
        marco -= dt; if (marco < 0.0f) marco = 0.0f;

        for (size_t i = 0; i < avisos.size(); ) {
            avisos[i].vida -= dt;
            if (avisos[i].vida <= 0.0f) avisos.erase(avisos.begin() + i); else i++;
        }
    }

    std::string Detalle(const Elemento& e, glm::vec3& color) const
    {
        char b[96];
        if (e.archivo == "hielo1" || e.archivo == "hielo2" || e.archivo == "iceberg" || e.archivo == "iceberg_meshy") {
            color = hielo > 0.5f ? glm::vec3(0.6f, 0.95f, 1.0f) : glm::vec3(1.0f, 0.7f, 0.5f);
            snprintf(b, sizeof(b), "Hielo restante: %d%%  ·  Clic: calentar el agua", (int)(hielo * 100.0f + 0.5f));
            return b;
        }
        if (EsFantasma(e.archivo)) {
            std::string pista = "  ·  X: foto";
            if (e.archivo == "ballena") pista += "  ·  Clic: dar de comer";
            else if (e.archivo != "Narwhal2" && e.archivo != "Cachalote2") pista += "  ·  Clic: dar pescado";
            if (e.fantasma > 0.5f) { color = glm::vec3(0.7f, 0.85f, 1.0f); return "Estado: fantasma (sin hábitat)  ·  X: foto"; }
            if (e.fantasma > 0.05f) { color = glm::vec3(1.0f, 0.8f, 0.5f); return "Estado: perdiendo su hábitat" + pista; }
            color = glm::vec3(0.5f, 1.0f, 0.6f); return "Estado: sano" + pista;
        }
        color = glm::vec3(1.0f, 0.85f, 0.6f);
        if (e.archivo == "rompehielos") return "Clic: detener el barco  ·  su paso calienta el agua";
        if (e.archivo == "cueva_meshy") return "Refugio de osos polares";
        if (e.archivo == "Krill") return "Clic: recoger krill  ·  X: foto";
        if (e.archivo == "pescado") return "Clic: recoger pescado";
        if (e.archivo == "bacalao") return "Clic: dar de comer (krill)  ·  X: foto";
        if (e.archivo == "beluga" || e.archivo == "morsa" || e.archivo == "Calamar2" || e.archivo == "pezdracoide") return "X: foto";
        return "";
    }

    // ------------------------------------------------------------------ HUD
    void DibujarHUD(MenuInicio& ui, GLFWwindow* ventana, float t, float tiempoEnZona)
    {
        ui.Comenzar(ventana);
        float s = ui.Escala(), W = ui.Ancho(), H = ui.Alto();
        char b[96];
        glm::vec3 cTemp = ColorTemperatura(temperatura);

        // ---------------- termometro (derecha)
        float pw = 150.0f * s, ph = 430.0f * s, px = W - pw - 18.0f * s, py = 96.0f * s;
        if (temperatura >= T_CONGELA) ui.Resplandor(glm::vec4(px - 14 * s, py - 14 * s, pw + 28 * s, ph + 28 * s), glm::vec4(cTemp, 0.25f + 0.2f * sinf(t * 5.0f)), 18 * s, 14 * s);
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
        for (int k = -2; k <= 4; k += 2) {
            float y = ty + th * (1.0f - (k - T_MIN) / (T_MAX - T_MIN));
            ui.Rect(glm::vec4(tx - tw * 0.5f - 14 * s, y - 1 * s, 9 * s, 2 * s), glm::vec4(1, 1, 1, 0.7f));
            snprintf(b, sizeof(b), "%d", k);
            ui.Texto(b, tx - tw * 0.5f - 18 * s, y - 8 * s, 13 * s, glm::vec4(0.85f, 0.92f, 1.0f, 0.9f), 2, false);
        }
        float yu = ty + th * (1.0f - (T_CONGELA - T_MIN) / (T_MAX - T_MIN));
        ui.Rect(glm::vec4(tx - tw * 0.5f - 5 * s, yu - 1.5f * s, tw + 10 * s, 3 * s), glm::vec4(0.7f, 0.9f, 1.0f, 1.0f));
        ui.Texto("Congela", tx + tw * 0.5f + 9 * s, yu - 16 * s, 12 * s, glm::vec4(0.7f, 0.9f, 1.0f, 1.0f));
        ui.Texto("0 °C", tx + tw * 0.5f + 9 * s, yu - 2 * s, 12 * s, glm::vec4(0.7f, 0.9f, 1.0f, 1.0f));
        ui.Texto("T  calentar   G  enfriar", px + pw * 0.5f, py + ph - 42 * s, 12 * s, glm::vec4(0.85f, 0.92f, 1.0f, 0.95f), 1, false);
        ui.Texto("X  foto   R  reiniciar", px + pw * 0.5f, py + ph - 24 * s, 12 * s, glm::vec4(0.85f, 0.92f, 1.0f, 0.95f), 1, false);

        // ---------------- cadena trofica (izquierda, arriba)
        float cw = 270.0f * s, ch = 168.0f * s, cx = 18.0f * s, cy = 96.0f * s;
        ui.Rect(glm::vec4(cx, cy, cw, ch), glm::vec4(0.01f, 0.07f, 0.12f, 0.75f), 14.0f * s);
        ui.Texto("CADENA TRÓFICA", cx + 16 * s, cy + 12 * s, 13 * s, glm::vec4(0.55f, 0.9f, 1.0f, 1.0f));
        const char* nombres[4] = { "Krill", "Bacalao", "Focas y pingüinos", "Ballena y oso polar" };
        for (int i = 0; i < 4; i++) {
            float v = Nivel(i);
            glm::vec3 c = v > 0.66f ? glm::vec3(0.4f, 0.95f, 0.55f) : (v > 0.33f ? glm::vec3(1.0f, 0.8f, 0.35f) : glm::vec3(1.0f, 0.45f, 0.4f));
            float yy = cy + 38 * s + i * 31 * s;
            ui.Texto(nombres[i], cx + 16 * s, yy, 13 * s, glm::vec4(0.95f, 0.95f, 0.95f, 1.0f), 0, false);
            float bw = cw - 32 * s;
            ui.Rect(glm::vec4(cx + 16 * s, yy + 16 * s, bw, 7 * s), glm::vec4(1, 1, 1, 0.15f), 3.5f * s);
            ui.Rect(glm::vec4(cx + 16 * s, yy + 16 * s, bw * (v < 0.02f ? 0.02f : v), 7 * s), glm::vec4(c, 1.0f), 3.5f * s);
        }

        // ---------------- objetivos (abajo a la izquierda): uno por actividad
        float bw = 330.0f * s, bh = 200.0f * s, bx = 18.0f * s, by = H - bh - 18.0f * s;
        ui.Rect(glm::vec4(bx, by, bw, bh), glm::vec4(0.01f, 0.07f, 0.12f, 0.75f), 14.0f * s);
        ui.Texto("OBJETIVOS", bx + 16 * s, by + 12 * s, 13 * s, glm::vec4(0.55f, 0.9f, 1.0f, 1.0f));
        glm::vec3 cH = hielo > 0.66f ? glm::vec3(0.55f, 0.9f, 1.0f) : (hielo > 0.33f ? glm::vec3(1.0f, 0.8f, 0.35f) : glm::vec3(1.0f, 0.45f, 0.4f));
        float barW = bw - 32 * s - 52 * s;
        ui.Rect(glm::vec4(bx + 16 * s, by + 36 * s, barW, 14 * s), glm::vec4(1, 1, 1, 0.15f), 7 * s);
        ui.Rect(glm::vec4(bx + 16 * s, by + 36 * s, barW * (hielo < 0.02f ? 0.02f : hielo), 14 * s), glm::vec4(cH, 1.0f), 7 * s);
        snprintf(b, sizeof(b), "%d%%", (int)(hielo * 100.0f + 0.5f));
        ui.Texto(b, bx + bw - 16 * s, by + 31 * s, 19 * s, glm::vec4(cH, 1.0f), 2);
        ui.Texto("Hielo restante", bx + 16 * s, by + 52 * s, 11 * s, glm::vec4(0.7f, 0.85f, 0.95f, 0.9f), 0, false);
        bool o1 = objCalor, o2 = objFantasma, o3 = (int)fotos.size() >= TOTAL_FOTOS, o4 = (int)alimentados.size() >= TOTAL_ALIMENTADOS;
        bool o5 = o1 && o2 && o3 && o4 && hielo >= 0.95f;
        glm::vec4 cOk(0.5f, 1.0f, 0.6f, 1.0f), cNo(0.95f, 0.95f, 0.95f, 1.0f);
        float y0 = by + 72 * s, dy = 22 * s;
        ui.Texto(o1 ? "[x] Calienta (T) y derrite la mitad del hielo" : "[ ] Calienta (T) y derrite la mitad del hielo", bx + 16 * s, y0, 14 * s, o1 ? cOk : cNo, 0, false);
        ui.Texto(o2 ? "[x] Fotografía a un animal fantasma (X)" : "[ ] Fotografía a un animal fantasma (X)", bx + 16 * s, y0 + dy, 14 * s, o2 ? cOk : cNo, 0, false);
        snprintf(b, sizeof(b), "%s Fotografía las 14 especies: %d/%d", o3 ? "[x]" : "[ ]", (int)fotos.size(), TOTAL_FOTOS);
        ui.Texto(b, bx + 16 * s, y0 + 2 * dy, 14 * s, o3 ? cOk : cNo, 0, false);
        snprintf(b, sizeof(b), "%s Alimenta a 4 animales: %d/%d", o4 ? "[x]" : "[ ]", (int)alimentados.size(), TOTAL_ALIMENTADOS);
        ui.Texto(b, bx + 16 * s, y0 + 3 * dy, 14 * s, o4 ? cOk : cNo, 0, false);
        ui.Texto(o5 ? "[x] Enfría (G) y restaura el hielo al 95 %" : "[ ] Enfría (G) y restaura el hielo al 95 %", bx + 16 * s, y0 + 4 * dy, 14 * s, o5 ? cOk : cNo, 0, false);
        snprintf(b, sizeof(b), "Bolsa: krill %d · pescado %d", krillMochila, pescadoMochila);
        ui.Texto(b, bx + 16 * s, y0 + 5 * dy + 2 * s, 12 * s, glm::vec4(1.0f, 0.85f, 0.6f, 1.0f), 0, false);

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

        // ---------------- ayuda de controles (primeros 20 segundos)
        float ayuda = tiempoEnZona < 18.0f ? 1.0f : (tiempoEnZona < 20.0f ? (20.0f - tiempoEnZona) / 2.0f : 0.0f);
        if (ayuda > 0.0f) {
            float x0 = bx + bw + 18 * s, x1 = px - 18 * s, aw = x1 - x0;
            const char* l1 = "WASD nadar  ·  Espacio / Ctrl subir y bajar  ·  Shift aletear  ·  Clic ver ficha  ·  Esc menú";
            const char* l2 = "T / G temperatura  ·  X foto  ·  Clic en krill o pescado para recogerlos y en un animal para alimentarlo";
            std::vector<std::string> a1 = ui.Lineas(l1, aw - 28 * s, 14 * s, false), a2 = ui.Lineas(l2, aw - 28 * s, 14 * s, true);
            float lh = 14 * s * 1.35f, ah = (a1.size() + a2.size()) * lh + 20 * s, ay2 = H - 18 * s - ah;
            ui.Rect(glm::vec4(x0, ay2, aw, ah), glm::vec4(0.01f, 0.07f, 0.12f, 0.72f * ayuda), 12 * s);
            float yy = ay2 + 10 * s;
            for (auto& l : a1) { ui.Texto(l, x0 + aw * 0.5f, yy, 14 * s, glm::vec4(0.88f, 0.94f, 1.0f, ayuda), 1, false); yy += lh; }
            for (auto& l : a2) { ui.Texto(l, x0 + aw * 0.5f, yy, 14 * s, glm::vec4(1.0f, 0.8f, 0.5f, ayuda), 1, true); yy += lh; }
        }

        // ---------------- marco de camara y flash (ACTIVIDAD 3)
        if (marco > 0.0f) {
            float a = marco > 0.3f ? 1.0f : marco / 0.3f;
            float mx = W * 0.28f, my = H * 0.22f, mw = W * 0.44f, mh = H * 0.56f, L = 38 * s, g2 = 3 * s;
            glm::vec4 c(1, 1, 1, 0.9f * a);
            ui.Rect(glm::vec4(mx, my, L, g2), c);           ui.Rect(glm::vec4(mx, my, g2, L), c);
            ui.Rect(glm::vec4(mx + mw - L, my, L, g2), c);  ui.Rect(glm::vec4(mx + mw - g2, my, g2, L), c);
            ui.Rect(glm::vec4(mx, my + mh - g2, L, g2), c); ui.Rect(glm::vec4(mx, my + mh - L, g2, L), c);
            ui.Rect(glm::vec4(mx + mw - L, my + mh - g2, L, g2), c); ui.Rect(glm::vec4(mx + mw - g2, my + mh - L, g2, L), c);
            ui.Texto("FOTO", mx + 12 * s, my + 12 * s, 14 * s, glm::vec4(1.0f, 0.4f, 0.4f, a), 0);
        }
        if (flash > 0.0f) ui.Rect(glm::vec4(0, 0, W, H), glm::vec4(1.0f, 1.0f, 1.0f, 0.75f * flash));

        // ---------------- ACTIVIDAD 5: pantallas del gran momento
        if (estadoFinal == 1) {
            ui.Rect(glm::vec4(0, 0, W, H), glm::vec4(0.35f, 0.02f, 0.02f, 0.18f + 0.06f * sinf(t * 3.0f)));
            ui.Texto("PERDIMOS EL HÁBITAT", W * 0.5f, H * 0.40f, 46 * s, glm::vec4(1.0f, 0.5f, 0.45f, 1.0f), 1);
            ui.Texto("Sin hielo, los gigantes del mar se vuelven fantasmas. Enfría el agua (G) para recuperarlo.", W * 0.5f, H * 0.40f + 60 * s, 18 * s, glm::vec4(1.0f, 0.88f, 0.85f, 1.0f), 1, false);
        }
        else if (estadoFinal == 2) {
            ui.Resplandor(glm::vec4(W * 0.5f - 330 * s, H * 0.40f - 10 * s, 660 * s, 70 * s), glm::vec4(0.5f, 1.0f, 0.8f, 0.25f + 0.1f * sinf(t * 2.0f)), 20 * s, 20 * s);
            ui.Texto("AGUAS FRÍAS RESTAURADAS", W * 0.5f, H * 0.40f, 42 * s, glm::vec4(0.6f, 1.0f, 0.85f, 1.0f), 1);
            ui.Texto("El hielo volvió a brillar. Así de rápido puede cambiar un ecosistema... en los dos sentidos.", W * 0.5f, H * 0.40f + 56 * s, 17 * s, glm::vec4(0.9f, 1.0f, 0.95f, 1.0f), 1, false);
        }
        ui.Terminar();
    }

private:
    static const int TOTAL_FOTOS = 14, TOTAL_ALIMENTADOS = 4;

    struct AvisoHUD { std::string texto; glm::vec3 color; float vida, total; };
    struct Base { float escala, alpha, emision; glm::vec3 tinte; int tipo; float posY; };
    std::vector<AvisoHUD> avisos;
    std::vector<Base> base;
    int hitos = 0;
    std::set<std::string> fotos, alimentados;
    int krillMochila = 0, pescadoMochila = 0;
    float pob[4] = { 1, 1, 1, 1 };
    float apoyo[4] = { 0, 0, 0, 0 };
    float flash = 0.0f, marco = 0.0f, tReloj = 0.0f;
    bool objCalor = false, objFantasma = false;
    float barcoParado = 0.0f, barcoXPrev = -999.0f;

    float Suave(float x) const { x = x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); return x * x * (3.0f - 2.0f * x); }
    float Nivel(int i) const { float v = pob[i] + apoyo[i]; return v > 1.0f ? 1.0f : v; }

    void Aviso(const std::string& texto, glm::vec3 color, float segundos)
    {
        avisos.push_back({ texto, color, segundos, segundos });
        if (avisos.size() > 4) avisos.erase(avisos.begin());
    }

    static bool EsFantasma(const std::string& a)
    {
        return a == "ballena" || a == "foca" || a == "pinguino" || a == "osito" || a == "morsa" || a == "Orca2" || a == "Narwhal2" || a == "Cachalote2" || a == "LeonMarino2";
    }

    void TomarFoto(Escenario& esc)
    {
        flash = 1.0f; marco = 1.2f;
        const Elemento* s = esc.Seleccionado();
        const char* especies[14] = { "Ballena azul", "Beluga", "Foca", "Pinguino", "Oso polar", "Morsa", "Krill", "Bacalao", "Orca", "Narval", "Cachalote", "Calamar colosal", "Leon marino", "Pez dracoide" };
        bool esEspecie = false;
        if (s) for (int i = 0; i < 14; i++) if (s->etiqueta == especies[i]) esEspecie = true;
        if (!esEspecie) { Aviso("Apunta con la mira a un animal y presiona X para fotografiarlo.", glm::vec3(0.8f, 0.9f, 1.0f), 4.0f); return; }
        const Ficha* f = BuscarFicha(s->etiqueta);
        std::string nombre = f ? f->nombre : s->etiqueta;
        if (s->fantasma > 0.5f && !objFantasma) objFantasma = true;
        if (fotos.count(s->etiqueta)) { Aviso("Ya tienes una foto de esta especie: " + nombre + ".", glm::vec3(0.8f, 0.9f, 1.0f), 3.5f); return; }
        fotos.insert(s->etiqueta); sonarDescubrir = true;
        if (s->fantasma > 0.5f)
            Aviso("Último registro de la especie: " + nombre + ". Así se vería si desaparece su hábitat.", glm::vec3(0.7f, 0.85f, 1.0f), 6.0f);
        else
            Aviso("Foto guardada: " + nombre + " (" + std::to_string((int)fotos.size()) + "/" + std::to_string(TOTAL_FOTOS) + ")", glm::vec3(0.55f, 0.95f, 1.0f), 4.0f);
    }

    void Alimentar(const Elemento& e, const std::string& n, bool conKrill)
    {
        if (e.fantasma > 0.5f) { Aviso("Es un fantasma: ya no puede comer. Enfría el agua para que regrese.", glm::vec3(0.7f, 0.85f, 1.0f), 4.5f); return; }
        int& bolsa = conKrill ? krillMochila : pescadoMochila;
        if (bolsa <= 0) { Aviso(conKrill ? "Necesita krill. Recoge krill primero (clic en los crustáceos)." : "Necesita pescado. Recoge pescado primero.", glm::vec3(1.0f, 0.85f, 0.6f), 4.0f); return; }
        bolsa--; sonarRecoger = true;
        int nivel = n == "Bacalao" ? 1 : 2;
        if (n == "Ballena azul" || n == "Oso polar") nivel = 3;
        apoyo[nivel] = apoyo[nivel] + 0.08f > 0.2f ? 0.2f : apoyo[nivel] + 0.08f;
        bool nuevo = alimentados.insert(n).second;
        const Ficha* f = BuscarFicha(n);
        std::string nombre = f ? f->nombre : n;
        Aviso("Alimentaste a: " + nombre + (nuevo ? " (" + std::to_string((int)alimentados.size()) + "/" + std::to_string(TOTAL_ALIMENTADOS) + ")" : "") + ". Cada nivel de la cadena depende del anterior.", glm::vec3(0.55f, 1.0f, 0.7f), 5.0f);
    }

    glm::vec3 ColorTemperatura(float T) const
    {
        glm::vec3 frio(0.35f, 0.75f, 1.0f), hielo2(0.80f, 0.95f, 1.0f), calido(1.0f, 0.75f, 0.3f), caliente(1.0f, 0.35f, 0.3f);
        if (T < 0.0f) return glm::mix(frio, hielo2, (T + 2.0f) / 2.0f);
        if (T < 2.0f) return glm::mix(hielo2, calido, T / 2.0f);
        float k = (T - 2.0f) / 2.0f;
        return glm::mix(calido, caliente, k > 1.0f ? 1.0f : k);
    }
};

#endif
