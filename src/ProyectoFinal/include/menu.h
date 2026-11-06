#ifndef MENU_H
#define MENU_H
/*
* Menu de inicio del Acuario Virtual Interactivo.
*
* Dibuja un fondo submarino animado (shader procedural), el titulo y tres tarjetas,
* una por zona. Se navega con el raton (pasar encima + clic) o con el teclado
* (1, 2, 3, flechas y Enter). Al elegir una zona hace un fundido a negro y
* devuelve el numero de zona elegido.
*
* Tambien dibuja el HUD de la zona (insignia con su nombre) y el fundido de entrada.
*
* Archivos que usa (rutas relativas a src/bin):
*   shaders/menu_fondo.vs, shaders/menu_fondo.fs
*   shaders/menu_ui.vs,    shaders/menu_ui.fs
*   textures/menu/*.png
*/
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>   // shader_m.h lo necesita
#include <iostream>
#include <string>
#include <map>
#include <cmath>

#include <shader_m.h>
#include <stb_image.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <fichas.h>

// Utilidades sin depender de std::min/std::max (Windows define macros min/max).
inline float menuMin(float a, float b) { return a < b ? a : b; }
inline float menuMax(float a, float b) { return a > b ? a : b; }
inline float menuClamp(float v, float lo, float hi) { return menuMin(menuMax(v, lo), hi); }

struct MenuTextura {
    GLuint id = 0;
    int ancho = 0;
    int alto = 0;
};

// Atlas de fuente generado con herramientas/genera_fuente_y_sonidos.py
struct MenuFuente {
    MenuTextura tex;
    float base = 56.0f, asc = 0.0f, desc = 0.0f;
    std::map<int, glm::vec4> rect;   // x, y, ancho, alto en el atlas (pixeles)
    std::map<int, glm::vec3> met;    // desplazamiento x, desplazamiento y, avance
};

inline MenuTextura menuCargarTextura(const std::string& ruta)
{
    MenuTextura t;
    stbi_set_flip_vertically_on_load(false);   // fila 0 = arriba de la imagen
    int canales = 0;
    unsigned char* datos = stbi_load(ruta.c_str(), &t.ancho, &t.alto, &canales, 4);
    if (!datos) {
        std::cout << "[Menu] No se pudo cargar la imagen: " << ruta << std::endl;
        return t;
    }
    glGenTextures(1, &t.id);
    glBindTexture(GL_TEXTURE_2D, t.id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t.ancho, t.alto, 0, GL_RGBA, GL_UNSIGNED_BYTE, datos);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(datos);
    return t;
}

class MenuInicio
{
public:
    static const int NUM_ZONAS = 3;

    // Carga shaders, imagenes y geometria. Llamar una vez despues de iniciar GLAD.
    bool Init()
    {
        fondo = new Shader("shaders/menu_fondo.vs", "shaders/menu_fondo.fs");
        ui = new Shader("shaders/menu_ui.vs", "shaders/menu_ui.fs");

        tarjetas[0] = menuCargarTextura("textures/menu/zona1_arrecifes.png");
        tarjetas[1] = menuCargarTextura("textures/menu/zona2_aguas_frias.png");
        tarjetas[2] = menuCargarTextura("textures/menu/zona3_naufragios.png");
        hud[0] = menuCargarTextura("textures/menu/hud_zona1.png");
        hud[1] = menuCargarTextura("textures/menu/hud_zona2.png");
        hud[2] = menuCargarTextura("textures/menu/hud_zona3.png");
        titulo = menuCargarTextura("textures/menu/titulo.png");
        pie = menuCargarTextura("textures/menu/pie.png");
        controles = menuCargarTextura("textures/menu/hud_controles.png");
        CargarFuente(negrita, "textures/menu/fuente_negrita");
        CargarFuente(normal, "textures/menu/fuente_normal");

        bool ok = titulo.id && pie.id;
        for (int i = 0; i < NUM_ZONAS; i++) ok = ok && tarjetas[i].id && hud[i].id;
        if (!ok) {
            std::cout << "[Menu] Faltan imagenes en src/bin/textures/menu/. "
                      << "Revisa que el directorio de trabajo sea src/bin." << std::endl;
        }

        // Cuadrado unitario (dos triangulos) para todos los elementos de interfaz
        float quad[] = { 0, 0,  1, 0,  1, 1,   0, 0,  1, 1,  0, 1 };
        glGenVertexArrays(1, &vaoQuad);
        glGenBuffers(1, &vboQuad);
        glBindVertexArray(vaoQuad);
        glBindBuffer(GL_ARRAY_BUFFER, vboQuad);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glBindVertexArray(0);

        // El fondo no usa vertices (gl_VertexID), pero el perfil core exige un VAO
        glGenVertexArrays(1, &vaoVacio);

        Reiniciar();
        return ok;
    }

