#ifndef LUCES_Y_MATERIALES_H
#define LUCES_Y_MATERIALES_H
// ============================================================================================
// ILUMINACION Y MATERIALES (practicas 07, 08 y 14: Phong, varias luces, materiales y Fresnel)
// Aqui se envian al shader de las zonas (bin/shaders/zona.fs):
//   - ConfigurarShader(): luz del sol, luz ambiental, niebla, causticas y la LINTERNA del buzo.
//   - Comunes(): datos de material de cada objeto (tinte, transparencia, brillo, emision).
//   - MatOpaco / MatPlastico / MatTraslucido / MatMetalico: los 4 tipos de material.
// Los valores de luz de cada zona (color del sol, niebla...) estan en zonas/<zona>/<zona>_escena.h (cfg.*).
// La formula de Phong y Fresnel esta en bin/shaders/zona.fs.
// ============================================================================================

inline void Escenario::ConfigurarShader(const glm::mat4& proj, const glm::mat4& view, const Camera& cam, float t, const Camera* camBuzo)
{
    sh->setMat4("projection", proj);
    sh->setMat4("view", view);
    sh->setVec3("uCamPos", cam.Position);
    // [CAMBIAR] la luz del sol se toma de cfg (direccion, color y luz ambiental). Sus valores estan en
    //            zonas/<zona>/<zona>_escena.h. Para un cambio en TODAS las zonas multiplica aqui, ej: cfg.solColor * 1.5f
    sh->setVec3("uSolDir", glm::normalize(cfg.solDir));
    sh->setVec3("uSolColor", cfg.solColor);
    sh->setVec3("uAmbArriba", cfg.ambArriba);
    sh->setVec3("uAmbAbajo", cfg.ambAbajo);
    sh->setVec3("uNiebla", cfg.niebla);
    sh->setFloat("uDensidad", cfg.densidad);
    sh->setFloat("uCausticas", cfg.causticas);
    sh->setFloat("uSuperficieY", cfg.superficieY);
    sh->setFloat("uTiempo", t);
    // [CAMBIAR] LINTERNA (luz puntual tipo reflector): su color, alcance y apertura estan en bin/shaders/zona.fs
    sh->setInt("uLinterna", linterna ? 1 : 0);
    // La linterna sale siempre del buzo (1a persona), aunque se mire en órbita
    const Camera& luz = camBuzo ? *camBuzo : cam;
    sh->setVec3("uLinternaPos", luz.Position - luz.Up * 0.25f + luz.Right * 0.2f);
    sh->setVec3("uLinternaDir", luz.Front);
}

inline void Escenario::Comunes(const glm::vec3& tinte, float ondula, float largo, float mecer, float fase, float alpha, float emision)
{
    sh->setVec3("uTinte", tinte);
    sh->setFloat("uOndula", ondula);
    sh->setFloat("uLargo", largo);
    sh->setFloat("uVelNado", 6.0f);
    sh->setFloat("uMecer", mecer);
    sh->setFloat("uFase", fase);
    sh->setFloat("uAlpha", alpha);
    sh->setFloat("uEmision", emision);
    sh->setFloat("uResaltado", 0.0f);
    sh->setFloat("uBlanqueo", 0.0f);
    sh->setFloat("uMuerte", 0.0f);
    sh->setFloat("uFantasma", 0.0f);
    sh->setInt("uOndulaEje", 0);
    sh->setInt("uSkinning", 0);
    sh->setFloat("uSpecPower", 40.0f);
    sh->setFloat("uSpecInt", 0.25f);
    sh->setFloat("uMetalico", 0.0f);
}

// [CAMBIAR] MATERIALES: specInt = intensidad del brillo, specPower = que tan concentrado es el brillo,
//            metalico = 1 tine el brillo con el color del objeto (metal). Para aplicar uno: MatMetalico(e);
inline void Escenario::MatOpaco(Elemento& e)
{ e.specInt = 0.08f; e.specPower = 24.0f; e.metalico = 0.0f; }

inline void Escenario::MatPlastico(Elemento& e)
{ e.specInt = 0.60f; e.specPower = 32.0f; e.metalico = 0.0f; }

inline void Escenario::MatTraslucido(Elemento& e)
{ e.specInt = 0.30f; e.specPower = 64.0f; e.metalico = 0.0f; if (e.alpha >= 1.0f) e.alpha = 0.75f; }

inline void Escenario::MatMetalico(Elemento& e)
{ e.specInt = 1.00f; e.specPower = 96.0f; e.metalico = 1.0f; }

#endif
