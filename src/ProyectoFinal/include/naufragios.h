#ifndef NAUFRAGIOS_H
#define NAUFRAGIOS_H
/*
* Zona 3 - Naufragios: huella humana y contaminacion.
*
*  - Limpieza: cada residuo recogido (clic con la mira) aclara el agua: baja la
*    turbidez, entra mas luz del sol y aparecen las causticas.
*  - Pez leon (invasor): se retira con clic; al hacerlo regresan los peces nativos.
*  - Red fantasma: mantener presionado el clic corta la red y libera a la tortuga,
*    que escapa con una animacion por cuadros clave.
*  - Barril toxico: al retirarlo desaparece la nube verde.
*  - Cangrejo ermitano: deja su lata por una concha natural cuando el fondo esta limpio.
*  - Dron guia: acompana al buzo y senala la basura mas cercana.
*  - Cofre: se abre (cuadros clave) cuando el fondo queda limpio y sin invasores.
*/
#include <string>
#include <vector>
#include <set>
#include <cstdio>
#include <cmath>
#include <zonas.h>
#include <menu.h>
#include <fichas.h>

class MecanicaNaufragios
{
public:
    float limpieza = 0.0f;                 // 0..1 (por peso de los residuos)
    int recogidos[5] = { 0 }, totales[5] = { 0 };
    int leonesRetirados = 0, leonesTotal = 0;
    bool tortugaLibre = false, cofreAbierto = false;
    // eventos de sonido (ProyectoFinal los reproduce y los apaga)
    bool sonarRecoger = false, sonarAlerta = false, sonarDescubrir = false;

    void Reiniciar(Escenario& esc, float t)
    {
        limpieza = 0.0f; pesoRecogido = 0.0f; pesoTotal = 0.0f; hitos = 0; corte = 0.0f;
        leonesRetirados = 0; leonesTotal = 0; tortugaLibre = false; cofreAbierto = false; tiempoLibre = -1.0f; barrilRetirado = false;
        for (int i = 0; i < 5; i++) avisoTipo[i] = false;
        for (int i = 0; i < 5; i++) recogidos[i] = totales[i] = 0;
        avisos.clear(); registradas.clear(); ultimaMira.clear(); tiempoMira = 0.0f;
        iDron = iRed = iTortuga = iBarril = iNube = iCLata = iCNat = iTapa = iMonedas = iColumna = iSub = -1;
        std::set<std::string> especies;
        auto& el = esc.Elementos();
        for (int i = 0; i < (int)el.size(); i++) {
            Elemento& e = el[i];
            if (e.residuo > 0) { totales[e.residuo]++; pesoTotal += e.peso; }
            if (e.invasor) leonesTotal++;
            if (e.archivo == "dron") iDron = i;
            else if (e.archivo == "red_fantasma") iRed = i;
            else if (e.etiqueta == "Tortuga atrapada") iTortuga = i;
            else if (e.archivo == "barril") iBarril = i;
            else if (e.archivo == "burbuja" && e.pulsar) iNube = i;
            else if (e.archivo == "cangrejo_lata") iCLata = i;
            else if (e.archivo == "cangrejoermitano" && !e.visible) iCNat = i;
            else if (e.archivo == "cofre_tapa") iTapa = i;
            else if (e.archivo == "moneda") iMonedas = i;
            else if (e.archivo == "columna" && e.mov == MOV_KF) iColumna = i;
            else if (e.archivo == "submarino") iSub = i;
            const Ficha* f = BuscarFicha(e.etiqueta);
            if (f && f->bitacora) especies.insert(e.etiqueta);
        }
        especies.insert("Tortuga marina");      // se registra al liberarla
        totalEspecies = (int)especies.size();
        if (iSub >= 0) el[iSub].kfInicio = t + 1.5f;   // el submarino termina de hundirse frente al buzo
        Aviso("El fondo está invadido por basura y el agua está turbia. Lee el letrero frente a ti y empieza a limpiar: apunta a un residuo y haz clic.",
              glm::vec3(0.55f, 0.9f, 1.0f), 9.0f);
    }

