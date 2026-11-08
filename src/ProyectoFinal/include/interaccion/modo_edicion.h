#ifndef MODO_EDICION_H
#define MODO_EDICION_H
// ============================================================================================
// MODO EDICION (practicas 02 y 04: transformaciones) · tecla E
// Permite mover (WASD), subir/bajar (Espacio/Ctrl), rotar (Q/R, 1-4) y escalar (- / =) el objeto senalado.
// F5 guarda la pose en la consola para copiarla al codigo de la zona.
// ============================================================================================

inline Elemento* Escenario::Editable()
{
    if (editIdx < 0 || editIdx >= (int)elementos.size()) return nullptr;
    Elemento& e = elementos[editIdx];
    if (!e.visible) return nullptr;
    // Solo fijos / basura / KF: no cardúmenes ni animales de recorrido libre
    if (e.mov == MOV_CARDUMEN || e.mov == MOV_LIBRE || e.mov == MOV_ORBITA) return nullptr;
    return &e;
}

inline void Escenario::CiclarEditable()
{
    int n = (int)elementos.size();
    if (n == 0) return;
    for (int step = 1; step <= n; step++) {
        int i = (editIdx + step) % n;
        Elemento& e = elementos[i];
        if (!e.visible) continue;
        if (e.mov == MOV_CARDUMEN || e.mov == MOV_LIBRE || e.mov == MOV_ORBITA) continue;
        editIdx = i;
        seleccion = i;
        std::cout << ">> Editando: " << (e.etiqueta.empty() ? e.archivo : e.etiqueta)
                  << "  (pos " << e.pos.x << "," << e.pos.y << "," << e.pos.z
                  << "  esc " << e.escala << "  yaw " << e.yaw << ")" << std::endl;
        return;
    }
}

inline void Escenario::FijarEditableDesdeSeleccion()
{
    if (seleccion < 0 || seleccion >= (int)elementos.size()) { CiclarEditable(); return; }
    Elemento& e = elementos[seleccion];
    if (e.mov == MOV_CARDUMEN || e.mov == MOV_LIBRE || e.mov == MOV_ORBITA) { CiclarEditable(); return; }
    editIdx = seleccion;
    std::cout << ">> Editando: " << (e.etiqueta.empty() ? e.archivo : e.etiqueta)
              << "  (pos " << e.pos.x << "," << e.pos.y << "," << e.pos.z
              << "  esc " << e.escala << "  yaw " << e.yaw << ")" << std::endl;
}

inline void Escenario::GuardarPoseActual()
{
    Elemento* pe = Editable();
    if (!pe) { std::cout << ">> No hay objeto editable seleccionado" << std::endl; return; }
    Elemento& e = *pe;

    std::cout << "// Pose de " << e.archivo << " (carpeta " << e.carpeta << "):\n"
              << "  Elemento p = Fijo(\"" << e.archivo << "\", \"" << e.carpeta << "\",\n"
              << "      glm::vec3(" << e.pos.x << "f, " << e.pos.y << "f, " << e.pos.z << "f),\n"
              << "      " << e.yaw << "f, " << e.escala << "f);\n"
              << "  p.rotExtra = glm::vec3(" << e.rotExtra.x << "f, " << e.rotExtra.y << "f, " << e.rotExtra.z << "f);\n"
              << "  p.tinte = glm::vec3(" << e.tinte.r << "f, " << e.tinte.g << "f, " << e.tinte.b << "f);\n";

    std::ofstream out("poses_editadas.json", std::ios::app);
    if (out) {
        out << "{\n"
            << "  \"archivo\": \"" << e.archivo << "\",\n"
            << "  \"carpeta\": \"" << e.carpeta << "\",\n"
            << "  \"pos\": [" << e.pos.x << ", " << e.pos.y << ", " << e.pos.z << "],\n"
            << "  \"yaw\": " << e.yaw << ",\n"
            << "  \"escala\": " << e.escala << ",\n"
            << "  \"rotExtra\": [" << e.rotExtra.x << ", " << e.rotExtra.y << ", " << e.rotExtra.z << "],\n"
            << "  \"tinte\": [" << e.tinte.r << ", " << e.tinte.g << ", " << e.tinte.b << "]\n"
            << "},\n";
        std::cout << ">> Pose anexada a poses_editadas.json" << std::endl;
    }
}

#endif
