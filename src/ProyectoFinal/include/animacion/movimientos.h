#ifndef MOVIMIENTOS_H
#define MOVIMIENTOS_H
// ============================================================================================
// ANIMACION SIMPLE Y PROCEDURAL DE CADA OBJETO (practicas 02, 04, 09 y 10)
// Matriz() arma la matriz de modelo de cada objeto en cada cuadro:  traslacion * rotacion * escala.
// Segun e.mov el objeto: se queda fijo (MOV_FIJO), nada en circulo (MOV_ORBITA), en cardumen (MOV_CARDUMEN),
// en forma de 8 (MOV_LIBRE), a la deriva (MOV_DERIVA), por cuadros clave (MOV_KF), etc.
// [CAMBIAR] la velocidad de TODA la fauna: velFauna en escena/escenario.h.
// [CAMBIAR] la velocidad de UN animal: su campo .vel donde se crea (zonas/<zona>/<zona>_escena.h).
// ============================================================================================

inline glm::vec3 Escenario::PosMovil(const Elemento& e, int idx, int k, float t, float& nado, glm::vec3& dir) const
{
    nado = 1.0f; dir = glm::vec3(0.0f);
    float signo = e.vel >= 0.0f ? 1.0f : -1.0f;
    glm::vec3 off = k > 0 ? Aleatorio3(idx * 97 + k) * e.dispersion : glm::vec3(0.0f);
    glm::vec3 p = e.pos;
    switch (e.mov) {
    case MOV_ORBITA:
    case MOV_CARDUMEN: {
        float a = t * e.vel * velFauna + e.fase - (k > 0 ? (0.15f + 0.25f * Aleatorio(idx, k)) * signo * (float)(k % 7) * 0.3f : 0.0f);
        float r = e.radio + off.x;
        p = e.pos + glm::vec3(cosf(a) * r, off.y + sinf(t * e.oscVel + e.fase + k) * e.oscAmp, sinf(a) * r);
        p.y += e.baseY;
        break; }
    case MOV_LIBRE: {
        float a = t * e.vel * velFauna + e.fase;
        p = e.pos + glm::vec3(e.radio * sinf(a), sinf(t * e.oscVel + e.fase) * e.oscAmp, e.radio * 0.6f * sinf(2.0f * a)) + off;
        break; }
    case MOV_RUTA: {   // curva con frecuencias que no coinciden: casi nunca repite el mismo recorrido
        float a = t * fabsf(e.vel) * velFauna, R = e.radio;
        p = e.pos + glm::vec3(R * (0.85f * sinf(a + e.fase) + 0.35f * sinf(0.37f * a + 1.3f)),
                              e.oscAmp * (sinf(0.43f * a + 0.5f) + 0.35f * sinf(0.97f * a + 2.0f)),
                              R * (0.60f * cosf(0.71f * a + 1.7f * e.fase) + 0.30f * sinf(0.53f * a + 0.4f))) + off;
        break; }
    case MOV_BARCO: {
        float x = -110.0f + fmodf(e.reloj * e.vel, 220.0f);
        p = glm::vec3(x, e.pos.y + 0.12f * sinf(t * 0.8f + e.fase), e.pos.z);
        dir = glm::vec3(1.0f, 0.0f, 0.0f);
        break; }
    case MOV_ZAMBULLIDA: {
        const float Tj = e.vel > 0.1f ? e.vel : 1.3f, Tn = 8.0f, R = e.radio, h = e.oscAmp;
        float P = e.periodo < 2.0f * Tj + Tn + 4.0f ? 2.0f * Tj + Tn + 4.0f : e.periodo;
        float Ti = P - 2.0f * Tj - Tn;
        float tt = t + (float)k * P * 0.23f + e.fase * P / 6.2832f;
        float c = floorf(tt / P), s = tt - c * P;
        bool par = ((long)c % 2) == 0;
        float sg = par ? 1.0f : -1.0f;
        glm::vec3 d(cosf(e.rumbo), 0.0f, sinf(e.rumbo)), lat(-d.z, 0.0f, d.x);
        glm::vec3 A = e.pos + lat * ((float)k - 0.5f * (float)(e.miembros - 1)) * e.dispersion - lat * R;
        glm::vec3 base = par ? A : A + lat * (2.0f * R);
        float yAgua = e.nivelAgua - 0.35f;
        glm::vec3 W = base + d * e.alcance; W.y = yAgua;
        if (s < Ti) {
            float w = Suavizar(s / (0.4f * Ti));
            float ang = 3.14159f * w;
            dir = d * (-cosf(ang)) + lat * sg * sinf(ang);
            p = base; nado = 0.0f;
        } else if (s < Ti + Tj) {
            float u = (s - Ti) / Tj;
            p = base + d * (e.alcance * u);
            p.y = e.pos.y + (yAgua - e.pos.y) * u + h * 4.0f * u * (1.0f - u);
            nado = Suavizar(u * 1.6f);
        } else if (s < Ti + Tj + Tn) {
            float u = (s - Ti - Tj) / Tn, th = 3.14159f * u;
            p = W + lat * (sg * R * (1.0f - cosf(th))) + d * (R * sinf(th));
            p.y = yAgua - e.profundidad * sinf(th);
        } else {
            float u = (s - Ti - Tj - Tn) / Tj;
            glm::vec3 W2 = W + lat * (sg * 2.0f * R), B = base + lat * (sg * 2.0f * R);
            p = W2 + (B - W2) * u; p.y = yAgua + (e.pos.y - yAgua) * u + h * 4.0f * u * (1.0f - u);
            nado = 1.0f - Suavizar((u - 0.4f) * 1.8f);
        }
        break; }
    default: break;
    }
    return p;
}