    // Clic sobre el elemento 'idx' (o -1 si la mira no apunta a nada)
    void Clic(int idx, Escenario& esc, float t)
    {
        if (idx < 0) return;
        Elemento& e = esc.Elementos()[idx];
        if (!e.visible) return;
        if (e.residuo > 0 && e.residuo != 4) {
            e.visible = false; recogidos[e.residuo]++; pesoRecogido += e.peso; sonarRecoger = true;
            if (idx == iBarril && iNube >= 0) esc.Elementos()[iNube].visible = false;
            PrimerResiduo(e.residuo, e.etiqueta);
            esc.seleccion = -1;
        }
        else if (e.invasor) {
            e.visible = false; leonesRetirados++; sonarRecoger = true; esc.seleccion = -1;
            if (leonesRetirados == 1)
                Aviso("Retiraste un pez león. Un solo pez león puede reducir hasta 79% los peces juveniles de un arrecife en pocas semanas.", glm::vec3(1.0f, 0.75f, 0.4f), 8.0f);
            else if (leonesRetirados == leonesTotal)
                Aviso("¡Retiraste a todos los peces león! Sin el invasor, los peces nativos vuelven a crecer.", glm::vec3(0.5f, 1.0f, 0.6f), 7.0f);
            else {
                char b[96]; snprintf(b, sizeof(b), "Pez león retirado (%d de %d).", leonesRetirados, leonesTotal);
                Aviso(b, glm::vec3(1.0f, 0.8f, 0.5f), 3.0f);
            }
        }
        else if ((idx == iRed || idx == iTortuga) && !tortugaLibre)
            Aviso("Mantén presionado el clic sobre la red para cortarla.", glm::vec3(1.0f, 0.75f, 0.5f), 3.0f);
        else if (idx == iTapa || e.archivo == "cofre") {
            if (!cofreAbierto) {
                char b[160]; snprintf(b, sizeof(b), "El cofre está cerrado. Se abrirá cuando el fondo esté limpio (%d%%) y sin peces león (%d de %d).",
                                      (int)(limpieza * 100.0f + 0.5f), leonesRetirados, leonesTotal);
                Aviso(b, glm::vec3(1.0f, 0.85f, 0.45f), 5.0f);
            }
        }
    }

