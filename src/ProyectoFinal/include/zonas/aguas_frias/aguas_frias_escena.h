#ifndef AGUAS_FRIAS_ESCENA_H
#define AGUAS_FRIAS_ESCENA_H
// ============================================================================================
// ZONA 2 · AGUAS FRIAS · Que objetos hay y donde estan (ConstruirAguasFrias).
// [CAMBIAR] luz, niebla y color del agua: bloque cfg.* al inicio de ConstruirAguasFrias().
// [AGREGAR] un modelo: copia una linea Fijo(...) o Nadador(...) existente y cambia archivo y posicion.
// ============================================================================================

inline void Escenario::ConstruirAguasFrias()
{
    cfg = ZonaConfig();
    cfg.nombre = "Aguas Frias";
    // [CAMBIAR] AMBIENTE DE LA ZONA: niebla = color del agua (R,G,B de 0 a 1); densidad = que tan turbia (0.01 clara, 0.05 turbia);
    //            solColor = color e intensidad del sol; solDir = direccion de la luz; ambArriba/ambAbajo = luz ambiental;
    //            causticas = reflejos de luz en el fondo (0 a 1); superficieY = altura de la superficie; inicioCamara = donde aparece el buzo.
    cfg.niebla = glm::vec3(0.20f, 0.34f, 0.44f);
    cfg.densidad = 0.028f;
    cfg.solColor = glm::vec3(0.85f, 0.93f, 1.0f) * 0.95f;
    cfg.solDir = glm::vec3(0.2f, -1.0f, 0.35f);
    cfg.ambArriba = glm::vec3(0.40f, 0.50f, 0.60f);
    cfg.ambAbajo = glm::vec3(0.10f, 0.14f, 0.20f);
    cfg.causticas = 0.3f;
    cfg.superficieY = 16.0f;
    cfg.inicioCamara = glm::vec3(0.0f, 8.0f, 24.0f);
    terrenoArchivo = "terreno_aguas_frias"; terrenoCarpeta = "aguas_frias";
    const char* c = "aguas_frias";

    Elemento roca = Fijo("roca_fria", c, glm::vec3(0.0f, -0.5f, 0.0f), 0, 1); roca.etiqueta = "Roca";
    Esparcir(roca, 16, glm::vec2(0.0f), 3.0f, 34.0f, 0.8f, 2.8f, 12);
    // plataformas de hielo: el buzo puede subirse; los animales descansan, se lanzan al agua y regresan
    const float sy = cfg.superficieY;
    auto hacerPlataforma = [&](const char* archivo, float x, float z, float esc, float yaw) {
        Elemento h = Fijo(archivo, c, glm::vec3(x, sy + 0.7f, z), yaw, esc);
        h.sobreSuelo = false; h.interactivo = true; h.etiqueta = "Plataforma de hielo"; h.caminable = true;
        h.tinte = glm::vec3(0.85f, 0.95f, 1.05f); h.emision = 0.55f;   // luz que atraviesa el hielo
        MatTraslucido(h); h.alpha = 0.9f;
        elementos.push_back(h);
    };
    // habitante de una plataforma: n individuos que se turnan para zambullirse hacia el centro de la zona
    auto habitante = [&](const char* archivo, const char* etiqueta, float x, float z, float esc, int n, float disp, float radio, float periodo, float fase, float salto, float altura) {
        Elemento a = Nadador(archivo, c, MOV_ZAMBULLIDA, glm::vec3(x, sy + 0.7f, z), radio, salto, etiqueta);
        a.sobreSuelo = false; a.sobreHielo = true; a.miembros = n; a.dispersion = disp; a.periodo = periodo; a.fase = fase;
        a.rumbo = atan2f(-z, -x); a.alcance = 5.0f * esc + 1.5f; a.profundidad = 2.6f; a.nivelAgua = sy; a.oscAmp = altura;
        a.ondula = 0.0f; a.velNado = 3.0f;
        elementos.push_back(a);
        return elementos.size() - 1;
    };
    hacerPlataforma("hielo1", 18.0f, 24.0f, 1.4f, 20.0f);
    hacerPlataforma("hielo2", -6.0f, 26.0f, 1.2f, 100.0f);
    hacerPlataforma("hielo1", -30.0f, 18.0f, 1.4f, 200.0f);
    hacerPlataforma("hielo2", 28.0f, -24.0f, 1.2f, 40.0f);
    hacerPlataforma("hielo1", -10.0f, -26.0f, 1.3f, 300.0f);
    hacerPlataforma("hielo2", 8.0f, -32.0f, 1.0f, 150.0f);

    // la cueva de hielo: un oso polar vigila desde la cima
    {
        Elemento cueva = Fijo("cueva_meshy", c, glm::vec3(-36.0f, sy + 2.6f, -26.0f), 30.0f, 8.0f);
        cueva.sobreSuelo = false; cueva.interactivo = true; cueva.etiqueta = "Cueva de hielo"; cueva.caminable = true;
        cueva.specInt = 0.35f; cueva.specPower = 48.0f;
        elementos.push_back(cueva);
        Elemento oso2 = Fijo("osito", c, glm::vec3(-36.0f, sy + 6.0f, -26.0f + 2.3f), 200.0f, 1.0f);
        oso2.sobreSuelo = false; oso2.sobreHielo = true; oso2.interactivo = true; oso2.etiqueta = "Oso polar";
        elementos.push_back(oso2);
    }
    // icebergs de Meshy (la mayor parte queda bajo el agua)
    {
        Elemento ice = Fijo("iceberg_meshy", c, glm::vec3(38.0f, sy, 30.0f), 30.0f, 11.0f);
        ice.sobreSuelo = false; ice.etiqueta = "Iceberg"; ice.interactivo = true;
        ice.tinte = glm::vec3(0.9f, 0.98f, 1.05f); ice.emision = 0.3f; ice.specInt = 0.35f; ice.specPower = 64.0f;
        elementos.push_back(ice);
        ice.pos = glm::vec3(-12.0f, sy, -48.0f); ice.yaw = 140.0f; ice.escala = 9.0f; elementos.push_back(ice);
    }

    // rompehielos: cruza la zona de lado a lado por un canal libre de hielo
    {
        Elemento barco = Fijo("rompehielos", c, glm::vec3(0.0f, sy + 0.31f * 11.0f, 0.0f), 0.0f, 11.0f);
        barco.sobreSuelo = false; barco.interactivo = true; barco.etiqueta = "Rompehielos";
        barco.mov = MOV_BARCO; barco.vel = 3.2f; barco.reloj = 25.0f; barco.rotExtra = glm::vec3(0.0f, 180.0f, 0.0f);
        barco.specInt = 0.5f; barco.specPower = 40.0f; barco.metalico = 0.3f;
        elementos.push_back(barco);
    }

    Elemento ballena = Nadador("ballena", c, MOV_RUTA, glm::vec3(0.0f, 9.0f, -4.0f), 30.0f, 0.06f, "Ballena azul");
    ballena.ondula = 0.25f; ballena.velNado = 1.5f; ballena.oscAmp = 3.5f; ballena.ondulaVertical = true;
    elementos.push_back(ballena);
    Elemento belugas = Nadador("beluga", c, MOV_CARDUMEN, glm::vec3(0.0f, 7.0f, 4.0f), 15.0f, -0.12f, "Beluga");
    belugas.miembros = 3; belugas.dispersion = 2.0f; belugas.ondula = 0.12f; belugas.velNado = 3.0f;
    elementos.push_back(belugas);

    // animales del hielo (cada plataforma tiene su especie)
    {
        size_t i0 = habitante("osito", "Oso polar", 18.0f, 24.0f, 1.4f, 1, 0.0f, 2.8f, 40.0f, 0.0f, 2.4f, 0.7f);
        elementos[i0].escala = 1.1f;
        size_t i1 = habitante("pinguino", "Pinguino", -6.0f, 26.0f, 1.2f, 5, 1.1f, 2.4f, 22.0f, 1.0f, 1.2f, 1.9f);
        elementos[i1].rotNado = glm::vec3(0.0f, 0.0f, -90.0f); elementos[i1].escala = 1.5f;
        habitante("foca", "Foca", -30.0f, 18.0f, 1.4f, 2, 1.5f, 2.6f, 30.0f, 2.2f, 1.6f, 0.9f);
        size_t i3 = habitante("morsa", "Morsa", 28.0f, -24.0f, 1.2f, 2, 2.0f, 2.8f, 46.0f, 3.1f, 2.4f, 0.5f);
        elementos[i3].escala = 1.1f;
        habitante("LeonMarino2", "Leon marino", -10.0f, -26.0f, 1.3f, 2, 1.8f, 2.6f, 26.0f, 4.0f, 1.3f, 1.4f);
        size_t i5 = habitante("pinguino", "Pinguino", 8.0f, -32.0f, 1.0f, 4, 1.0f, 2.2f, 24.0f, 5.0f, 1.2f, 1.9f);
        elementos[i5].rotNado = glm::vec3(0.0f, 0.0f, -90.0f); elementos[i5].escala = 1.5f;
    }
    // focas que nadan en mar abierto, lentas
    Elemento focas = Nadador("foca", c, MOV_CARDUMEN, glm::vec3(4.0f, 9.0f, -4.0f), 11.0f, 0.13f, "Foca");
    focas.miembros = 3; focas.dispersion = 1.8f; focas.ondula = 0.1f; focas.velNado = 2.5f; focas.oscAmp = 0.9f;
    elementos.push_back(focas);
    Elemento krill = Nadador("Krill", c, MOV_CARDUMEN, glm::vec3(-4.0f, 6.0f, -6.0f), 5.0f, 0.25f, "Krill");
    krill.miembros = 60; krill.dispersion = 2.5f; krill.escala = 1.4f; krill.ondula = 0.0f; krill.oscAmp = 0.6f;
    elementos.push_back(krill);
    Elemento bacalaos = Nadador("bacalao", c, MOV_CARDUMEN, glm::vec3(6.0f, 3.5f, 6.0f), 8.0f, -0.3f, "Bacalao");
    bacalaos.miembros = 9; bacalaos.dispersion = 1.6f; bacalaos.sobreSuelo = true; elementos.push_back(bacalaos);
    Elemento pescados = Nadador("pescado", "arrecifes", MOV_CARDUMEN, glm::vec3(-10.0f, 5.0f, 8.0f), 6.0f, 0.3f, "Pescado");
    pescados.miembros = 5; pescados.dispersion = 1.4f; elementos.push_back(pescados);

    // Orca2, Narwhal2, Cachalote2 y LeonMarino2 ya vienen con Y arriba (exporta_al_juego.py): sin rotExtra
    Elemento orcas = Nadador("Orca2", c, MOV_CARDUMEN, glm::vec3(-12.0f, 7.0f, -14.0f), 18.0f, 0.12f, "Orca");
    orcas.miembros = 3; orcas.dispersion = 3.0f; orcas.ondula = 0.08f; orcas.velNado = 4.0f; orcas.oscAmp = 0.8f;
    elementos.push_back(orcas);
    Elemento narvales = Nadador("Narwhal2", c, MOV_CARDUMEN, glm::vec3(14.0f, 9.0f, -18.0f), 12.0f, -0.10f, "Narval");
    narvales.miembros = 2; narvales.dispersion = 2.5f; narvales.ondula = 0.08f; narvales.velNado = 4.0f; narvales.oscAmp = 0.6f;
    elementos.push_back(narvales);
    Elemento cachalote = Nadador("Cachalote2", c, MOV_LIBRE, glm::vec3(0.0f, 5.0f, -22.0f), 22.0f, 0.05f, "Cachalote");
    cachalote.tinte = glm::vec3(0.55f, 0.65f, 0.8f); cachalote.ondula = 0.10f; cachalote.velNado = 2.5f; cachalote.oscAmp = 1.2f; cachalote.oscVel = 0.2f;
    elementos.push_back(cachalote);
    Elemento calamar = Nadador("Calamar2", c, MOV_ORBITA, glm::vec3(-14.0f, 3.0f, 10.0f), 8.0f, 0.08f, "Calamar colosal");
    calamar.ondula = 0.15f; calamar.velNado = 3.0f; calamar.oscAmp = 1.0f;
    elementos.push_back(calamar);
    Elemento leones = Nadador("LeonMarino2", c, MOV_CARDUMEN, glm::vec3(10.0f, 10.0f, 14.0f), 9.0f, 0.30f, "Leon marino");
    leones.miembros = 3; leones.dispersion = 1.8f; leones.ondula = 0.10f; leones.velNado = 6.0f; leones.oscAmp = 1.0f;
    elementos.push_back(leones);
    Elemento dracoides = Nadador("pezdracoide", "arrecifes", MOV_CARDUMEN, glm::vec3(-2.0f, 4.0f, 14.0f), 6.0f, 0.35f, "Pez dracoide");
    dracoides.miembros = 6; dracoides.dispersion = 1.4f; dracoides.tinte = glm::vec3(0.75f, 0.9f, 1.1f);
    elementos.push_back(dracoides);
    fuentes.clear();
}

#endif
