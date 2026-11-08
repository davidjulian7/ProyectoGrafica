#ifndef RENDER_H
#define RENDER_H
// ============================================================================================
// ESCENA · Render (practicas 01 y 02): orden en que se dibuja cada cuadro.
//   1) agua de fondo   2) terreno   3) objetos opacos   4) superficie del agua   5) transparentes y burbujas
// [CAMBIAR] el orden o agregar un pase nuevo (por ejemplo un objeto con otro shader) dentro de Dibujar().
// ============================================================================================

inline void Escenario::Dibujar(const glm::mat4& proj, const glm::mat4& view, const Camera& cam, float t, int fbW, int fbH, const Camera* camBuzo, bool mostrarBuzo)
{
    ZonaConfig cfgGuardada = cfg;
    bool arriba = zona == 2 && cam.Position.y > cfg.superficieY;   // sobre el hielo: aire, cielo y mar
    if (arriba) {
        cfg.niebla = glm::vec3(0.66f, 0.80f, 0.90f); cfg.densidad = 0.0045f; cfg.causticas = 0.0f;
        cfg.ambArriba = glm::vec3(0.62f, 0.70f, 0.80f); cfg.ambAbajo = glm::vec3(0.34f, 0.38f, 0.44f);
        cfg.solColor = glm::vec3(1.05f, 1.0f, 0.92f);
    }
    glClearColor(cfg.niebla.r, cfg.niebla.g, cfg.niebla.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    float profundidad = cfg.superficieY - cam.Position.y;

    // 1) Agua de fondo (direccion de vista -> color del agua, sol y rayos)
    glDepthMask(GL_FALSE);
    shFondo->use();
    glm::mat4 vistaRot = glm::mat4(glm::mat3(view));
    shFondo->setMat4("uInvVP", glm::inverse(proj * vistaRot));
    shFondo->setVec2("uRes", glm::vec2((float)fbW, (float)fbH));
    shFondo->setVec3("uNiebla", cfg.niebla);
    shFondo->setVec3("uSolDir", glm::normalize(cfg.solDir));
    shFondo->setVec3("uSolColor", cfg.solColor);
    shFondo->setFloat("uTiempo", t);
    shFondo->setFloat("uProfCam", profundidad);
    shFondo->setInt("uArriba", arriba ? 1 : 0);
    glBindVertexArray(vaoVacio); glDrawArrays(GL_TRIANGLES, 0, 3); glBindVertexArray(0);
    glDepthMask(GL_TRUE);

    // parametros del descarte por campo de vision y niebla
    cullPos = cam.Position; cullFront = cam.Front;
    float tanY = tanf(glm::radians(cam.Zoom) * 0.5f), aspecto = (float)fbW / (float)(fbH > 0 ? fbH : 1);
    cullAngulo = atanf(tanY * sqrtf(1.0f + aspecto * aspecto));
    cullDist = 3.3f / (cfg.densidad > 0.001f ? cfg.densidad : 0.001f);

    // 2) Terreno y elementos opacos
    glDisable(GL_BLEND);
    sh->use();
    ConfigurarShader(proj, view, cam, t, camBuzo);
    Comunes(glm::vec3(1.0f), 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    sh->setMat4("model", glm::mat4(1.0f));
    terreno->Draw(*sh);
    for (size_t i = 0; i < elementos.size(); i++) {
        Elemento& e = elementos[i];
        if (!e.visible || e.alpha < 1.0f) continue;
        if (InspeccionActiva() && (int)i == inspeccionIdx) continue;
        DibujarElemento(e, (int)i, t);
    }

    // El jugador comparte la pose de la camara de primera persona en todas las zonas.
    // Se oculta desde sus ojos para evitar que la mascara tape la vista.
    if (mostrarBuzo && camBuzo && buzoModelo) {
        Comunes(glm::vec3(1.0f), 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
        float dtB = t - buzoUltimoT; buzoUltimoT = t;
        if (dtB < 0.0f || dtB > 0.2f) { dtB = 0.016f; buzoFrente = camBuzo->Front; }
        // el cuerpo sigue a la mirada con retraso, asi no gira de golpe
        glm::vec3 objF = camBuzo->Front;
        if (buzoDePie) { objF.y = 0.0f; objF = glm::length(objF) > 0.01f ? glm::normalize(objF) : buzoFrente; }
        buzoFrente = glm::normalize(glm::mix(buzoFrente, objF, 1.0f - expf(-5.0f * dtB)));
        float vel = glm::length(buzoVel);
        buzoVelSuave += (vel - buzoVelSuave) * (1.0f - expf(-4.0f * dtB));
        buzoFase += dtB * (1.6f + buzoVelSuave * 0.9f);
        float mov = buzoVelSuave > 4.0f ? 1.0f : buzoVelSuave * 0.25f;
        glm::vec3 der = glm::normalize(glm::cross(buzoFrente, glm::vec3(0.0f, 1.0f, 0.0f)));
        glm::vec3 arr = glm::cross(der, buzoFrente);
        glm::mat4 pose(1.0f);
        pose[0] = glm::vec4(der, 0.0f);
        pose[1] = glm::vec4(arr, 0.0f);
        pose[2] = glm::vec4(-buzoFrente, 0.0f);
        pose[3] = glm::vec4(camBuzo->Position + camBuzo->BobOffset + arr * (0.05f * sinf(buzoFase * 2.0f) * (0.4f + mov)), 1.0f);
        if (!buzoDePie) pose = glm::rotate(pose, -0.85f * mov, glm::vec3(1.0f, 0.0f, 0.0f));   // se inclina hacia adelante al nadar
        pose = glm::rotate(pose, 0.10f * sinf(buzoFase) * (0.3f + mov), glm::vec3(0.0f, 0.0f, 1.0f));   // balanceo de aleteo
        pose = glm::rotate(pose, 0.06f * sinf(buzoFase * 2.0f + 1.0f) * mov, glm::vec3(1.0f, 0.0f, 0.0f));
        // Modelo original: Z arriba, cara hacia -Y; la mascara queda en el origen del jugador.
        pose = glm::rotate(pose, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        pose = glm::rotate(pose, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        pose = glm::translate(pose, glm::vec3(0.0f, 0.25f, -0.72f));
        sh->setMat4("model", pose);
        buzoModelo->Draw(*sh);
    }

    // monedas voladoras: oro metalico con destellos
    if (modeloMoneda && !monedasVuelo.empty()) {
        Comunes(glm::vec3(1.15f, 0.92f, 0.45f), 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
        sh->setFloat("uSpecPower", 90.0f); sh->setFloat("uSpecInt", 1.6f); sh->setFloat("uMetalico", 1.0f);
        for (const MonedaVoladora& m : monedasVuelo) {
            if (!EnVista(m.p, 0.2f)) continue;
            sh->setFloat("uEmision", 0.25f + 0.35f * (0.5f + 0.5f * sinf(t * 7.0f + m.brillo)));
            glm::mat4 mm = glm::translate(glm::mat4(1.0f), m.p);
            if (m.quieta) mm = glm::rotate(mm, m.brillo, glm::vec3(0.0f, 1.0f, 0.0f));
            else mm = glm::rotate(mm, m.ang, m.eje);
            sh->setMat4("model", glm::scale(mm, glm::vec3(2.4f)));
            modeloMoneda->Draw(*sh);
        }
    }

    // 3) Superficie del agua vista desde abajo (Fresnel + ventana de Snell)
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    shSuperficie->use();
    glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(cam.Position.x, cfg.superficieY, cam.Position.z));
    m = glm::scale(m, glm::vec3(400.0f, 1.0f, 400.0f));
    shSuperficie->setMat4("model", m);
    shSuperficie->setMat4("view", view);
    shSuperficie->setMat4("projection", proj);
    shSuperficie->setVec3("uCamPos", cam.Position);
    shSuperficie->setVec3("uNiebla", cfg.niebla);
    shSuperficie->setFloat("uDensidad", cfg.densidad);
    shSuperficie->setVec3("uSolDir", glm::normalize(cfg.solDir));
    shSuperficie->setVec3("uSolColor", cfg.solColor);
    shSuperficie->setFloat("uTiempo", t);
    shSuperficie->setInt("uCubo", 1);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_CUBE_MAP, cubo); glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vaoQuad); glDrawArrays(GL_TRIANGLES, 0, 6); glBindVertexArray(0);

    // 4) Transparentes: elementos semitransparentes y burbujas
    sh->use();
    glDepthMask(GL_FALSE);
    for (size_t i = 0; i < elementos.size(); i++) {
        Elemento& e = elementos[i];
        if (InspeccionActiva() && (int)i == inspeccionIdx) continue;
        if (e.visible && e.alpha < 1.0f) DibujarElemento(e, (int)i, t);
    }
    Comunes(glm::vec3(0.85f, 0.95f, 1.0f), 0.0f, 0.0f, 0.0f, 0.0f, 0.45f, 0.35f);
    sh->setFloat("uSpecPower", 64.0f);
    sh->setFloat("uSpecInt", 0.45f);
    sh->setFloat("uMetalico", 0.0f);
    for (const Burbuja& b : burbujas) {
        if (!EnVista(b.p, b.r * 2.0f)) continue;
        glm::mat4 mb = glm::translate(glm::mat4(1.0f), b.p);
        mb = glm::scale(mb, glm::vec3(b.r * 2.0f));
        sh->setMat4("model", mb);
        burbujaModelo->Draw(*sh);
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    cfg = cfgGuardada;
}

inline void Escenario::DibujarInspeccion(const glm::mat4& proj, const glm::mat4& view, const Camera& cam, float t)
{
    if (!InspeccionActiva()) return;
    Elemento& e = elementos[inspeccionIdx];
    glClear(GL_DEPTH_BUFFER_BIT);
    glDisable(GL_BLEND);
    sh->use();
    ConfigurarShader(proj, view, cam, t, &cam);
    Comunes(e.tinte, e.ondula, e.medioLargo, e.mecer, e.fase, e.alpha, e.emision);
    sh->setFloat("uSpecPower", e.specPower);
    sh->setFloat("uSpecInt", e.specInt);
    sh->setFloat("uMetalico", e.metalico);
    sh->setFloat("uBlanqueo", e.blanqueo);
    sh->setFloat("uMuerte", e.muerte);
    sh->setInt("uOndulaEje", e.ondulaVertical ? 1 : 0);
    sh->setFloat("uFantasma", e.fantasma);
    glm::mat4 pose = Matriz(e, inspeccionIdx, 0, t);
    glm::mat4 rotacion(1.0f);
    rotacion = glm::rotate(rotacion, glm::radians(giroInspeccion), cam.Up);
    rotacion = glm::rotate(rotacion, glm::radians(inclinacionInspeccion), cam.Right);
    float diametro = 2.0f * e.radioBase * e.escala;
    float ajuste = 2.2f / (diametro > 0.05f ? diametro : 0.05f);
    glm::vec3 centro = cam.Position + cam.Front * 3.8f + cam.Up * 0.05f;
    glm::mat4 modelo = glm::translate(glm::mat4(1.0f), centro) * rotacion
                     * glm::mat4(glm::mat3(pose)) * glm::scale(glm::mat4(1.0f), glm::vec3(ajuste * zoomInspeccion));
    sh->setMat4("model", modelo);
    e.modelo->Draw(*sh, e.modelo->TieneAnimacion() ? t : -1.0f);
}

inline void Escenario::DibujarElemento(const Elemento& e, int idx, float t)
{
    glm::vec3 tinte = e.tinte;
    if (e.camuflaje) {   // el pulpo alterna entre rojo, arena y moteado oscuro
        float c = 0.5f + 0.5f * sinf(t * 0.35f + e.fase);
        tinte = glm::mix(glm::vec3(1.25f, 0.95f, 0.85f), glm::vec3(0.75f, 0.55f, 0.45f), c);
    }
    Comunes(tinte, e.ondula, e.medioLargo, e.mecer, e.fase, e.alpha, e.emision);
    sh->setFloat("uSpecPower", e.specPower);
    sh->setFloat("uSpecInt", e.specInt);
    sh->setFloat("uMetalico", e.metalico);
    sh->setFloat("uVelNado", e.velNado);
    sh->setFloat("uBlanqueo", e.blanqueo);
    sh->setFloat("uMuerte", e.muerte);
    sh->setInt("uOndulaEje", e.ondulaVertical ? 1 : 0);
    sh->setFloat("uFantasma", e.fantasma);
    sh->setFloat("uResaltado", idx == seleccion ? 0.25f + 0.15f * sinf(t * 6.0f) : 0.0f);
    sh->setFloat("uVelNado", e.velNado * (0.4f + 0.6f * velFauna));
    for (int k = 0; k < Visibles(e); k++) {
        glm::mat4 M = Matriz(e, idx, k, t);
        if (!EnVista(glm::vec3(M[3]), e.radioBase * EscalaMax(M) * 1.15f)) continue;
        sh->setFloat("uFase", e.fase + k * 1.3f);
        sh->setMat4("model", M);
        e.modelo->Draw(*sh, e.modelo->TieneAnimacion() ? t + e.fase + k * 0.37f : -1.0f);
    }
}

#endif
