#ifndef COLOCAR_MODELOS_H
#define COLOCAR_MODELOS_H
// (cabecera)

inline Elemento Escenario::Fijo(const char* archivo, const char* carpeta, glm::vec3 pos, float yaw, float escala)
{
    Elemento e; e.archivo = archivo; e.carpeta = carpeta; e.pos = pos; e.yaw = yaw; e.escala = escala;
    e.sobreSuelo = true; e.mov = MOV_FIJO;
    return e;
}

inline Elemento Escenario::Nadador(const char* archivo, const char* carpeta, TipoMov mov, glm::vec3 centro, float radio, float vel, const char* etiqueta)
{
    Elemento e; e.archivo = archivo; e.carpeta = carpeta; e.mov = mov; e.pos = centro; e.radio = radio; e.vel = vel;
    e.oscAmp = 0.4f; e.oscVel = 0.7f; e.ondula = 0.06f; e.interactivo = true; e.etiqueta = etiqueta;
    return e;
}

inline void Escenario::Esparcir(Elemento base, int n, glm::vec2 centro, float rMin, float rMax, float escMin, float escMax, int semilla, const std::vector<glm::vec3>& tintes)
{
    for (int i = 0; i < n; i++) {
        Elemento e = base;
        float a = Aleatorio(semilla, i) * 6.2832f;
        float r = rMin + (rMax - rMin) * sqrtf(Aleatorio(semilla + 1, i));
        e.pos = glm::vec3(centro.x + cosf(a) * r, base.pos.y, centro.y + sinf(a) * r);
        e.yaw = Aleatorio(semilla + 2, i) * 360.0f;
        e.escala = escMin + (escMax - escMin) * Aleatorio(semilla + 3, i);
        e.fase = Aleatorio(semilla + 4, i) * 6.28f;
        if (!tintes.empty()) e.tinte = tintes[i % tintes.size()];
        elementos.push_back(e);
    }
}

#endif
