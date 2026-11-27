// ============================================================================================
// INTERFAZ · Album de fotos (Zona 1 y Zona 2)
//   X  toma la foto del animal que senala la mira (la mecanica de cada zona decide si cuenta)
//   L  abre / cierra el album: una tarjeta por especie; las fotografiadas muestran su foto
//   Con el album abierto aparece el cursor: clic en una foto la ve en grande (← → cambia de foto,
//   clic o Esc regresa al album). La camara y el nado se detienen mientras esta abierto.
// La foto es una captura real de la pantalla: el centro de la escena 3D (sin HUD) se copia a una
// textura pequena con glBlitFramebuffer justo despues de dibujar la zona (ProyectoFinal.cpp, Update).
// [CAMBIAR] tamano de cada foto: FOTO_ANCHO x FOTO_ALTO · cuanto de la pantalla entra: FOTO_ALTO_PANTALLA
// ============================================================================================
#ifndef ALBUM_H
#define ALBUM_H
#include <map>
#include <string>
#include <vector>
#include <cstdio>
#include <glad/glad.h>
#include <interfaz/menu.h>
#include <interfaz/fichas.h>

const int FOTO_ANCHO = 320, FOTO_ALTO = 240;   // pixeles de cada foto guardada (4:3)
const float FOTO_ALTO_PANTALLA = 0.56f;         // la foto toma el 56% del alto de la pantalla (el marco de la camara)

class AlbumFotos
{
public:
    std::string pendiente;   // especie fotografiada en este cuadro: la mecanica la escribe y Capturar() la guarda
    bool abierto = false;    // tecla L
    int vista = -1;          // foto que se ve en grande (indice en la lista de especies) o -1
    glm::vec2 cursor = glm::vec2(-1.0f);   // cursor en pixeles del framebuffer (lo pone ProyectoFinal)

    void Limpiar()
    {
        for (auto& f : fotos) glDeleteTextures(1, &f.second);
        fotos.clear(); pendiente.clear(); abierto = false; vista = -1;
    }
    void Cerrar() { abierto = false; vista = -1; }

    // Clic con el album abierto: abre la foto bajo el cursor, regresa de la foto grande o cierra el album (clic afuera)
    void Clic()
    {
        if (vista >= 0) { vista = -1; return; }
        if (sobre >= 0) { vista = sobre; return; }
        if (cursor.x < panel.x || cursor.x > panel.x + panel.z || cursor.y < panel.y || cursor.y > panel.y + panel.w) Cerrar();
    }
    // ← → en la foto grande: siguiente / anterior especie que ya tenga foto
    void Mover(int paso)
    {
        int n = (int)lista.size();
        if (vista < 0 || n == 0) return;
        for (int k = 1; k <= n; k++) {
            int i = ((vista + paso * k) % n + n) % n;
            if (Tiene(lista[i])) { vista = i; return; }
        }
    }
    bool Tiene(const std::string& etiqueta) const { return fotos.count(etiqueta) > 0; }

