#ifndef KEYFRAMES_H
#define KEYFRAMES_H
/*
 * Animaciones basadas en cuadros clave (keyframes) a nivel de objeto.
 *
 * Cada AnimacionKF es una lista de poses KeyPose en el tiempo. Entre dos keys
 * se interpola posicion/rotacion/escala con smoothstep (yaw con interpolacion
 * angular de camino corto). Se integra con Elemento::MOV_KF en zonas.h.
 *
 * Son las "5 animaciones basadas en cuadros clave" de la rúbrica:
 *   1. Ballena salta y reingresa (Zona 2)
 *   2. Submarino se asienta con escora (Zona 3)
 *   3. Columna cae, rebota y se posa (Zona 3)
 *   4. Almeja abre y cierra la concha (Zona 1)
 *   5. Bolsa de plástico recorre puntos de flotación (Zona 3)
 */
#include <glm/glm.hpp>
#include <vector>
#include <cmath>

struct KeyPose {
    float t = 0.0f;                    // segundos desde el inicio del ciclo
    glm::vec3 pos = glm::vec3(0.0f);
    float yaw = 0.0f;                  // grados
    glm::vec3 rotExtra = glm::vec3(0.0f); // grados X,Y,Z
    float escala = 1.0f;
};

struct AnimacionKF {
    std::vector<KeyPose> keys;
    bool loop = true;

    float Duracion() const
    {
        if (keys.empty()) return 0.0f;
        return keys.back().t;
    }

    // Interpola la pose en el tiempo t (segundos, se envuelve si loop)
    void PoseAt(float t, glm::vec3& pos, float& yaw, glm::vec3& rotExtra, float& escala) const
    {
        if (keys.empty()) { pos = glm::vec3(0.0f); yaw = 0.0f; rotExtra = glm::vec3(0.0f); escala = 1.0f; return; }
        if (keys.size() == 1) {
            pos = keys[0].pos; yaw = keys[0].yaw; rotExtra = keys[0].rotExtra; escala = keys[0].escala;
            return;
        }
        float dur = Duracion();
        if (dur <= 0.0f) {
            pos = keys[0].pos; yaw = keys[0].yaw; rotExtra = keys[0].rotExtra; escala = keys[0].escala;
            return;
        }
        if (loop) t = std::fmod(t, dur); if (t < 0.0f) t += dur;
        else { if (t <= 0.0f) t = 0.0f; if (t >= dur) t = dur; }

        // buscar segmento [i, i+1]
        int i = 0;
        for (int n = (int)keys.size() - 1; i < n; i++)
            if (t <= keys[i + 1].t) break;
        const KeyPose& a = keys[i];
        const KeyPose& b = keys[(i + 1 < (int)keys.size()) ? i + 1 : i];
        float span = b.t - a.t;
        float u = (span > 1e-6f) ? (t - a.t) / span : 0.0f;
        u = u * u * (3.0f - 2.0f * u);   // smoothstep: sin saltos en los keys

        pos = glm::mix(a.pos, b.pos, u);
        escala = glm::mix(a.escala, b.escala, u);
        rotExtra = glm::mix(a.rotExtra, b.rotExtra, u);
        // yaw por camino corto (evita girar 350° en vez de 10°)
        float d = b.yaw - a.yaw;
        while (d > 180.0f) d -= 360.0f;
        while (d < -180.0f) d += 360.0f;
        yaw = a.yaw + d * u;
    }
};

// Fábricas de las 5 animaciones de la rúbrica (coordenadas en el espacio de cada zona)
inline AnimacionKF KF_BallenaSalto(const glm::vec3& base)
{
    // sale del fondo, rompe la superficie y regresa (ciclo ~12 s)
    AnimacionKF a;
    a.keys = {
        { 0.0f,  base + glm::vec3(0.0f,  0.0f,  0.0f), 0.0f,  glm::vec3(0.0f),              1.0f },
        { 3.5f,  base + glm::vec3(4.0f,  7.0f,  2.0f), 25.0f, glm::vec3(-25.0f, 0.0f, 0.0f), 1.0f },
        { 5.0f,  base + glm::vec3(7.0f, 11.5f,  4.0f), 40.0f, glm::vec3(-45.0f, 0.0f, 0.0f), 1.05f }, // pico: sale del agua
        { 6.5f,  base + glm::vec3(10.0f, 7.0f,  6.0f), 55.0f, glm::vec3(-15.0f, 0.0f, 0.0f), 1.0f },
        { 9.5f,  base + glm::vec3(14.0f, 1.0f,  8.0f), 80.0f, glm::vec3(20.0f,  0.0f, 0.0f), 1.0f },
        { 12.0f, base + glm::vec3(0.0f,  0.0f,  0.0f), 360.0f,glm::vec3(0.0f),               1.0f },
    };
    return a;
}