inline glm::mat4 Escenario::Matriz(const Elemento& e, int idx, int k, float t) const
{
    glm::vec3 p = e.pos;
    float yawUsar = e.yaw, escUsar = e.escala, pitch = 0.0f, roll = 0.0f, nado = 0.0f, yawMov = 0.0f;
    glm::vec3 rotUsar = e.rotExtra;
    bool orientar = false;
    glm::vec3 off = k > 0 ? Aleatorio3(idx * 97 + k) * e.dispersion : glm::vec3(0.0f);
    switch (e.mov) {
    case MOV_FIJO:
        p.y += e.baseY + sinf(t * e.oscVel + e.fase) * e.oscAmp;
        if (k > 0) { p += glm::vec3(off.x, 0.0f, off.z); yawUsar += k * 137.5f; }   // copias esparcidas (monedas)
        break;
    case MOV_KF:
        if (e.kf) e.kf->PoseAt(t - e.kfInicio + (e.kf->loop ? e.fase : 0.0f), p, yawUsar, rotUsar, escUsar);
        break;
    case MOV_DERIVA:
        p += glm::vec3(sinf(t * 0.21f + e.fase) * 0.6f, sinf(t * 0.37f + e.fase) * 0.5f, cosf(t * 0.17f + e.fase) * 0.6f);
        p.y += e.baseY;
        break;
    case MOV_ORBITA: case MOV_CARDUMEN: case MOV_LIBRE: case MOV_RUTA: case MOV_BARCO: case MOV_ZAMBULLIDA: {
        glm::vec3 d0, d1, d2, q1, q2;
        float n1, n2;
        p = PosMovil(e, idx, k, t, nado, d0);
        q1 = PosMovil(e, idx, k, t + 0.12f, n1, d1);
        q2 = PosMovil(e, idx, k, t + 0.24f, n2, d2);
        glm::vec3 v1 = q1 - p, v2 = q2 - q1;
        float h1 = sqrtf(v1.x * v1.x + v1.z * v1.z), h2 = sqrtf(v2.x * v2.x + v2.z * v2.z);
        glm::vec3 dir = glm::dot(d0, d0) > 0.0f ? d0 : v1;
        if (glm::dot(dir, dir) < 1e-8f) dir = glm::vec3(1.0f, 0.0f, 0.0f);
        yawMov = atan2f(-dir.z, dir.x);
        if (e.mov == MOV_BARCO) {
            pitch = 0.015f * sinf(t * 0.7f + e.fase); roll = 0.025f * sinf(t * 0.9f);
        } else if (!e.pulsar && glm::dot(d0, d0) == 0.0f) {
            if (h1 > 1e-4f && (glm::dot(d0, d0) == 0.0f)) {
                pitch = atan2f(v1.y, h1); pitch = pitch > 0.6f ? 0.6f : (pitch < -0.6f ? -0.6f : pitch);
                if (e.mov != MOV_ZAMBULLIDA && e.mov != MOV_RUTA) pitch *= 0.6f;
                if (h2 > 1e-4f) {
                    float dy = atan2f(-v2.z, v2.x) - atan2f(-v1.z, v1.x);
                    while (dy > 3.14159f) dy -= 6.28318f;
                    while (dy < -3.14159f) dy += 6.28318f;
                    roll = -dy / 0.12f * 0.45f; roll = roll > 0.4f ? 0.4f : (roll < -0.4f ? -0.4f : roll);
                }
            }
            if (nado > 0.5f && e.mov != MOV_ZAMBULLIDA) yawMov += 0.07f * sinf(t * e.velNado * 0.6f + e.fase + k);
        }
        orientar = true;
        break; }
    }
    glm::mat4 M = glm::translate(glm::mat4(1.0f), p);
    if (orientar) {
        M = glm::rotate(M, yawMov, glm::vec3(0.0f, 1.0f, 0.0f));
        if (pitch != 0.0f) M = glm::rotate(M, pitch, glm::vec3(0, 0, 1));
        if (roll != 0.0f) M = glm::rotate(M, roll, glm::vec3(1, 0, 0));
        if (e.mov == MOV_ZAMBULLIDA) rotUsar = glm::mix(e.rotExtra, e.rotNado, nado);
    }
    else M = glm::rotate(M, glm::radians(yawUsar), glm::vec3(0.0f, 1.0f, 0.0f));
    if (rotUsar.x != 0.0f) M = glm::rotate(M, glm::radians(rotUsar.x), glm::vec3(1, 0, 0));
    if (rotUsar.y != 0.0f) M = glm::rotate(M, glm::radians(rotUsar.y), glm::vec3(0, 1, 0));
    if (rotUsar.z != 0.0f) M = glm::rotate(M, glm::radians(rotUsar.z), glm::vec3(0, 0, 1));
    if (e.mov == MOV_DERIVA) M = glm::rotate(M, t * 0.2f + e.fase, glm::normalize(glm::vec3(0.3f, 1.0f, 0.5f)));
    float sc = escUsar * (k > 0 ? 0.85f + 0.3f * Aleatorio(idx, k + 50) : 1.0f) * (1.0f + 0.8f * e.inflado);
    glm::vec3 sv(sc);
    if (e.pulsar) {   // la campana se contrae (mas angosta y alta) y se relaja
        float pu = sinf(t * 2.2f + e.fase + k * 1.3f);
        sv = glm::vec3(sc * (1.0f - 0.10f * pu), sc * (1.0f + 0.12f * pu), sc * (1.0f - 0.10f * pu));
    }
    return glm::scale(M, sv);
}

#endif