    // Copia el centro de la pantalla a la foto de la especie pendiente.
    // Se llama despues de dibujar la escena y antes del HUD, para que la foto no lleve textos.
    void Capturar(int fbW, int fbH)
    {
        if (pendiente.empty()) return;
        std::string etiqueta = pendiente; pendiente.clear();
        if (fbW <= 0 || fbH <= 0) return;
        if (!fbo) glGenFramebuffers(1, &fbo);

        GLuint tex = Tiene(etiqueta) ? fotos[etiqueta] : 0;
        if (!tex) {
            glGenTextures(1, &tex);
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, FOTO_ANCHO, FOTO_ALTO, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            fotos[etiqueta] = tex;
        }

        // rectangulo 4:3 en el centro de la pantalla
        float h = fbH * FOTO_ALTO_PANTALLA, w = h * FOTO_ANCHO / (float)FOTO_ALTO;
        if (w > fbW) { w = (float)fbW; h = w * FOTO_ALTO / (float)FOTO_ANCHO; }
        int x0 = (int)((fbW - w) * 0.5f), y0 = (int)((fbH - h) * 0.5f);

        GLint leerAntes = 0, dibujarAntes = 0;
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &leerAntes);
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &dibujarAntes);
        GLboolean tijera = glIsEnabled(GL_SCISSOR_TEST);
        glDisable(GL_SCISSOR_TEST);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
        // la pantalla guarda la fila 0 abajo y la interfaz la espera arriba: se voltea al copiar (destino de FOTO_ALTO a 0)
        glBlitFramebuffer(x0, y0, x0 + (int)w, y0 + (int)h, 0, FOTO_ALTO, FOTO_ANCHO, 0, GL_COLOR_BUFFER_BIT, GL_LINEAR);
        // alfa = 1 en toda la foto (la escena puede dejar transparencias en el canal alfa)
        GLfloat limpiar[4]; glGetFloatv(GL_COLOR_CLEAR_VALUE, limpiar);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glClearColor(limpiar[0], limpiar[1], limpiar[2], limpiar[3]);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, leerAntes);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dibujarAntes);
        if (tijera) glEnable(GL_SCISSOR_TEST);
    }

    // Album en pantalla (tecla L). especies = etiquetas en el orden en que se muestran.
    // Se dibuja entre ui.Comenzar() y ui.Terminar().
    void Dibujar(MenuInicio& ui, const std::vector<std::string>& especies, const std::string& zona)
    {
        lista = especies; sobre = -1;
        int n = (int)especies.size();
        if (!abierto || n == 0) return;
        if (vista >= n || (vista >= 0 && !Tiene(especies[vista]))) vista = -1;
        float s = ui.Escala(), W = ui.Ancho(), H = ui.Alto();
        int cols = n > 12 ? 5 : 4, filas = (n + cols - 1) / cols;
        float sep = 12.0f * s, cabeza = 56.0f * s, pie = 42.0f * s, nombreH = 24.0f * s, borde = 20.0f * s;
        // tamano de cada tarjeta para que todo quepa en la pantalla
        float cw = (W * 0.88f - 2 * borde - (cols - 1) * sep) / cols;
        float ch = cw * 0.75f + nombreH;
        float maxH = H * 0.88f - cabeza - pie - (filas - 1) * sep;
        if (filas * ch > maxH) { ch = maxH / filas; cw = (ch - nombreH) / 0.75f; }
        float w = cols * cw + (cols - 1) * sep + 2 * borde, h = cabeza + filas * ch + (filas - 1) * sep + pie;
        float x = (W - w) * 0.5f, y = (H - h) * 0.5f;
        panel = glm::vec4(x, y, w, h);

        int hechas = 0;
        for (const auto& e : especies) if (Tiene(e)) hechas++;
        ui.Rect(panel, glm::vec4(0.01f, 0.07f, 0.12f, 0.92f), 14.0f * s);
        char b[128];
        snprintf(b, sizeof(b), "ÁLBUM DE FOTOS  ·  %s   %d / %d", zona.c_str(), hechas, n);
        ui.Texto(b, W * 0.5f, y + 18.0f * s, 17.0f * s, glm::vec4(0.55f, 0.9f, 1.0f, 1.0f), 1);

        glm::vec3 oro(1.0f, 0.82f, 0.35f);
        for (int i = 0; i < n; i++) {
            float cx = x + borde + (i % cols) * (cw + sep), cy = y + cabeza + (i / cols) * (ch + sep);
            glm::vec4 foto(cx, cy, cw, cw * 0.75f);
            const Ficha* f = BuscarFicha(especies[i]);
            std::string nombre = f ? f->nombre : especies[i];
            auto it = fotos.find(especies[i]);
            if (it != fotos.end()) {   // foto tomada: marco blanco tipo instantanea (dorado si el cursor esta encima)
                bool encima = vista < 0 && cursor.x >= cx && cursor.x <= cx + cw && cursor.y >= cy && cursor.y <= cy + ch;
                if (encima) sobre = i;
                if (encima) ui.Resplandor(glm::vec4(foto.x - 3 * s, foto.y - 3 * s, foto.z + 6 * s, foto.w + 6 * s), glm::vec4(oro, 0.6f), 6 * s, 10 * s);
                ui.Rect(glm::vec4(foto.x - 3 * s, foto.y - 3 * s, foto.z + 6 * s, foto.w + 6 * s), encima ? glm::vec4(oro, 1.0f) : glm::vec4(0.95f, 0.95f, 0.92f, 1.0f), 5.0f * s);
                ui.Imagen(it->second, FOTO_ANCHO, FOTO_ALTO, foto, 3.0f * s);
                ui.Texto(nombre, cx + cw * 0.5f, cy + foto.w + 7 * s, 13.0f * s, encima ? glm::vec4(oro, 1.0f) : glm::vec4(1.0f), 1, false);
            }
            else {                     // falta: hueco con signo de interrogacion
                ui.Rect(foto, glm::vec4(1.0f, 1.0f, 1.0f, 0.06f), 5.0f * s);
                ui.Texto("?", cx + cw * 0.5f, cy + foto.w * 0.5f - 16 * s, 30.0f * s, glm::vec4(1.0f, 1.0f, 1.0f, 0.25f), 1);
                ui.Texto(nombre, cx + cw * 0.5f, cy + foto.w + 7 * s, 13.0f * s, glm::vec4(0.75f, 0.82f, 0.88f, 0.6f), 1, false);
            }
        }
        const char* pista = hechas == 0 ? "Apunta a un animal con la mira y presiona X para fotografiarlo   ·   L para cerrar"
                          : (hechas == n ? "¡Álbum completo!   ·   Clic en una foto para verla en grande   ·   L para cerrar"
                                         : "Clic en una foto para verla en grande   ·   L para cerrar");
        ui.Texto(pista, W * 0.5f, y + h - 30.0f * s, 13.0f * s, glm::vec4(1.0f, 0.85f, 0.5f, 1.0f), 1, false);

        if (vista >= 0) DibujarGrande(ui, especies[vista]);
    }