inline AnimacionKF KF_SubmarinoAsienta(const glm::vec3& base)
{
    // se hunde, escora progresivamente y queda varado (ciclo 10 s)
    AnimacionKF a;
    a.keys = {
        { 0.0f,  base + glm::vec3(0.0f, 4.0f,  0.0f),  25.0f, glm::vec3(0.0f),   1.0f },
        { 3.0f,  base + glm::vec3(1.0f, 2.0f, -1.0f),  28.0f, glm::vec3(4.0f),   1.0f },
        { 6.0f,  base + glm::vec3(1.8f, 0.6f, -1.8f),  32.0f, glm::vec3(8.0f),   1.0f },
        { 8.0f,  base + glm::vec3(2.2f, 0.4f, -2.2f),  34.0f, glm::vec3(12.0f),  1.0f }, // golpe seco
        { 8.4f,  base + glm::vec3(2.3f, 0.7f, -2.3f),  34.0f, glm::vec3(9.0f),   1.0f }, // rebote mínimo
        { 10.0f, base + glm::vec3(2.4f, 0.55f, -2.4f), 35.0f, glm::vec3(10.0f),  1.0f },
    };
    return a;
}

inline AnimacionKF KF_ColumnaCae(const glm::vec3& base)
{
    // cae, rebota y se posa con escora (ciclo 6 s)
    AnimacionKF a;
    a.keys = {
        { 0.0f, base + glm::vec3(0.0f, 5.0f, 0.0f),  0.0f, glm::vec3(0.0f,  0.0f,  0.0f), 1.0f },
        { 1.2f, base + glm::vec3(0.0f, 0.0f, 0.0f),  0.0f, glm::vec3(0.0f,  0.0f, 40.0f), 1.0f },
        { 1.5f, base + glm::vec3(0.0f, 1.1f, 0.0f),  0.0f, glm::vec3(0.0f,  0.0f, 55.0f), 1.0f }, // rebote
        { 1.9f, base + glm::vec3(0.0f, 0.0f, 0.0f),  0.0f, glm::vec3(0.0f,  0.0f, 70.0f), 1.0f },
        { 2.2f, base + glm::vec3(0.0f, 0.35f,0.0f),  0.0f, glm::vec3(0.0f,  0.0f, 78.0f), 1.0f }, // micro-rebote
        { 3.0f, base + glm::vec3(0.0f, 0.0f, 0.0f),  0.0f, glm::vec3(0.0f,  0.0f, 88.0f), 1.0f },
        { 6.0f, base + glm::vec3(0.0f, 0.0f, 0.0f),  0.0f, glm::vec3(0.0f,  0.0f, 88.0f), 1.0f }, // reposo
    };
    return a;
}

inline AnimacionKF KF_AlmejaAbre(const glm::vec3& base)
{
    // abre y cierra la concha alternando rotExtra.x (ciclo 4 s)
    AnimacionKF a;
    a.keys = {
        { 0.0f, base, 30.0f, glm::vec3(0.0f),  1.0f },
        { 1.0f, base, 30.0f, glm::vec3(35.0f), 1.06f }, // abre
        { 2.0f, base, 30.0f, glm::vec3(0.0f),  1.0f },  // cierra
        { 3.0f, base, 30.0f, glm::vec3(35.0f), 1.06f },
        { 4.0f, base, 30.0f, glm::vec3(0.0f),  1.0f },
    };
    return a;
}

inline AnimacionKF KF_BolsaFlota(const glm::vec3& base)
{
    // recorre 5 puntos de flotación en el agua (ciclo 9 s)
    AnimacionKF a;
    a.keys = {
        { 0.0f, base + glm::vec3( 0.0f,  0.0f,  0.0f),   0.0f, glm::vec3(0.0f), 1.0f },
        { 2.0f, base + glm::vec3( 2.5f,  1.8f, -1.5f),  60.0f, glm::vec3(15.0f, 25.0f, 0.0f), 1.05f },
        { 4.0f, base + glm::vec3(-1.0f,  3.2f,  2.0f), 140.0f, glm::vec3(-10.0f, 70.0f, 20.0f), 1.1f },
        { 6.0f, base + glm::vec3(-3.0f,  1.5f, -2.5f), 220.0f, glm::vec3(20.0f, 130.0f, -15.0f), 1.0f },
        { 7.5f, base + glm::vec3( 1.5f,  4.0f,  1.0f), 300.0f, glm::vec3(-25.0f, 200.0f, 10.0f), 1.08f },
        { 9.0f, base + glm::vec3( 0.0f,  0.0f,  0.0f), 360.0f, glm::vec3(0.0f), 1.0f },
    };
    return a;
}

#endif // KEYFRAMES_H
