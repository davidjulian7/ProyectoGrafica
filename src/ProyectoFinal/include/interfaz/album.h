// ============================================================================================
// INTERFAZ · Album de fotos (Zona 1 y Zona 2)
//   X  toma la foto del animal que senala la mira (la mecanica de cada zona decide si cuenta)
//   L  abre / cierra el album: una tarjeta por especie; las fotografiadas muestran su foto
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

    void Limpiar()
    {
        for (auto& f : fotos) glDeleteTextures(1, &f.second);
        fotos.clear(); pendiente.clear(); abierto = false;
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
        int n = (int)especies.size();
        if (!abierto || n == 0) return;
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

        int hechas = 0;
        for (const auto& e : especies) if (Tiene(e)) hechas++;
        ui.Rect(glm::vec4(x, y, w, h), glm::vec4(0.01f, 0.07f, 0.12f, 0.92f), 14.0f * s);
        char b[128];
        snprintf(b, sizeof(b), "ÁLBUM DE FOTOS  ·  %s   %d / %d", zona.c_str(), hechas, n);
        ui.Texto(b, W * 0.5f, y + 18.0f * s, 17.0f * s, glm::vec4(0.55f, 0.9f, 1.0f, 1.0f), 1);

        for (int i = 0; i < n; i++) {
            float cx = x + borde + (i % cols) * (cw + sep), cy = y + cabeza + (i / cols) * (ch + sep);
            glm::vec4 foto(cx, cy, cw, cw * 0.75f);
            const Ficha* f = BuscarFicha(especies[i]);
            std::string nombre = f ? f->nombre : especies[i];
            auto it = fotos.find(especies[i]);
            if (it != fotos.end()) {   // foto tomada: marco blanco tipo instantanea
                ui.Rect(glm::vec4(foto.x - 3 * s, foto.y - 3 * s, foto.z + 6 * s, foto.w + 6 * s), glm::vec4(0.95f, 0.95f, 0.92f, 1.0f), 5.0f * s);
                ui.Imagen(it->second, FOTO_ANCHO, FOTO_ALTO, foto, 3.0f * s);
                ui.Texto(nombre, cx + cw * 0.5f, cy + foto.w + 7 * s, 13.0f * s, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 1, false);
            }
            else {                     // falta: hueco con signo de interrogacion
                ui.Rect(foto, glm::vec4(1.0f, 1.0f, 1.0f, 0.06f), 5.0f * s);
                ui.Texto("?", cx + cw * 0.5f, cy + foto.w * 0.5f - 16 * s, 30.0f * s, glm::vec4(1.0f, 1.0f, 1.0f, 0.25f), 1);
                ui.Texto(nombre, cx + cw * 0.5f, cy + foto.w + 7 * s, 13.0f * s, glm::vec4(0.75f, 0.82f, 0.88f, 0.6f), 1, false);
            }
        }
        ui.Texto(hechas == n ? "¡Álbum completo!   ·   L para cerrar" : "Apunta a un animal con la mira y presiona X para fotografiarlo   ·   L para cerrar",
                 W * 0.5f, y + h - 30.0f * s, 13.0f * s, glm::vec4(1.0f, 0.85f, 0.5f, 1.0f), 1, false);
    }

private:
    std::map<std::string, GLuint> fotos;   // etiqueta de la especie -> textura con su foto
    GLuint fbo = 0;                        // framebuffer auxiliar para copiar la pantalla a la foto
};

inline AlbumFotos& Album() { static AlbumFotos a; return a; }

#endif