private:
    std::map<std::string, GLuint> fotos;   // etiqueta de la especie -> textura con su foto
    std::vector<std::string> lista;        // especies del album que se dibujo (para ← →)
    glm::vec4 panel = glm::vec4(0.0f);     // rectangulo del album (clic afuera = cerrar)
    int sobre = -1;                        // foto bajo el cursor

    // Foto en grande con el nombre y el dato de su ficha
    void DibujarGrande(MenuInicio& ui, const std::string& etiqueta)
    {
        auto it = fotos.find(etiqueta);
        if (it == fotos.end()) return;
        float s = ui.Escala(), W = ui.Ancho(), H = ui.Alto();
        const Ficha* f = BuscarFicha(etiqueta);
        ui.Rect(glm::vec4(0, 0, W, H), glm::vec4(0.0f, 0.02f, 0.05f, 0.80f));
        // [CAMBIAR] tamano de la foto grande: 72% del ancho o 62% del alto
        float fw = W * 0.72f, fh = fw * 0.75f;
        if (fh > H * 0.62f) { fh = H * 0.62f; fw = fh / 0.75f; }
        float fx = (W - fw) * 0.5f, fy = H * 0.08f;
        ui.Rect(glm::vec4(fx - 6 * s, fy - 6 * s, fw + 12 * s, fh + 12 * s), glm::vec4(0.95f, 0.95f, 0.92f, 1.0f), 8.0f * s);
        ui.Imagen(it->second, FOTO_ANCHO, FOTO_ALTO, glm::vec4(fx, fy, fw, fh), 4.0f * s);
        float ty = fy + fh + 18 * s;
        ui.Texto(f ? f->nombre : etiqueta, W * 0.5f, ty, 26 * s, glm::vec4(1.0f), 1); ty += 36 * s;
        if (f) {
            for (const std::string& l : ui.Lineas(f->dato, fw, 15.0f * s, false)) {
                ui.Texto(l, W * 0.5f, ty, 15 * s, glm::vec4(0.88f, 0.94f, 1.0f, 0.95f), 1, false); ty += 20 * s;
            }
        }
        ui.Texto("← →  otra foto   ·   Clic o Esc: volver al álbum", W * 0.5f, H - 34 * s, 13 * s, glm::vec4(1.0f, 0.85f, 0.5f, 1.0f), 1, false);
    }
    GLuint fbo = 0;                        // framebuffer auxiliar para copiar la pantalla a la foto
};

inline AlbumFotos& Album() { static AlbumFotos a; return a; }

#endif
