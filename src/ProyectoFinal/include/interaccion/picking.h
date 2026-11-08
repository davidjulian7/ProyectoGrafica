#ifndef PICKING_H
#define PICKING_H
// (cabecera)

inline void Escenario::Picking(const glm::mat4& proj, const glm::mat4& view, float t, int fbW, int fbH)
{
    if (InspeccionActiva()) return;
    glm::mat4 inv = glm::inverse(view);
    glm::vec3 o = glm::vec3(inv[3]), d = -glm::normalize(glm::vec3(inv[2]));
    int R = (int)(16.0f * fbH / 768.0f); if (R < 5) R = 5;
    float tanMitad = 1.0f / proj[1][1];
    float holgura = (R + 2) * 2.0f * tanMitad / (float)fbH;     // radio angular del cuadrito
    int cx = fbW / 2, cy = fbH / 2;

    glEnable(GL_SCISSOR_TEST);
    glScissor(cx - R, cy - R, 2 * R + 1, 2 * R + 1);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_BLEND);
    shPicking->use();
    shPicking->setMat4("projection", proj);
    shPicking->setMat4("view", view);
    shPicking->setVec4("PickingColor", glm::vec4(1.0f));       // el terreno tapa lo que esta detras
    shPicking->setMat4("model", glm::mat4(1.0f));
    terreno->Draw(*shPicking);
    for (size_t i = 0; i < elementos.size(); i++) {
        Elemento& e = elementos[i];
        if (!e.visible || e.alpha < 0.35f && !e.interactivo) continue;
        unsigned int id = (zona == 1 || e.interactivo) ? e.id : 0xFFFFFF;
        bool colorPuesto = false;
        for (int k = 0; k < Visibles(e); k++) {
            glm::mat4 M = Matriz(e, (int)i, k, t);
            glm::vec3 c = glm::vec3(M[3]);
            float r = e.radioBase * EscalaMax(M) * 1.1f;
            glm::vec3 v = c - o;
            float tca = glm::dot(v, d);
            if (tca < -r) continue;
            float rr = r + (tca > 0.0f ? tca : 0.0f) * holgura;
            if (glm::dot(v, v) - tca * tca > rr * rr) continue;   // la esfera no toca el rayo de la mira
            if (!colorPuesto) {
                shPicking->setVec4("PickingColor", glm::vec4((id & 0xFF) / 255.0f, ((id >> 8) & 0xFF) / 255.0f, ((id >> 16) & 0xFF) / 255.0f, 1.0f));
                colorPuesto = true;
            }
            shPicking->setMat4("model", M);
            e.modelo->Draw(*shPicking);
        }
    }
    int lado = 2 * R + 1;
    std::vector<unsigned char> px(lado * lado * 4);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(cx - R, cy - R, lado, lado, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    glDisable(GL_SCISSOR_TEST);
    int nuevo = -1, mejor = 1 << 30;
    for (int y = 0; y < lado; y++)
        for (int x = 0; x < lado; x++) {
            const unsigned char* q = &px[(y * lado + x) * 4];
            unsigned int id = q[0] + q[1] * 256 + q[2] * 65536;
            if (id < 1000 || id >= 1000 + elementos.size()) continue;
            int d2 = (x - R) * (x - R) + (y - R) * (y - R);
            if (d2 < mejor) { mejor = d2; nuevo = (int)(id - 1000); }
        }
    if (nuevo != seleccion) {
        seleccion = nuevo;
        if (seleccion >= 0) std::cout << "  Apuntando a: " << elementos[seleccion].etiqueta << std::endl;
    }
}

inline void Escenario::Interactuar()
{
    if (seleccion < 0) return;
    Elemento& e = elementos[seleccion];
    if (e.basura) {
        e.visible = false; basuraRecogida++;
        std::cout << ">> Basura recogida: " << e.etiqueta << "  (" << basuraRecogida << "/" << basuraTotal << ")" << std::endl;
        seleccion = -1;
    }
    else std::cout << ">> " << e.etiqueta << std::endl;
}

#endif