    void Actualizar(float dt, float t, GLFWwindow* ventana, Escenario& esc, const Camera& cam, const Elemento* sel)
    {
        if (dt > 0.1f) dt = 0.1f;
        auto& el = esc.Elementos();

        // --- cortar la red fantasma (clic sostenido)
        bool sobreRed = sel && !tortugaLibre && ((iRed >= 0 && sel == &el[iRed]) || (iTortuga >= 0 && sel == &el[iTortuga]));
        if (sobreRed && !esc.modoEdicion && glfwGetMouseButton(ventana, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) corte += dt / 1.8f;
        else corte -= dt * 0.8f;
        corte = corte < 0.0f ? 0.0f : corte;
        if (corte >= 1.0f && !tortugaLibre && iRed >= 0) Liberar(esc, t);
        if (iTortuga >= 0 && !tortugaLibre) {   // la tortuga forcejea en la red
            el[iTortuga].rotExtra = glm::vec3(8.0f * sinf(t * 2.7f), 0.0f, 12.0f * sinf(t * 3.4f));
            el[iTortuga].ondula = 0.04f;
        }
        if (tortugaLibre && tiempoLibre >= 0.0f && t - tiempoLibre > 7.2f && iTortuga >= 0) {
            Elemento& tg = el[iTortuga];       // al terminar su escape (KF) nada libre en circulos
            glm::vec3 p = esc.Posicion(iTortuga, t);
            tg.mov = MOV_ORBITA; tg.pos = p - glm::vec3(9.0f, 0.0f, 0.0f); tg.radio = 9.0f; tg.vel = 0.12f; tg.fase = 0.0f;
            tg.oscAmp = 0.8f; tg.oscVel = 0.4f; tg.baseY = 0.0f; tg.sobreSuelo = false; tg.rotExtra = glm::vec3(0.0f);
            tiempoLibre = -1.0f;
        }

        // --- limpieza y respuesta del agua
        limpieza = pesoTotal > 0.0f ? pesoRecogido / pesoTotal : 0.0f;
        float L = powf(limpieza, 0.85f);
        const ZonaConfig& b = esc.cfgBase;
        esc.cfg.niebla = glm::mix(b.niebla, glm::vec3(0.05f, 0.30f, 0.38f), L);
        esc.cfg.densidad = glm::mix(b.densidad, 0.014f, L);
        esc.cfg.solColor = glm::mix(b.solColor, glm::vec3(0.95f, 1.0f, 0.95f) * 1.15f, L);
        esc.cfg.ambArriba = glm::mix(b.ambArriba, glm::vec3(0.32f, 0.50f, 0.55f), L);
        esc.cfg.ambAbajo = glm::mix(b.ambAbajo, glm::vec3(0.10f, 0.18f, 0.20f), L);
        esc.cfg.causticas = glm::mix(b.causticas, 0.6f, L);
        if (iNube >= 0 && el[iNube].visible) el[iNube].alpha = 0.26f + 0.06f * sinf(t * 1.3f);

        // --- fauna: los nativos regresan al retirar invasores y limpiar
        float fl = leonesTotal ? (float)leonesRetirados / leonesTotal : 1.0f;
        for (auto& e : el)
            if (e.mov == MOV_CARDUMEN && (e.etiqueta == "Roncador" || e.etiqueta == "Pez cirujano azul"))
                e.presencia = 0.15f + 0.55f * fl + 0.30f * limpieza;

        // --- el cangrejo cambia su lata por una concha natural
        if (!(hitos & 64) && limpieza >= 0.6f && iCLata >= 0 && iCNat >= 0) {
            hitos |= 64; el[iCLata].visible = false; el[iCNat].visible = true; sonarDescubrir = true;
            Aviso("El cangrejo ermitaño encontró una concha natural y dejó la lata: con menos basura, recupera su refugio.", glm::vec3(0.5f, 1.0f, 0.6f), 7.0f);
        }

        // --- la columna inestable se derrumba cuando el buzo se acerca
        if (iColumna >= 0 && el[iColumna].kfInicio > 1e5f && glm::length(cam.Position - glm::vec3(-5.5f, cam.Position.y, 7.0f)) < 11.0f) {
            el[iColumna].kfInicio = t; sonarAlerta = true;
            Aviso("¡Cuidado! Una columna de las ruinas se derrumba: las estructuras hundidas son inestables.", glm::vec3(1.0f, 0.7f, 0.4f), 5.0f);
        }

        // --- avisos de progreso
        if (!(hitos & 1) && limpieza >= 0.25f) { hitos |= 1; Aviso("25% limpio: el agua empieza a aclararse. Menos residuos, más luz.", glm::vec3(0.6f, 0.95f, 1.0f), 6.0f); }
        if (!(hitos & 2) && limpieza >= 0.50f) { hitos |= 2; sonarDescubrir = true; Aviso("La mitad del fondo está limpia: regresan los peces nativos.", glm::vec3(0.6f, 0.95f, 1.0f), 6.0f); }
        if (!(hitos & 4) && limpieza >= 0.75f) { hitos |= 4; Aviso("75% limpio: ya casi no necesitas la linterna (tecla F).", glm::vec3(0.6f, 0.95f, 1.0f), 6.0f); }
        if (!(hitos & 8) && limpieza >= 0.999f) { hitos |= 8; sonarDescubrir = true;
            Aviso(leonesRetirados < leonesTotal ? "¡Retiraste toda la basura! Retira también a los peces león para abrir el cofre."
                                                : "¡Fondo limpio!", glm::vec3(0.5f, 1.0f, 0.6f), 7.0f); }

        // --- cofre: se abre cuando todo esta limpio y sin invasores
        if (!cofreAbierto && limpieza >= 0.999f && leonesRetirados >= leonesTotal) {
            cofreAbierto = true; sonarDescubrir = true;
            if (iTapa >= 0) el[iTapa].kfInicio = t;
            if (iMonedas >= 0) el[iMonedas].presencia = 1.0f;
            Aviso("¡Se abrió el cofre! El verdadero tesoro es un océano limpio: retiraste toda la basura, liberaste a la tortuga y controlaste a la especie invasora.",
                  glm::vec3(1.0f, 0.85f, 0.4f), 12.0f);
        }

        // --- dron guia: sigue al buzo y apunta a la basura mas cercana
        objetivo = -1; float mejor = 1e9f;
        for (int i = 0; i < (int)el.size(); i++) {
            const Elemento& e = el[i];
            if (!e.visible || !(e.residuo > 0 || e.invasor) || (e.residuo == 4 && tortugaLibre)) continue;
            float d = glm::length(esc.Posicion(i, t) - cam.Position);
            if (d < mejor) { mejor = d; objetivo = i; }
        }
        distObjetivo = mejor;
        if (objetivo >= 0) posObjetivo = esc.Posicion(objetivo, t);
        if (iDron >= 0) {
            Elemento& d = el[iDron];
            glm::vec3 meta = cam.Position + cam.Front * 3.0f - cam.Right * 1.15f + glm::vec3(0.0f, 0.35f, 0.0f);   // a la izquierda, sin tapar el HUD
            d.pos += (meta - d.pos) * (dt * 1.8f > 1.0f ? 1.0f : dt * 1.8f);
            glm::vec3 mira = objetivo >= 0 ? posObjetivo - d.pos : cam.Front;
            float yawMeta = glm::degrees(atan2f(-mira.z, mira.x));
            float dy = yawMeta - d.yaw; while (dy > 180.0f) dy -= 360.0f; while (dy < -180.0f) dy += 360.0f;
            d.yaw += dy * (dt * 2.0f > 1.0f ? 1.0f : dt * 2.0f);
        }

        // --- bitacora
        if (sel) {
            if (sel->etiqueta == ultimaMira) tiempoMira += dt; else { ultimaMira = sel->etiqueta; tiempoMira = 0.0f; }
            const Ficha* f = BuscarFicha(sel->etiqueta);
            if (f && f->bitacora && tiempoMira > 0.25f && !registradas.count(sel->etiqueta)) {
                registradas.insert(sel->etiqueta); sonarDescubrir = true;
                Aviso("Nueva especie en tu bitácora: " + f->nombre, glm::vec3(0.55f, 0.95f, 1.0f), 3.5f);
            }
        }
        else ultimaMira.clear();

        for (size_t i = 0; i < avisos.size(); ) {
            avisos[i].vida -= dt;
            if (avisos[i].vida <= 0.0f) avisos.erase(avisos.begin() + i); else i++;
        }
    }

    // Texto de accion para la ficha bajo la mira
    std::string Detalle(const Elemento& e, glm::vec3& color) const
    {
        if (e.residuo > 0 && e.residuo != 4) { color = glm::vec3(1.0f, 0.7f, 0.5f); return "Clic: recoger"; }
        if (e.invasor) { color = glm::vec3(1.0f, 0.8f, 0.4f); return "Clic: retirar al invasor"; }
        if (e.etiqueta == "Red fantasma" || e.etiqueta == "Tortuga atrapada") {
            if (tortugaLibre) return "";
            color = glm::vec3(1.0f, 0.75f, 0.5f); return "Mantén presionado el clic para cortar la red";
        }
        if (e.etiqueta == "Cofre del tesoro") { color = glm::vec3(1.0f, 0.85f, 0.4f); return cofreAbierto ? "Abierto" : "Cerrado: limpia el fondo para abrirlo"; }
        return "";
    }

    void DibujarHUD(MenuInicio& ui, GLFWwindow* ventana, float t, float tiempoEnZona, const glm::mat4& proj, const glm::mat4& view)
    {
        ui.Comenzar(ventana);
        float s = ui.Escala(), W = ui.Ancho(), H = ui.Alto();
        char b[128];

        // ---------------- marcador del dron: basura mas cercana
        if (objetivo >= 0) {
            glm::vec4 clip = proj * view * glm::vec4(posObjetivo, 1.0f);
            bool delante = clip.w > 0.01f;
            glm::vec2 ndc = delante ? glm::vec2(clip) / clip.w : -glm::vec2(clip.x, clip.y);
            glm::vec2 px((ndc.x * 0.5f + 0.5f) * W, (1.0f - (ndc.y * 0.5f + 0.5f)) * H);
            float m = 60.0f * s;
            snprintf(b, sizeof(b), "%.0f m", distObjetivo);
            glm::vec4 col(1.0f, 0.8f, 0.35f, 0.95f);
            if (delante && px.x > m && px.x < W - m && px.y > m && px.y < H - m) {
                float r = 16.0f * s + 4.0f * s * sinf(t * 4.0f), g = 2.0f * s, l = 7.0f * s;
                ui.Rect(glm::vec4(px.x - r, px.y - r, l, g), col); ui.Rect(glm::vec4(px.x - r, px.y - r, g, l), col);
                ui.Rect(glm::vec4(px.x + r - l, px.y - r, l, g), col); ui.Rect(glm::vec4(px.x + r - g, px.y - r, g, l), col);
                ui.Rect(glm::vec4(px.x - r, px.y + r - g, l, g), col); ui.Rect(glm::vec4(px.x - r, px.y + r - l, g, l), col);
                ui.Rect(glm::vec4(px.x + r - l, px.y + r - g, l, g), col); ui.Rect(glm::vec4(px.x + r - g, px.y + r - l, g, l), col);
                ui.Texto(b, px.x, px.y + r + 4 * s, 13 * s, col, 1);
            }
            else {   // fuera de pantalla: flecha en el borde
                glm::vec2 c(W * 0.5f, H * 0.5f), d = px - c;
                if (glm::length(d) < 1.0f) d = glm::vec2(0.0f, -1.0f);
                float k = std::fmin((W * 0.5f - m) / (fabsf(d.x) + 1e-4f), (H * 0.5f - m) / (fabsf(d.y) + 1e-4f));
                glm::vec2 q = c + d * k;
                const char* flecha = fabsf(d.x) > fabsf(d.y) ? (d.x > 0 ? "→" : "←") : (d.y > 0 ? "↓" : "↑");
                ui.Texto(flecha, q.x, q.y - 16 * s, 30 * s, col, 1);
                ui.Texto(b, q.x, q.y + 16 * s, 13 * s, col, 1);
            }
        }

        // ---------------- barra de corte de la red
        if (corte > 0.01f && !tortugaLibre) {
            float w = 260 * s, x = (W - w) * 0.5f, y = H * 0.5f - 64 * s;
            ui.Rect(glm::vec4(x - 6 * s, y - 26 * s, w + 12 * s, 42 * s), glm::vec4(0.01f, 0.07f, 0.12f, 0.8f), 10 * s);
            ui.Texto("Cortando la red...", W * 0.5f, y - 22 * s, 14 * s, glm::vec4(1.0f, 0.8f, 0.55f, 1.0f), 1);
            ui.Rect(glm::vec4(x, y, w, 9 * s), glm::vec4(1, 1, 1, 0.2f), 4 * s);
            ui.Rect(glm::vec4(x, y, w * (corte > 1.0f ? 1.0f : corte), 9 * s), glm::vec4(1.0f, 0.75f, 0.4f, 1.0f), 4 * s);
        }

        // ---------------- limpieza del oceano (derecha)
        float pw = 250 * s, ph = 236 * s, px = W - pw - 18 * s, py = 96 * s;
        glm::vec3 cl = glm::mix(glm::vec3(0.75f, 0.6f, 0.35f), glm::vec3(0.4f, 0.95f, 1.0f), limpieza);
        ui.Rect(glm::vec4(px, py, pw, ph), glm::vec4(0.01f, 0.07f, 0.12f, 0.75f), 14 * s);
        ui.Texto("LIMPIEZA DEL OCÉANO", px + 16 * s, py + 12 * s, 13 * s, glm::vec4(0.55f, 0.9f, 1.0f, 1.0f));
        snprintf(b, sizeof(b), "%d%%", (int)(limpieza * 100.0f + 0.5f));
        ui.Texto(b, px + pw - 16 * s, py + 30 * s, 24 * s, glm::vec4(cl, 1.0f), 2);
        ui.Rect(glm::vec4(px + 16 * s, py + 40 * s, pw - 100 * s, 12 * s), glm::vec4(1, 1, 1, 0.15f), 6 * s);
        ui.Rect(glm::vec4(px + 16 * s, py + 40 * s, (pw - 100 * s) * (limpieza < 0.02f ? 0.02f : limpieza), 12 * s), glm::vec4(cl, 1.0f), 6 * s);
        const char* nom[] = { "", "Plásticos", "Metales", "Tóxicos" };
        float yy = py + 68 * s;
        for (int k = 1; k <= 3; k++) {
            snprintf(b, sizeof(b), "%s", nom[k]); ui.Texto(b, px + 16 * s, yy, 14 * s, glm::vec4(0.9f, 0.95f, 1.0f, 1.0f), 0, false);
            snprintf(b, sizeof(b), "%d / %d", recogidos[k], totales[k]);
            ui.Texto(b, px + pw - 16 * s, yy, 14 * s, recogidos[k] == totales[k] ? glm::vec4(0.5f, 1.0f, 0.6f, 1.0f) : glm::vec4(1.0f), 2);
            yy += 22 * s;
        }
        ui.Texto("Peces león retirados", px + 16 * s, yy, 14 * s, glm::vec4(0.9f, 0.95f, 1.0f, 1.0f), 0, false);
        snprintf(b, sizeof(b), "%d / %d", leonesRetirados, leonesTotal);
        ui.Texto(b, px + pw - 16 * s, yy, 14 * s, leonesRetirados == leonesTotal ? glm::vec4(0.5f, 1.0f, 0.6f, 1.0f) : glm::vec4(1.0f, 0.8f, 0.45f, 1.0f), 2); yy += 22 * s;
        ui.Texto("Tortuga liberada", px + 16 * s, yy, 14 * s, glm::vec4(0.9f, 0.95f, 1.0f, 1.0f), 0, false);
        ui.Texto(tortugaLibre ? "Sí" : "No", px + pw - 16 * s, yy, 14 * s, tortugaLibre ? glm::vec4(0.5f, 1.0f, 0.6f, 1.0f) : glm::vec4(1.0f, 0.6f, 0.5f, 1.0f), 2); yy += 22 * s;
        snprintf(b, sizeof(b), "Especies registradas: %d de %d", (int)registradas.size(), totalEspecies);
        ui.Texto(b, px + 16 * s, yy + 6 * s, 13 * s, glm::vec4(0.6f, 0.95f, 1.0f, 1.0f), 0, false);

        // ---------------- objetivos (abajo a la izquierda)
        bool hecho[5] = { limpiezaSinGrandes(), leonesRetirados >= leonesTotal, tortugaLibre, barrilRetirado, cofreAbierto };
        const char* obj[5] = { "Recoge la basura del fondo", "Retira a los peces león", "Libera a la tortuga de la red", "Retira el barril tóxico", "Abre el cofre del tesoro" };
        float bw = 300 * s, bh = 150 * s, bx = 18 * s, by = H - bh - 18 * s;
        ui.Rect(glm::vec4(bx, by, bw, bh), glm::vec4(0.01f, 0.07f, 0.12f, 0.75f), 14 * s);
        ui.Texto("OBJETIVOS", bx + 16 * s, by + 12 * s, 13 * s, glm::vec4(0.55f, 0.9f, 1.0f, 1.0f));
        for (int k = 0; k < 5; k++) {
            float y = by + 36 * s + k * 22 * s;
            glm::vec4 c = hecho[k] ? glm::vec4(0.5f, 1.0f, 0.6f, 1.0f) : glm::vec4(0.92f, 0.95f, 1.0f, 1.0f);
            ui.Texto(hecho[k] ? "✓" : "·", bx + 22 * s, y - 2 * s, 16 * s, c, 1);
            ui.Texto(obj[k], bx + 38 * s, y, 14 * s, c, 0, false);
        }

        // ---------------- avisos (arriba al centro)
        float ay = 92.0f * s;
        for (int i = (int)avisos.size() - 1, n = 0; i >= 0 && n < 2; i--, n++) {
            const AvisoHUD& a = avisos[i];
            float alfa = a.vida < 0.6f ? a.vida / 0.6f : (a.total - a.vida < 0.3f ? (a.total - a.vida) / 0.3f : 1.0f);
            std::vector<std::string> ls = ui.Lineas(a.texto, 430.0f * s, 16.0f * s, true);
            float aw = 0.0f;
            for (auto& l : ls) { float m = ui.MedirTexto(l, 16.0f * s, true); aw = m > aw ? m : aw; }
            aw += 36 * s; float ah = ls.size() * 16.0f * s * 1.3f + 22 * s, ax = (W - aw) * 0.5f;
            ui.Rect(glm::vec4(ax, ay, aw, ah), glm::vec4(0.01f, 0.07f, 0.12f, 0.80f * alfa), 12 * s);
            ui.Rect(glm::vec4(ax, ay, 4 * s, ah), glm::vec4(a.color, alfa), 2 * s);
            for (size_t k = 0; k < ls.size(); k++) ui.Texto(ls[k], W * 0.5f, ay + 11 * s + k * 16.0f * s * 1.3f, 16.0f * s, glm::vec4(a.color, alfa), 1);
            ay += ah + 8 * s;
        }

        // ---------------- ayuda de controles (primeros 20 s)
        float ayuda = tiempoEnZona < 18.0f ? 1.0f : (tiempoEnZona < 20.0f ? (20.0f - tiempoEnZona) / 2.0f : 0.0f);
        if (ayuda > 0.0f) {
            float x0 = bx + bw + 18 * s, x1 = W - 18 * s, aw = x1 - x0;
            const char* l1 = "WASD nadar  ·  Espacio / Ctrl subir y bajar  ·  Shift aletear  ·  F linterna  ·  V cámara  ·  Esc menú";
            const char* l2 = "Clic: recoger basura y retirar peces león  ·  Clic sostenido: cortar la red  ·  El dron señala la basura más cercana";
            std::vector<std::string> a1 = ui.Lineas(l1, aw - 28 * s, 14 * s, false), a2 = ui.Lineas(l2, aw - 28 * s, 14 * s, true);
            float lh = 14 * s * 1.35f, ah = (a1.size() + a2.size()) * lh + 20 * s, y2 = H - 18 * s - ah;
            ui.Rect(glm::vec4(x0, y2, aw, ah), glm::vec4(0.01f, 0.07f, 0.12f, 0.72f * ayuda), 12 * s);
            float y = y2 + 10 * s;
            for (auto& l : a1) { ui.Texto(l, x0 + aw * 0.5f, y, 14 * s, glm::vec4(0.88f, 0.94f, 1.0f, ayuda), 1, false); y += lh; }
            for (auto& l : a2) { ui.Texto(l, x0 + aw * 0.5f, y, 14 * s, glm::vec4(1.0f, 0.8f, 0.5f, ayuda), 1, true); y += lh; }
        }
        ui.Terminar();
    }

private:
    struct AvisoHUD { std::string texto; glm::vec3 color; float vida, total; };
    std::vector<AvisoHUD> avisos;
    int hitos = 0;
    float pesoRecogido = 0.0f, pesoTotal = 0.0f, corte = 0.0f, tiempoLibre = -1.0f;
    int iDron = -1, iRed = -1, iTortuga = -1, iBarril = -1, iNube = -1, iCLata = -1, iCNat = -1, iTapa = -1, iMonedas = -1, iColumna = -1, iSub = -1;
    int objetivo = -1; float distObjetivo = 0.0f; glm::vec3 posObjetivo = glm::vec3(0.0f);
    std::set<std::string> registradas; int totalEspecies = 0;
    std::string ultimaMira; float tiempoMira = 0.0f;
    bool barrilRetirado = false;

    bool limpiezaSinGrandes() const
    {
        // todos los residuos recogibles (plasticos, metales y pilas) excepto el barril
        int r = recogidos[1] + recogidos[2] + recogidos[3] - (barrilRetirado ? 1 : 0);
        int tot = totales[1] + totales[2] + totales[3] - (iBarril >= 0 ? 1 : 0);
        return r >= tot;
    }

    void Liberar(Escenario& esc, float t)
    {
        auto& el = esc.Elementos();
        tortugaLibre = true; sonarDescubrir = true; pesoRecogido += el[iRed].peso; recogidos[4]++; corte = 0.0f;
        el[iRed].visible = false; esc.seleccion = -1;
        if (iTortuga >= 0) {
            Elemento& tg = el[iTortuga];
            glm::vec3 p = esc.Posicion(iTortuga, t);
            tg.mov = MOV_KF; tg.kf = esc.GuardarKF(KF_TortugaEscapa(p, tg.yaw)); tg.kfInicio = t; tg.rotExtra = glm::vec3(0.0f);
            tg.etiqueta = "Tortuga marina"; tg.ondula = 0.0f; tiempoLibre = t;
        }
        Aviso("¡Liberaste a la tortuga! Cada año miles de tortugas, delfines y vaquitas mueren atrapados en redes fantasma.", glm::vec3(0.5f, 1.0f, 0.6f), 8.0f);
    }

    void PrimerResiduo(int tipo, const std::string& etiqueta)
    {
        if (etiqueta == "Barril de residuos toxicos") {
            barrilRetirado = true; sonarAlerta = true;
            Aviso("Retiraste el barril tóxico: deja de filtrar metales pesados y químicos al agua.", glm::vec3(0.6f, 1.0f, 0.5f), 7.0f);
            return;
        }
        if (avisoTipo[tipo]) return;
        avisoTipo[tipo] = true;
        if (tipo == 1) Aviso("Primer plástico recogido. El plástico no desaparece: se rompe en microplásticos que comen los peces.", glm::vec3(0.6f, 0.9f, 1.0f), 6.0f);
        if (tipo == 2) Aviso("Primer metal recogido. Al oxidarse, el metal libera compuestos que dañan a la fauna.", glm::vec3(0.6f, 0.9f, 1.0f), 6.0f);
        if (tipo == 3) Aviso("Recogiste una pila: libera mercurio, cadmio o plomo que se acumulan en la cadena alimenticia.", glm::vec3(0.7f, 1.0f, 0.5f), 6.0f);
    }
    bool avisoTipo[5] = { false, false, false, false, false };

    void Aviso(const std::string& texto, glm::vec3 color, float segundos)
    {
        avisos.push_back({ texto, color, segundos, segundos });
        if (avisos.size() > 4) avisos.erase(avisos.begin());
    }
};

#endif
