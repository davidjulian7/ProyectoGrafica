#ifndef PARTICULAS_ZONA_H
#define PARTICULAS_ZONA_H
// (cabecera)

inline bool Escenario::Actualizar(float dt, float t, const Camera& cam)
{
    if (dt > 0.1f) dt = 0.1f;
    tiempoJuego = t;
    bool exhalo = false;
    for (MonedaVoladora& m : monedasVuelo) {
        if (m.quieta) continue;
        m.v.y -= 2.4f * dt;                         // bajo el agua caen despacio
        m.v *= expf(-0.7f * dt);
        m.p += m.v * dt; m.ang += m.vAng * dt;
        float suelo = AlturaSuelo(m.p.x, m.p.z) + 0.02f;
        if (m.p.y <= suelo && m.v.y < 0.0f) {      // al tocar el fondo quedan acostadas
            m.p.y = suelo; m.quieta = true; m.eje = glm::vec3(0.0f, 1.0f, 0.0f);
        }
    }
    for (size_t i = 0; i < elementos.size(); i++) {
        Elemento& e = elementos[i];
        if (!e.inflable) continue;
        glm::vec3 p = glm::vec3(Matriz(e, (int)i, 0, t)[3]);
        float objetivo = glm::length(p - cam.Position) < 3.5f ? 1.0f : 0.0f;
        e.inflado += (objetivo - e.inflado) * (dt * (objetivo > e.inflado ? 4.0f : 0.8f));
    }
    // respiracion del buzo: cada 4.5 s sale una nube de burbujas frente a la mascara
    respiracion += dt;
    if (respiracion > 4.5f) {
        respiracion = 0.0f;
        exhalo = true;
        // salen por debajo de la mascara y suben frente a la vista
        // el regulador expulsa el aire a los costados de la mascara
        for (int k = 0; k < 12; k++) {
            float lado = (k % 2 == 0 ? 1.0f : -1.0f) * (0.45f + 0.25f * Aleatorio(k, 11));
            glm::vec3 p = cam.Position + cam.Front * (0.5f + 0.4f * Aleatorio(k, 13)) + cam.Right * lado - cam.Up * 0.5f
                        + Aleatorio3(k + (int)(t * 10)) * 0.08f;
            burbujas.push_back({ p, 0.9f + Aleatorio(k, 3) * 0.6f, 0.006f + Aleatorio(k, 5) * 0.010f, Aleatorio(k, 9) * 6.0f });
        }
    }
    // fuentes de burbujas de la zona (corales, fugas del submarino)
    fuenteTiempo += dt;
    if (fuenteTiempo > 0.25f) {
        fuenteTiempo = 0.0f;
        for (size_t f = 0; f < fuentes.size(); f++) {
            int k = (int)(t * 13.0f) + (int)f * 31;
            burbujas.push_back({ fuentes[f] + Aleatorio3(k) * 0.3f, 0.5f + Aleatorio(k, 3) * 0.5f, 0.04f + Aleatorio(k, 5) * 0.08f, Aleatorio(k, 7) * 6.0f });
        }
    }
    for (size_t i = 0; i < burbujas.size(); ) {
        Burbuja& b = burbujas[i];
        b.p.y += b.vel * dt;
        b.p.x += sinf(t * 2.3f + b.fase) * 0.15f * dt;
        b.p.z += cosf(t * 1.9f + b.fase) * 0.15f * dt;
        b.r *= 1.0f + 0.03f * dt;       // crecen al subir (menos presion)
        if (b.p.y > cfg.superficieY - 0.05f) { burbujas[i] = burbujas.back(); burbujas.pop_back(); }
        else i++;
    }
    return exhalo;
}

inline void Escenario::LanzarMonedas(const glm::vec3& origen, int n)
{
    if (!modeloMoneda) modeloMoneda = Cargar("moneda", "naufragios");
    for (int i = 0; i < n; i++) {
        glm::vec3 r = Aleatorio3(i * 7 + (int)monedasVuelo.size());
        float ang = Aleatorio(i, 31) * 6.2832f, abre = 0.6f + 1.8f * Aleatorio(i, 32);
        MonedaVoladora m;
        m.p = origen + glm::vec3(r.x * 0.25f, Aleatorio(i, 33) * 0.2f, r.z * 0.15f);
        m.v = glm::vec3(cosf(ang) * abre, 3.2f + 2.6f * Aleatorio(i, 34), sinf(ang) * abre + 0.6f);
        m.eje = glm::normalize(glm::vec3(r.x, 0.3f + fabsf(r.y), r.z) + glm::vec3(0.001f));
        m.ang = 0.0f; m.vAng = 6.0f + 10.0f * Aleatorio(i, 35); m.brillo = Aleatorio(i, 36) * 6.28f; m.quieta = false;
        monedasVuelo.push_back(m);
    }
}

#endif