    // Llamar al volver al menu: reinicia animaciones de entrada y seleccion.
    void Reiniciar()
    {
        foco = -1;
        confirmada = -1;
        fundido = 0.0f;
        tiempoEntrada = 0.0f;
        ultimoCursor = glm::vec2(-1.0f);
        for (int i = 0; i < NUM_ZONAS; i++) realce[i] = 0.0f;
    }

    // Teclado (llamar desde el key callback con action == GLFW_PRESS).
    void OnKey(int key)
    {
        if (confirmada >= 0) return;
        if (key == GLFW_KEY_1 || key == GLFW_KEY_KP_1) Confirmar(0);
        else if (key == GLFW_KEY_2 || key == GLFW_KEY_KP_2) Confirmar(1);
        else if (key == GLFW_KEY_3 || key == GLFW_KEY_KP_3) Confirmar(2);
        else if (key == GLFW_KEY_LEFT || key == GLFW_KEY_A) foco = foco <= 0 ? NUM_ZONAS - 1 : foco - 1;
        else if (key == GLFW_KEY_RIGHT || key == GLFW_KEY_D) foco = (foco + 1) % NUM_ZONAS;
        else if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER || key == GLFW_KEY_SPACE)
            Confirmar(foco < 0 ? 0 : foco);
    }

    // Clic izquierdo: entra a la zona que esta debajo del cursor.
    void OnClick()
    {
        if (confirmada >= 0) return;
        if (cursorSobre >= 0) Confirmar(cursorSobre);
    }

    // Avanza animaciones. Devuelve 0 mientras no hay seleccion terminada,
    // o 1, 2, 3 cuando termina el fundido de la zona elegida.
    int Update(GLFWwindow* ventana, float dt)
    {
        dt = menuMin(dt, 0.1f);
        tiempoEntrada += dt;
        Layout(ventana);

        // Cursor en pixeles del framebuffer
        double cx, cy;
        int winW, winH;
        glfwGetCursorPos(ventana, &cx, &cy);
        glfwGetWindowSize(ventana, &winW, &winH);
        glm::vec2 cursor((float)cx * fbW / menuMax(1.0f, (float)winW), (float)cy * fbH / menuMax(1.0f, (float)winH));
        mouse01 = glm::vec2(cursor.x / menuMax(1.0f, fbW), cursor.y / menuMax(1.0f, fbH));

        cursorSobre = -1;
        for (int i = 0; i < NUM_ZONAS; i++) {
            glm::vec4 r = rectTarjeta[i];
            if (cursor.x >= r.x && cursor.x <= r.x + r.z && cursor.y >= r.y && cursor.y <= r.y + r.w) cursorSobre = i;
        }
        // El raton solo cambia el foco cuando se mueve (asi el teclado tambien funciona)
        if (glm::length(cursor - ultimoCursor) > 1.0f && ultimoCursor.x >= 0.0f && confirmada < 0)
            foco = cursorSobre;
        ultimoCursor = cursor;

        for (int i = 0; i < NUM_ZONAS; i++) {
            float objetivo = (i == foco || i == confirmada) ? 1.0f : 0.0f;
            realce[i] += (objetivo - realce[i]) * menuMin(1.0f, dt * 10.0f);
        }

        if (confirmada >= 0) {
            fundido += dt / 0.7f;
            if (fundido >= 1.0f) {
                int zona = confirmada + 1;
                return zona;
            }
        }
        return 0;
    }

    // Dibuja el menu completo.
    void Draw(GLFWwindow* ventana, float tiempo)
    {
        Layout(ventana);
        GLint modoPoligono[2];
        glGetIntegerv(GL_POLYGON_MODE, modoPoligono);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glViewport(0, 0, (int)fbW, (int)fbH);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Fondo animado con el tono de la zona enfocada
        glm::vec3 tinte(0.55f, 0.75f, 0.85f);
        for (int i = 0; i < NUM_ZONAS; i++) tinte = glm::mix(tinte, tonos[i], realce[i] * 0.85f);
        fondo->use();
        fondo->setVec2("uRes", glm::vec2(fbW, fbH));
        fondo->setFloat("uTime", tiempo);
        fondo->setVec3("uTint", tinte);
        fondo->setVec2("uMouse", mouse01);
        glBindVertexArray(vaoVacio);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        ui->use();
        ui->setVec2("uScreen", glm::vec2(fbW, fbH));
        ui->setInt("uTex", 0);
        glBindVertexArray(vaoQuad);
        glActiveTexture(GL_TEXTURE0);

        // Titulo con leve flotacion
        float entradaTitulo = Suave(tiempoEntrada / 0.6f);
        glm::vec4 rt = rectTitulo;
        rt.y += sinf(tiempo * 1.2f) * 3.0f * escala + (1.0f - entradaTitulo) * -20.0f * escala;
        DibujarImagen(titulo, rt, 0.0f, 0.0f, glm::vec4(0.0f), entradaTitulo, 1.0f);

        // Tarjetas: entrada escalonada, realce al enfocar y atenuado de las demas
        bool hayFoco = false;
        for (int i = 0; i < NUM_ZONAS; i++) if (realce[i] > 0.05f) hayFoco = true;
        for (int i = 0; i < NUM_ZONAS; i++) {
            float aparece = Suave((tiempoEntrada - 0.15f - 0.12f * i) / 0.6f);
            float h = realce[i];
            glm::vec4 r = rectTarjeta[i];
            float s = 1.0f + 0.05f * h;
            glm::vec2 centro(r.x + r.z * 0.5f, r.y + r.w * 0.5f - 10.0f * escala * h + (1.0f - aparece) * 40.0f * escala);
            glm::vec4 rr(centro.x - r.z * s * 0.5f, centro.y - r.w * s * 0.5f, r.z * s, r.w * s);
            float radio = 18.0f * escala;

            // sombra
            float pad = 34.0f * escala;
            DibujarBrillo(glm::vec4(rr.x - pad, rr.y - pad + 14.0f * escala, rr.z + 2 * pad, rr.w + 2 * pad),
                          glm::vec4(0.0f, 0.02f, 0.05f, 0.55f), radio, pad, aparece);
            // resplandor del color de la zona
            if (h > 0.01f) {
                float pg = 26.0f * escala;
                DibujarBrillo(glm::vec4(rr.x - pg, rr.y - pg, rr.z + 2 * pg, rr.w + 2 * pg),
                              glm::vec4(acentos[i], 0.65f * h), radio, pg, aparece);
            }
            float brillo = (hayFoco && h < 0.5f) ? 0.62f + 0.38f * h : 1.0f + 0.10f * h;
            glm::vec4 borde = glm::vec4(glm::mix(glm::vec3(0.75f, 0.9f, 1.0f), acentos[i], h), 0.25f + 0.75f * h);
            DibujarImagen(tarjetas[i], rr, radio, (2.0f + 2.0f * h) * escala, borde, aparece, brillo);
        }

        // Indicaciones inferiores con pulso suave
        float pulso = 0.75f + 0.25f * sinf(tiempo * 2.0f);
        DibujarImagen(pie, rectPie, 0.0f, 0.0f, glm::vec4(0.0f), Suave((tiempoEntrada - 0.6f) / 0.5f) * pulso, 1.0f);

        // Fundido al elegir zona ("inmersion")
        if (fundido > 0.0f)
            DibujarColor(glm::vec4(0, 0, fbW, fbH), glm::vec4(0.0f, 0.02f, 0.05f, 1.0f), 0.0f, Suave(fundido));
        // Fundido de entrada al abrir el menu
        float abre = 1.0f - Suave(tiempoEntrada / 0.5f);
        if (abre > 0.0f)
            DibujarColor(glm::vec4(0, 0, fbW, fbH), glm::vec4(0.0f, 0.02f, 0.05f, 1.0f), 0.0f, abre);

        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, modoPoligono[0]);
    }

    // HUD dentro de una zona: insignia con el nombre y fundido de entrada.
    // zona: 1..3, fundidoEntrada: 1 = negro, 0 = transparente.
    void DrawHUD(GLFWwindow* ventana, int zona, float fundidoEntrada, float tiempoEnZona = 0.0f, bool ayudaGenerica = true)
    {
        if (zona < 1 || zona > NUM_ZONAS) return;
        Layout(ventana);
        GLint modoPoligono[2];
        glGetIntegerv(GL_POLYGON_MODE, modoPoligono);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        ui->use();
        ui->setVec2("uScreen", glm::vec2(fbW, fbH));
        ui->setInt("uTex", 0);
        glBindVertexArray(vaoQuad);
        glActiveTexture(GL_TEXTURE0);

        const MenuTextura& t = hud[zona - 1];
        float w = menuMin(360.0f * escala, fbW * 0.45f);
        float h = w * (float)t.alto / menuMax(1.0f, (float)t.ancho);
        DibujarImagen(t, glm::vec4(18.0f * escala, 18.0f * escala, w, h), 0.0f, 0.0f, glm::vec4(0.0f), 0.92f, 1.0f);

        // ayuda de controles: visible los primeros 10 segundos en la zona
        float ayuda = 1.0f - Suave((tiempoEnZona - 10.0f) / 1.5f);
        if (ayuda > 0.0f && controles.id && ayudaGenerica) {
            float cw = menuMin(fbW * 0.9f, 900.0f * escala);
            float ch = cw * (float)controles.alto / menuMax(1.0f, (float)controles.ancho);
            DibujarImagen(controles, glm::vec4((fbW - cw) * 0.5f, fbH - ch - 22.0f * escala, cw, ch), 0.0f, 0.0f, glm::vec4(0.0f), ayuda, 1.0f);
        }

        if (fundidoEntrada > 0.0f)
            DibujarColor(glm::vec4(0, 0, fbW, fbH), glm::vec4(0.0f, 0.02f, 0.05f, 1.0f), 0.0f, Suave(fundidoEntrada));

        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, modoPoligono[0]);
    }

    // Mira del buzo en el centro de la pantalla y nombre de lo que apunta.
    void DrawMira(GLFWwindow* ventana, const std::string& etiqueta, bool basura,
                  const std::string& detalle = std::string(), glm::vec3 colorDetalle = glm::vec3(1.0f))
    {
        Layout(ventana);
        GLint modoPoligono[2];
        glGetIntegerv(GL_POLYGON_MODE, modoPoligono);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        ui->use();
        ui->setVec2("uScreen", glm::vec2(fbW, fbH));
        ui->setInt("uTex", 0);
        glBindVertexArray(vaoQuad);
        glActiveTexture(GL_TEXTURE0);

        bool hay = !etiqueta.empty();
        float cx = fbW * 0.5f, cy = fbH * 0.5f;
        glm::vec3 col = hay ? (basura ? glm::vec3(1.0f, 0.6f, 0.45f) : glm::vec3(0.55f, 0.95f, 1.0f)) : glm::vec3(1.0f);
        float sep = (hay ? 11.0f : 8.0f) * escala, largo = 7.0f * escala, grosor = 2.0f * escala;
        glm::vec4 c(col, hay ? 0.95f : 0.6f);
        DibujarColor(glm::vec4(cx - grosor, cy - grosor, grosor * 2, grosor * 2), c, grosor, 1.0f);
        DibujarColor(glm::vec4(cx - sep - largo, cy - grosor * 0.5f, largo, grosor), c, 0.0f, 1.0f);
        DibujarColor(glm::vec4(cx + sep, cy - grosor * 0.5f, largo, grosor), c, 0.0f, 1.0f);
        DibujarColor(glm::vec4(cx - grosor * 0.5f, cy - sep - largo, grosor, largo), c, 0.0f, 1.0f);
        DibujarColor(glm::vec4(cx - grosor * 0.5f, cy + sep, grosor, largo), c, 0.0f, 1.0f);

        if (hay) {
            const Ficha* f = BuscarFicha(etiqueta);
            if (f && negrita.tex.id) TarjetaFicha(*f, cx, cy + 30.0f * escala, detalle, colorDetalle);
            else {
                const MenuTextura& t = Etiqueta(etiqueta);
                if (t.id) {
                    float w = 320.0f * escala, h = w * (float)t.alto / menuMax(1.0f, (float)t.ancho);
                    DibujarImagen(t, glm::vec4(cx - w * 0.5f, cy + 30.0f * escala, w, h), 0.0f, 0.0f, glm::vec4(0.0f), 0.95f, 1.0f);
                }
            }
        }
        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, modoPoligono[0]);
    }

    void DrawInspeccion(GLFWwindow* ventana, const std::string& etiqueta, const std::string& modelo,
                        const std::string& detalle = std::string(), glm::vec3 colorDetalle = glm::vec3(1.0f))
    {
        Layout(ventana);
        Comenzar(ventana);
        float s = escala, w = menuMin(390.0f * s, fbW * 0.40f), pad = 18.0f * s;
        const Ficha* ficha = BuscarFicha(etiqueta);
        std::string titulo = ficha ? ficha->nombre : etiqueta;
        std::vector<std::string> dato = ficha ? Lineas(ficha->dato, w - 2.0f * pad, 15.0f * s, false)
                                             : Lineas("Modelo del arrecife seleccionado para observar sus formas y materiales.", w - 2.0f * pad, 15.0f * s, false);
        std::vector<std::string> extra = detalle.empty() ? std::vector<std::string>() : Lineas(detalle, w - 2.0f * pad, 13.0f * s, false);
        float h = pad * 2.0f + 18.0f * s + 34.0f * s + 22.0f * s + dato.size() * 19.0f * s
                + extra.size() * 17.0f * s + 58.0f * s;
        float x = 22.0f * s, y = (fbH - h) * 0.5f;
        DibujarColor(glm::vec4(0.0f, 0.0f, fbW, fbH), glm::vec4(0.0f, 0.025f, 0.05f, 0.58f), 0.0f, 1.0f);
        DibujarColor(glm::vec4(x, y, w, h), glm::vec4(0.01f, 0.07f, 0.12f, 0.94f), 14.0f * s, 1.0f);
        DibujarColor(glm::vec4(x, y, 4.0f * s, h), glm::vec4(0.35f, 0.86f, 0.84f, 1.0f), 2.0f * s, 1.0f);
        float ty = y + pad;
        Texto("INSPECCION · SIMULACION EN PAUSA", x + pad, ty, 12.0f * s, glm::vec4(0.45f, 0.9f, 0.88f, 1.0f)); ty += 23.0f * s;
        Texto(titulo, x + pad, ty, 24.0f * s, glm::vec4(1.0f), 0, true); ty += 32.0f * s;
        Texto(ficha ? ficha->categoria : "MODELO 3D", x + pad, ty, 13.0f * s, glm::vec4(0.65f, 0.82f, 0.88f, 1.0f)); ty += 20.0f * s;
        if (ficha) { Texto(ficha->estado, x + pad, ty, 14.0f * s, glm::vec4(0.9f, 0.78f, 0.48f, 1.0f), 0, false); ty += 22.0f * s; }
        for (const std::string& linea : dato) { Texto(linea, x + pad, ty, 15.0f * s, glm::vec4(0.94f, 0.96f, 0.98f, 1.0f), 0, false); ty += 19.0f * s; }
        for (const std::string& linea : extra) { Texto(linea, x + pad, ty + 4.0f * s, 13.0f * s, glm::vec4(colorDetalle, 1.0f), 0, false); ty += 17.0f * s; }
        Texto("MODELO: " + modelo + ".fbx", x + pad, y + h - 34.0f * s, 12.0f * s, glm::vec4(0.68f, 0.78f, 0.82f, 1.0f), 0, false);
        float cx = fbW * 0.70f;
        Texto("Mueve el raton para rotar", cx, fbH - 78.0f * s, 15.0f * s, glm::vec4(1.0f), 1);
        Texto("Rueda: acercar / alejar   ·   Clic o Esc: cerrar", cx, fbH - 52.0f * s, 13.0f * s, glm::vec4(0.8f, 0.9f, 0.94f, 1.0f), 1, false);
        Terminar();
    }

    // ------------------------------------------------------------------
    // API de interfaz para el HUD de las zonas (texto, paneles, barras)
    // Uso: Comenzar(ventana); ...Texto/Rect...; Terminar();
    // ------------------------------------------------------------------
    void Comenzar(GLFWwindow* ventana)
    {
        Layout(ventana);
        glGetIntegerv(GL_POLYGON_MODE, modoGuardado);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        ui->use();
        ui->setVec2("uScreen", glm::vec2(fbW, fbH));
        ui->setInt("uTex", 0);
        glBindVertexArray(vaoQuad);
        glActiveTexture(GL_TEXTURE0);
    }
    void Terminar()
    {
        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, modoGuardado[0]);
    }
    float Escala() const { return escala; }
    float Ancho() const { return fbW; }
    float Alto() const { return fbH; }
    void Rect(glm::vec4 r, glm::vec4 color, float radio = 0.0f) { DibujarColor(r, color, radio, 1.0f); }
    void Resplandor(glm::vec4 r, glm::vec4 color, float radio, float pad) { DibujarBrillo(r, color, radio, pad, 1.0f); }

    // Ancho en pixeles de un texto (UTF-8)
    float MedirTexto(const std::string& s, float tam, bool usarNegrita = true)
    {
        MenuFuente& f = usarNegrita ? negrita : normal;
        float k = tam / f.base, x = 0.0f;
        for (size_t i = 0; i < s.size(); ) {
            int cp = SiguienteLetra(s, i);
            auto m = f.met.find(cp);
            if (m != f.met.end()) x += m->second.z * k;
        }
        return x;
    }

    // Dibuja texto. y = parte superior de la linea. alinear: 0 izquierda, 1 centro, 2 derecha
    float Texto(const std::string& s, float x, float y, float tam, glm::vec4 color, int alinear = 0, bool usarNegrita = true, bool sombra = true)
    {
        MenuFuente& f = usarNegrita ? negrita : normal;
        if (!f.tex.id) return 0.0f;
        float ancho = MedirTexto(s, tam, usarNegrita);
        if (alinear == 1) x -= ancho * 0.5f; else if (alinear == 2) x -= ancho;
        glBindTexture(GL_TEXTURE_2D, f.tex.id);
        ui->setInt("uMode", 3);
        ui->setFloat("uAlpha", 1.0f);
        if (sombra) Letras(f, s, x + tam * 0.06f, y + tam * 0.06f, tam, glm::vec4(0.0f, 0.02f, 0.05f, color.a * 0.7f));
        Letras(f, s, x, y, tam, color);
        return ancho;
    }

    // Parte un texto en lineas que caben en 'ancho'
    std::vector<std::string> Lineas(const std::string& s, float ancho, float tam, bool usarNegrita = false)
    {
        std::vector<std::string> res;
        std::istringstream in(s);
        std::string palabra, linea;
        while (in >> palabra) {
            std::string prueba = linea.empty() ? palabra : linea + " " + palabra;
            if (!linea.empty() && MedirTexto(prueba, tam, usarNegrita) > ancho) { res.push_back(linea); linea = palabra; }
            else linea = prueba;
        }
        if (!linea.empty()) res.push_back(linea);
        return res;
    }

    // Parrafo con ajuste de linea; devuelve la altura usada
    float Parrafo(const std::string& s, float x, float y, float ancho, float tam, glm::vec4 color, bool usarNegrita = false, int alinear = 0)
    {
        std::vector<std::string> ls = Lineas(s, ancho, tam, usarNegrita);
        float alto = tam * 1.3f;
        for (size_t i = 0; i < ls.size(); i++) Texto(ls[i], alinear == 1 ? x + ancho * 0.5f : x, y + i * alto, tam, color, alinear, usarNegrita);
        return ls.size() * alto;
    }

