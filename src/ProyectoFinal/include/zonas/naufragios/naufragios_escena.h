#ifndef NAUFRAGIOS_ESCENA_H
#define NAUFRAGIOS_ESCENA_H
// ============================================================================================
// ZONA 3 · NAUFRAGIOS · Que objetos hay y donde estan (ConstruirNaufragios).
// [CAMBIAR] luz, niebla y color del agua: bloque cfg.* al inicio (el agua se aclara al limpiar: ver la mecanica).
// [AGREGAR] basura: copia una linea residuo("archivo", "Etiqueta", tipo, peso, cantidad, ...).
// ============================================================================================

inline void Escenario::ConstruirNaufragios()
{
    cfg = ZonaConfig();
    cfg.nombre = "Naufragios";
    // [CAMBIAR] AMBIENTE DE LA ZONA: niebla = color del agua (R,G,B de 0 a 1); densidad = que tan turbia (0.01 clara, 0.05 turbia);
    //            solColor = color e intensidad del sol; solDir = direccion de la luz; ambArriba/ambAbajo = luz ambiental;
    //            causticas = reflejos de luz en el fondo (0 a 1); superficieY = altura de la superficie; inicioCamara = donde aparece el buzo.
    cfg.niebla = glm::vec3(0.035f, 0.075f, 0.060f);   // agua turbia: empieza oscura y se aclara al limpiar
    cfg.densidad = 0.045f;
    cfg.solColor = glm::vec3(0.40f, 0.52f, 0.46f) * 0.6f;
    cfg.solDir = glm::vec3(0.1f, -1.0f, 0.1f);
    cfg.ambArriba = glm::vec3(0.12f, 0.18f, 0.16f);
    cfg.ambAbajo = glm::vec3(0.02f, 0.04f, 0.04f);
    cfg.causticas = 0.05f;
    cfg.superficieY = 22.0f;
    cfg.linterna = true;
    cfg.inicioCamara = glm::vec3(0.0f, 5.0f, 22.0f);
    terrenoArchivo = "terreno_naufragios"; terrenoCarpeta = "naufragios";
    const char* c = "naufragios";
    auto suelo = [&](float x, float z) { return AlturaSuelo(x, z); };
    auto fijo = [&](const char* archivo, glm::vec3 p, float yaw, float esc, const char* etiqueta) {
        Elemento e = Fijo(archivo, c, p, yaw, esc); e.interactivo = etiqueta != nullptr; if (etiqueta) e.etiqueta = etiqueta; return e;
    };

    // ---------------- 57 fondo de grava (terreno) + rocas
    Elemento roca = fijo("roca_naufragio", glm::vec3(0.0f, -0.4f, 0.0f), 0, 1, "Roca"); MatOpaco(roca);
    Esparcir(roca, 12, glm::vec2(0.0f), 7.0f, 32.0f, 0.8f, 2.4f, 14);

    // ---------------- letreros informativos
    struct L { const char* a; glm::vec3 p; float yaw; const char* et; };
    L letreros[] = { { "letrero_bienvenida", {0.0f, 0.0f, 15.0f}, 0.0f, "Letrero: como limpiar" },
                     { "letrero_plasticos", {6.5f, 0.0f, 9.0f}, -35.0f, "Letrero: plasticos" },
                     { "letrero_redes", {0.5f, 0.0f, -8.5f}, -15.0f, "Letrero: redes fantasma" },
                     { "letrero_invasor", {-8.0f, 0.0f, 3.5f}, 60.0f, "Letrero: pez leon" },
                     { "letrero_toxicos", {12.5f, 0.0f, 5.0f}, -45.0f, "Letrero: residuos toxicos" } };
    for (auto& l : letreros) { Elemento e = fijo(l.a, l.p, l.yaw, 1.15f, l.et); MatOpaco(e); e.emision = 0.25f; elementos.push_back(e); }

    // ---------------- 52 submarino hundido: se asienta en el fondo al llegar (KF, una vez)
    Elemento sub = fijo("submarino", glm::vec3(0.0f), 25, 1.0f, "Submarino hundido");
    sub.mov = MOV_KF; sub.kf = GuardarKF([&]() { AnimacionKF a = KF_SubmarinoAsienta(glm::vec3(3.0f, suelo(5.4f, -4.4f) + 0.6f, -2.0f)); a.loop = false; return a; }());
    sub.kfInicio = 1.5f; MatMetalico(sub); elementos.push_back(sub);

    // ---------------- 53/54 ruinas (modelos de Meshy): arcadas, balcones y columnas clasicas
    // el marmol es casi blanco: el tinte lo envejece con sedimento y algas
    const glm::vec3 marmolHundido(0.52f, 0.58f, 0.50f);
    Elemento arcada = fijo("ruinas_arcada", glm::vec3(-16.5f, -0.3f, -1.5f), 90, 1.0f, "Ruinas"); MatOpaco(arcada); arcada.tinte = marmolHundido;
    elementos.push_back(arcada);                                         // muro principal de arcos al fondo
    arcada.pos = glm::vec3(-13.0f, -0.4f, 13.5f); arcada.yaw = 15; arcada.escala = 0.75f;
    arcada.rotExtra = glm::vec3(0.0f, 0.0f, -4.0f); elementos.push_back(arcada);   // arcada menor, ligeramente inclinada
    Elemento balcon = fijo("ruinas_balcon", glm::vec3(-20.5f, -0.4f, 7.0f), 60, 1.0f, "Ruinas"); MatOpaco(balcon); balcon.tinte = marmolHundido;
    elementos.push_back(balcon);
    balcon.pos = glm::vec3(-14.5f, -0.4f, -15.5f); balcon.yaw = 200; balcon.escala = 0.85f; elementos.push_back(balcon);
    // pasillo de columnas que lleva hacia la arcada
    float colX[] = { -12.5f, -12.5f, -9.5f, -9.5f }, colZ[] = { -6.0f, 1.0f, -6.0f, 1.0f };
    float inclina[] = { 2.0f, -3.0f, 0.0f, 4.0f };
    for (int i = 0; i < 4; i++) {
        Elemento col = fijo("columna", glm::vec3(colX[i], -0.3f, colZ[i]), (float)(i * 23), 1.0f, "Columna");
        col.rotExtra = glm::vec3(inclina[i], 0.0f, inclina[3 - i]); MatOpaco(col); col.tinte = marmolHundido; elementos.push_back(col);
    }
    Elemento tumbada = fijo("columna", glm::vec3(-11.0f, 0.55f, 7.5f), 30, 0.95f, "Columna");   // ya derrumbada
    tumbada.rotExtra = glm::vec3(0.0f, 0.0f, 88.0f); MatOpaco(tumbada); tumbada.tinte = marmolHundido * 0.9f; elementos.push_back(tumbada);
    Elemento caida = fijo("columna", glm::vec3(0.0f), 0, 0.9f, "Columna");   // se derrumba cuando te acercas (KF)
    caida.mov = MOV_KF; caida.kf = GuardarKF(KF_ColumnaDerrumbe(glm::vec3(-5.5f, suelo(-5.5f, 7.0f) - 0.2f, 7.0f), 75.0f));
    caida.kfInicio = 1e6f; MatOpaco(caida); caida.tinte = marmolHundido; elementos.push_back(caida);
    Elemento bloque = fijo("bloque", glm::vec3(0.0f, 0.2f, 0.0f), 0, 1, "Ruinas"); MatOpaco(bloque);
    Esparcir(bloque, 8, glm::vec2(-13.0f, 1.0f), 2.0f, 8.0f, 0.7f, 1.3f, 13);

    // ---------------- 51 ancla, 66 timon, 74 senal de transito, 49 dron
    Elemento ancla = fijo("ancla", glm::vec3(2.0f, -0.35f, -10.0f), 60, 1.0f, "Ancla oxidada"); ancla.rotExtra = glm::vec3(0.0f, 0.0f, -38.0f); MatMetalico(ancla); elementos.push_back(ancla);
    Elemento barco = fijo("BarcoRuinas", glm::vec3(-8.0f, 2.0f, -21.0f), 20, 1.0f, "Barco hundido");   // modelo de Julian
    MatOpaco(barco); elementos.push_back(barco);
    Elemento timon = fijo("timon", glm::vec3(-13.6f, 0.45f, -6.5f), 30, 1.0f, "Timon"); timon.rotExtra = glm::vec3(72.0f, 0.0f, 0.0f); MatOpaco(timon); elementos.push_back(timon);
    Elemento senal = fijo("senal", glm::vec3(-6.5f, -0.25f, 11.0f), 200, 1.0f, "Senal de transito"); senal.rotExtra = glm::vec3(0.0f, 0.0f, 24.0f); MatMetalico(senal); elementos.push_back(senal);
    Elemento dron = fijo("dron", cfg.inicioCamara + glm::vec3(-1.5f, 0.0f, -2.6f), 0, 0.55f, "Dron de exploracion");
    dron.sobreSuelo = false; dron.emision = 0.3f; MatPlastico(dron); dron.oscAmp = 0.08f; dron.oscVel = 1.6f; elementos.push_back(dron);

    // ---------------- 55/56 cofre del tesoro (se abre al limpiar todo) y monedas
    glm::vec3 pc(1.5f, suelo(1.5f, 5.0f) - 0.05f, 5.0f);
    Elemento cofre = fijo("cofre", glm::vec3(pc.x, -0.05f, pc.z), 0, 1.0f, "Cofre del tesoro"); MatOpaco(cofre); elementos.push_back(cofre);
    Elemento tapa = fijo("cofre_tapa", glm::vec3(0.0f), 0, 1.0f, "Cofre del tesoro");
    tapa.mov = MOV_KF; tapa.kf = GuardarKF(KF_TapaCofre(pc + glm::vec3(0.0f, 0.507f, -0.315f), 0.0f)); tapa.kfInicio = 1e6f; MatOpaco(tapa);   // bisagra del cofre de Meshy
    elementos.push_back(tapa);
    Elemento monedas = fijo("moneda", glm::vec3(pc.x + 0.7f, 0.0f, pc.z + 0.5f), 0, 1.0f, nullptr);
    monedas.miembros = 30; monedas.dispersion = 0.9f; monedas.presencia = 0.25f; monedas.emision = 0.35f; MatMetalico(monedas); elementos.push_back(monedas);

    // ---------------- residuos (se recogen con clic)
    auto residuo = [&](const char* archivo, const char* etiqueta, int tipo, float peso, int n, int semilla, bool deriva, float altura, glm::vec2 centro, float radio, float escala) {
        for (int i = 0; i < n; i++) {
            float a = Aleatorio(semilla, i) * 6.28f, r = 2.0f + radio * sqrtf(Aleatorio(semilla + 1, i));
            Elemento b = fijo(archivo, glm::vec3(centro.x + cosf(a) * r, altura, centro.y + sinf(a) * r), Aleatorio(semilla + 2, i) * 360.0f, 1.0f, etiqueta);
            if (!deriva) b.rotExtra = glm::vec3((Aleatorio(semilla + 3, i) - 0.5f) * 30.0f, 0.0f, Aleatorio(semilla + 4, i) > 0.4f ? 88.0f : 0.0f);
            b.mov = deriva ? MOV_DERIVA : MOV_FIJO; b.fase = (float)i * 1.7f;
            b.basura = true; b.residuo = tipo; b.peso = peso; b.escala = escala;   // mas grandes que en la realidad: faciles de ver y tomar
            elementos.push_back(b);
        }
    };
    size_t ini = elementos.size();
    residuo("bolsaplastico", "Bolsa de plastico", 1, 1.0f, 6, 83, true, 3.0f, glm::vec2(0.0f, 0.0f), 16.0f, 1.5f);
    residuo("botella", "Botella PET", 1, 1.0f, 4, 84, true, 2.5f, glm::vec2(2.0f, 3.0f), 14.0f, 1.8f);
    residuo("botella", "Botella PET", 1, 1.0f, 3, 85, false, 0.07f, glm::vec2(-3.0f, -2.0f), 12.0f, 1.8f);
    residuo("bota", "Bota de hule", 1, 1.0f, 3, 86, false, 0.08f, glm::vec2(-10.0f, 4.0f), 8.0f, 1.5f);
    residuo("Llanta", "Llanta", 1, 2.0f, 3, 82, false, 0.15f, glm::vec2(4.0f, 6.0f), 10.0f, 1.0f);
    residuo("lata", "Lata de aluminio", 2, 1.0f, 7, 80, false, 0.14f, glm::vec2(2.0f, 2.0f), 14.0f, 1.9f);
    residuo("lataaplastada", "Lata aplastada", 2, 1.0f, 5, 81, false, 0.10f, glm::vec2(-4.0f, 0.0f), 12.0f, 2.0f);
    residuo("pila", "Pila", 3, 1.0f, 6, 87, false, 0.06f, glm::vec2(6.0f, -2.0f), 10.0f, 2.5f);
    for (size_t i = ini; i < elementos.size(); i++) {
        Elemento& b = elementos[i];
        if (b.archivo == "lata" || b.archivo == "lataaplastada" || b.archivo == "pila") MatMetalico(b);
        else if (b.archivo == "bolsaplastico") { b.alpha = 0.55f; MatTraslucido(b); }
        else if (b.archivo == "botella") { b.alpha = 0.65f; MatTraslucido(b); b.fantasma = 0.15f; }
        else MatPlastico(b);
    }
    Elemento bolsaKF = fijo("bolsaplastico", glm::vec3(0.0f), 0, 1.0f, "Bolsa de plastico");   // KF de Julian: recorre puntos de flotacion
    bolsaKF.mov = MOV_KF; bolsaKF.kf = &KF_Bolsa(); bolsaKF.escala = 1.5f; bolsaKF.basura = true; bolsaKF.residuo = 1; bolsaKF.alpha = 0.55f; MatTraslucido(bolsaKF);
    elementos.push_back(bolsaKF);

    // 64 barril toxico con su nube verde
    Elemento barril = fijo("barril", glm::vec3(9.5f, -0.1f, 1.5f), 20, 1.0f, "Barril de residuos toxicos");
    barril.rotExtra = glm::vec3(8.0f, 0.0f, -12.0f); barril.basura = true; barril.residuo = 3; barril.peso = 5.0f; MatMetalico(barril);
    elementos.push_back(barril);
    Elemento nube = fijo("burbuja", glm::vec3(9.5f, 0.9f, 1.5f), 0, 3.2f, nullptr);
    nube.carpeta = "comun"; nube.tinte = glm::vec3(0.45f, 1.0f, 0.35f); nube.alpha = 0.30f; nube.emision = 0.7f; nube.pulsar = true; nube.fantasma = 0.4f;
    elementos.push_back(nube);

    // 63 red de pesca fantasma con una tortuga atrapada
    Elemento red = fijo("red_fantasma", glm::vec3(-3.5f, -0.2f, -13.0f), 15, 1.0f, "Red fantasma");
    red.residuo = 4; red.peso = 6.0f; MatOpaco(red); elementos.push_back(red);
    Elemento tortuga = Fijo("tortuga", "arrecifes", glm::vec3(-3.5f, 1.7f, -13.0f), 15, 1.0f);
    tortuga.interactivo = true; tortuga.etiqueta = "Tortuga atrapada"; elementos.push_back(tortuga);

    // ---------------- 67/68 cangrejos ermitanos (el de la lata cambia de concha al limpiar)
    Elemento cLata = fijo("cangrejo_lata", glm::vec3(4.0f, 0.12f, 7.5f), 200, 1.0f, "Cangrejo con lata"); MatMetalico(cLata); cLata.specInt = 0.6f; elementos.push_back(cLata);
    Elemento cNat = Fijo("cangrejoermitano", "arrecifes", glm::vec3(4.0f, 0.15f, 7.5f), 200, 1.0f); cNat.interactivo = true; cNat.etiqueta = "Cangrejo ermitano"; cNat.visible = false;
    elementos.push_back(cNat);
    cNat.pos = glm::vec3(-10.5f, 0.15f, -1.0f); cNat.yaw = 40; cNat.visible = true; elementos.push_back(cNat);

    // ---------------- 71 erizos y 73 estrellas de mar
    Elemento erizo = fijo("erizo", glm::vec3(0.0f, 0.0f, 0.0f), 0, 1, "Erizo de mar"); MatOpaco(erizo);
    Esparcir(erizo, 7, glm::vec2(-8.0f, -2.0f), 1.0f, 9.0f, 0.8f, 1.3f, 15);
    Elemento estrella = fijo("estrella_mar", glm::vec3(0.0f, 0.01f, 0.0f), 0, 1, "Estrella de mar"); MatOpaco(estrella);
    Esparcir(estrella, 6, glm::vec2(3.0f, 2.0f), 2.0f, 12.0f, 0.8f, 1.4f, 16, { {1.0f,1.0f,1.0f}, {1.0f,0.7f,0.75f}, {0.9f,0.6f,1.0f} });

    // ---------------- 65 peces leon (invasores, se retiran con clic)
    glm::vec4 leones[] = { {-12.0f, 2.5f, 0.0f, 2.5f}, {-9.0f, 2.0f, -5.0f, 2.0f}, {6.0f, 2.5f, -6.0f, 3.0f}, {-3.0f, 3.0f, 8.0f, 2.5f}, {10.0f, 2.0f, 3.0f, 2.0f}, {-15.0f, 3.0f, 6.0f, 3.0f} };
    for (int i = 0; i < 6; i++) {
        Elemento l = Nadador("pezleon", c, MOV_ORBITA, glm::vec3(leones[i]), leones[i].w, (i % 2 ? -0.25f : 0.22f), "Pez leon (invasor)");
        l.sobreSuelo = true; l.velNado = 4.0f; l.fase = i * 1.1f; l.invasor = true; elementos.push_back(l);
    }
    // peces nativos: pocos al inicio, aumentan al retirar invasores y limpiar
    Elemento roncadores = Nadador("roncador", c, MOV_CARDUMEN, glm::vec3(1.0f, 3.5f, 1.0f), 7.0f, 0.3f, "Roncador");
    roncadores.miembros = 18; roncadores.dispersion = 1.4f; roncadores.velNado = 12.0f; roncadores.sobreSuelo = true; roncadores.presencia = 0.15f;
    elementos.push_back(roncadores);

    // ---------------- 48 tiburon blanco y 69 medusas
    Elemento tiburon = Nadador("Tiburon", c, MOV_ORBITA, glm::vec3(0.0f, 9.0f, 0.0f), 20.0f, 0.10f, "Tiburon");
    tiburon.ondula = 0.15f; tiburon.velNado = 3.0f; elementos.push_back(tiburon);
    for (int i = 0; i < 6; i++) {
        Elemento m = fijo("medusa", glm::vec3(-12.0f + i * 5.0f, 5.0f + (i % 3) * 2.5f, -6.0f + (i % 2) * 12.0f), 0, 0.8f + 0.1f * i, "Medusa");
        m.sobreSuelo = false; m.mov = MOV_DERIVA; m.fase = i * 1.9f; m.pulsar = true; m.alpha = 0.78f; m.emision = 0.30f; m.fantasma = 0.25f;
        m.mecer = 0.5f; MatTraslucido(m); elementos.push_back(m);
    }

    // ---------------- 23 burbujas: fugas del submarino
    fuentes = { glm::vec3(5.0f, 1.6f, -4.5f), glm::vec3(7.5f, 2.3f, -6.0f) };
}

#endif