private:
    MenuTextura controles;
    MenuFuente negrita, normal;
    GLint modoGuardado[2] = { GL_FILL, GL_FILL };

    static int SiguienteLetra(const std::string& s, size_t& i)
    {
        unsigned char c = (unsigned char)s[i];
        if (c < 0x80) { i++; return c; }
        if ((c >> 5) == 6 && i + 1 < s.size()) { int cp = ((c & 0x1F) << 6) | (s[i + 1] & 0x3F); i += 2; return cp; }
        if ((c >> 4) == 14 && i + 2 < s.size()) { int cp = ((c & 0x0F) << 12) | ((s[i + 1] & 0x3F) << 6) | (s[i + 2] & 0x3F); i += 3; return cp; }
        i++; return '?';
    }

    void CargarFuente(MenuFuente& f, const std::string& ruta)
    {
        f.tex = menuCargarTextura(ruta + ".png");
        std::ifstream in(ruta + ".txt");
        if (!in) { std::cout << "[Menu] Falta la fuente " << ruta << ".txt" << std::endl; return; }
        in >> f.base >> f.asc >> f.desc;
        int cp; float x, y, w, h, ox, oy, av;
        while (in >> cp >> x >> y >> w >> h >> ox >> oy >> av) {
            f.rect[cp] = glm::vec4(x, y, w, h);
            f.met[cp] = glm::vec3(ox, oy, av);
        }
    }

    void Letras(MenuFuente& f, const std::string& s, float x, float y, float tam, glm::vec4 color)
    {
        float k = tam / f.base;
        ui->setVec4("uColor", color);
        for (size_t i = 0; i < s.size(); ) {
            int cp = SiguienteLetra(s, i);
            auto r = f.rect.find(cp);
            auto m = f.met.find(cp);
            if (r == f.rect.end() || m == f.met.end()) continue;
            if (cp != ' ') {
                glm::vec4 q = r->second;
                ui->setVec4("uRect", glm::vec4(x + m->second.x * k, y + m->second.y * k, q.z * k, q.w * k));
                ui->setVec2("uSize", glm::vec2(q.z * k, q.w * k));
                ui->setVec4("uUV", glm::vec4(q.x / f.tex.ancho, q.y / f.tex.alto, q.z / f.tex.ancho, q.w / f.tex.alto));
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
            x += m->second.z * k;
        }
    }

    // Tarjeta educativa bajo la mira
    void TarjetaFicha(const Ficha& f, float cx, float y, const std::string& detalle, glm::vec3 colorDetalle)
    {
        float s = escala, w = 470.0f * s, pad = 16.0f * s;
        std::vector<std::string> ls = Lineas(f.dato, w - 2 * pad, 15.0f * s, false);
        float h = pad + 18.0f * s + 32.0f * s + 22.0f * s + ls.size() * 15.0f * s * 1.3f + (detalle.empty() ? 0.0f : 24.0f * s) + pad;
        if (y + h > fbH - 12.0f * s) y = fbH - 12.0f * s - h;
        glm::vec3 ac = f.categoria == "BASURA" ? glm::vec3(1.0f, 0.6f, 0.45f)
                     : f.categoria == "EN PELIGRO" ? glm::vec3(1.0f, 0.5f, 0.5f)
                     : f.categoria == "ESPECIE INVASORA" ? glm::vec3(1.0f, 0.75f, 0.35f)
                     : f.categoria == "ARRECIFE" ? glm::vec3(1.0f, 0.62f, 0.45f) : glm::vec3(0.55f, 0.9f, 1.0f);
        float x = cx - w * 0.5f;
        DibujarColor(glm::vec4(x, y, w, h), glm::vec4(0.01f, 0.07f, 0.12f, 0.80f), 14.0f * s, 1.0f);
        DibujarColor(glm::vec4(x, y, 4.0f * s, h), glm::vec4(ac, 0.9f), 2.0f * s, 1.0f);
        float yy = y + pad;
        Texto(f.categoria, x + pad, yy, 13.0f * s, glm::vec4(ac, 1.0f)); yy += 18.0f * s;
        Texto(f.nombre, x + pad, yy, 25.0f * s, glm::vec4(1.0f)); yy += 32.0f * s;
        Texto(f.estado, x + pad, yy, 14.0f * s, glm::vec4(0.75f, 0.85f, 0.9f, 1.0f), 0, false); yy += 22.0f * s;
        for (const std::string& l : ls) { Texto(l, x + pad, yy, 15.0f * s, glm::vec4(0.92f, 0.95f, 0.97f, 1.0f), 0, false); yy += 15.0f * s * 1.3f; }
        if (!detalle.empty()) Texto(detalle, x + pad, yy + 4.0f * s, 15.0f * s, glm::vec4(colorDetalle, 1.0f));
    }
    std::map<std::string, MenuTextura> etiquetas;

    // "Pez leon (invasor)" -> textures/menu/etiquetas/pez_leon_invasor.png
    const MenuTextura& Etiqueta(const std::string& nombre)
    {
        auto it = etiquetas.find(nombre);
        if (it != etiquetas.end()) return it->second;
        std::string f;
        for (char ch : nombre) {
            if (ch == ' ') f += '_';
            else if ((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')) f += ch;
            else if (ch >= 'A' && ch <= 'Z') f += (char)(ch - 'A' + 'a');
        }
        etiquetas[nombre] = menuCargarTextura("textures/menu/etiquetas/" + f + ".png");
        return etiquetas[nombre];
    }

    Shader* fondo = nullptr;
    Shader* ui = nullptr;
    GLuint vaoQuad = 0, vboQuad = 0, vaoVacio = 0;

    MenuTextura tarjetas[NUM_ZONAS];
    MenuTextura hud[NUM_ZONAS];
    MenuTextura titulo, pie;

    // Colores de acento y tonos de agua por zona: arrecifes, aguas frias, naufragios
    glm::vec3 acentos[NUM_ZONAS] = { glm::vec3(1.0f, 0.59f, 0.39f), glm::vec3(0.55f, 0.90f, 1.0f), glm::vec3(1.0f, 0.78f, 0.43f) };
    glm::vec3 tonos[NUM_ZONAS] = { glm::vec3(0.45f, 0.90f, 0.85f), glm::vec3(0.75f, 0.90f, 1.10f), glm::vec3(0.35f, 0.62f, 0.50f) };

    int foco = -1;          // tarjeta resaltada (raton o teclado)
    int cursorSobre = -1;   // tarjeta bajo el cursor
    int confirmada = -1;    // zona elegida (0..2)
    float fundido = 0.0f;
    float tiempoEntrada = 0.0f;
    float realce[NUM_ZONAS] = { 0.0f, 0.0f, 0.0f };
    glm::vec2 ultimoCursor = glm::vec2(-1.0f);
    glm::vec2 mouse01 = glm::vec2(0.5f);

    float fbW = 1024.0f, fbH = 768.0f, escala = 1.0f;
    glm::vec4 rectTitulo, rectPie, rectTarjeta[NUM_ZONAS];

    void Confirmar(int i)
    {
        confirmada = i;
        foco = i;
        fundido = 0.0f;
    }

    static float Suave(float t)
    {
        t = menuClamp(t, 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    // Calcula posiciones segun el tamano actual de la ventana (se adapta al maximizar).
    void Layout(GLFWwindow* ventana)
    {
        int w, h;
        glfwGetFramebufferSize(ventana, &w, &h);
        fbW = (float)menuMax(1, w);
        fbH = (float)menuMax(1, h);
        escala = menuMin(fbW / 1024.0f, fbH / 768.0f);

        float tituloW = menuMin(fbW * 0.80f, fbH * 1.25f);
        float tituloH = tituloW * (float)titulo.alto / menuMax(1.0f, (float)titulo.ancho);
        rectTitulo = glm::vec4((fbW - tituloW) * 0.5f, fbH * 0.045f, tituloW, tituloH);

        float pieW = menuMin(fbW * 0.78f, fbH * 1.15f);
        float pieH = pieW * (float)pie.alto / menuMax(1.0f, (float)pie.ancho);
        rectPie = glm::vec4((fbW - pieW) * 0.5f, fbH * 0.955f - pieH, pieW, pieH);

        float zonaArriba = rectTitulo.y + tituloH + fbH * 0.035f;
        float zonaAbajo = rectPie.y - fbH * 0.035f;
        float altoDisp = zonaAbajo - zonaArriba;
        float hueco = fbW * 0.03f;
        float tarjW = (fbW * 0.86f - 2.0f * hueco) / 3.0f;
        float tarjH = tarjW * 1.25f;
        if (tarjH > altoDisp) { tarjH = altoDisp; tarjW = tarjH / 1.25f; }
        float total = 3.0f * tarjW + 2.0f * hueco;
        float x0 = (fbW - total) * 0.5f;
        float y0 = zonaArriba + (altoDisp - tarjH) * 0.5f;
        for (int i = 0; i < NUM_ZONAS; i++)
            rectTarjeta[i] = glm::vec4(x0 + i * (tarjW + hueco), y0, tarjW, tarjH);
    }

    void DibujarImagen(const MenuTextura& t, glm::vec4 r, float radio, float borde, glm::vec4 colorBorde, float alpha, float brillo)
    {
        if (!t.id || alpha <= 0.001f) return;
        glBindTexture(GL_TEXTURE_2D, t.id);
        ui->setVec4("uUV", glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
        ui->setInt("uMode", 0);
        ui->setVec4("uRect", r);
        ui->setVec2("uSize", glm::vec2(r.z, r.w));
        ui->setFloat("uRadius", radio);
        ui->setFloat("uBorder", borde);
        ui->setVec4("uColor", colorBorde);
        ui->setFloat("uAlpha", alpha);
        ui->setFloat("uBright", brillo);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    void DibujarColor(glm::vec4 r, glm::vec4 color, float radio, float alpha)
    {
        ui->setVec4("uUV", glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
        ui->setInt("uMode", 1);
        ui->setVec4("uRect", r);
        ui->setVec2("uSize", glm::vec2(r.z, r.w));
        ui->setFloat("uRadius", radio);
        ui->setVec4("uColor", color);
        ui->setFloat("uAlpha", alpha);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    void DibujarBrillo(glm::vec4 r, glm::vec4 color, float radio, float pad, float alpha)
    {
        ui->setInt("uMode", 2);
        ui->setVec4("uRect", r);
        ui->setVec2("uSize", glm::vec2(r.z, r.w));
        ui->setFloat("uRadius", radio);
        ui->setFloat("uGlowPad", pad);
        ui->setVec4("uColor", color);
        ui->setFloat("uAlpha", alpha);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
};

#endif
