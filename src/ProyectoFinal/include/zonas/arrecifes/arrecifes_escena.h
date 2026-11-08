#ifndef ARRECIFES_ESCENA_H
#define ARRECIFES_ESCENA_H
// ============================================================================================
// ZONA 1 · ARRECIFES · Que objetos hay y donde estan (ConstruirArrecifes).
// [CAMBIAR] luz, niebla y color del agua: bloque cfg.* al inicio de ConstruirArrecifes().
// [CAMBIAR] posicion, tamano o velocidad de un animal: busca su nombre de archivo (ej. "tortuga").
// [AGREGAR] un modelo: ver la plantilla marcada con [AGREGAR] al final de ConstruirArrecifes().
// ============================================================================================

inline void Escenario::AjustarArrecife()
{
    for (auto& e : elementos) {
        float largo = 0.0f;
        if (e.archivo == "CaballitoMar") largo = 0.20f;
        else if (e.archivo == "PezPayaso") largo = 0.12f;
        else if (e.archivo == "Pezcirujano") largo = 0.30f;
        else if (e.archivo == "PezGlobo") largo = 0.45f;
        else if (e.archivo == "langosta") largo = 0.50f;
        else if (e.archivo == "PepinoMar") largo = 0.35f;
        else if (e.archivo == "EstrellaCorona") largo = 0.45f;
        else if (e.archivo == "manati") largo = 3.0f;
        else if (e.archivo == "delfin") largo = 2.8f;
        else if (e.archivo == "barracuda") largo = 1.4f;
        else if (e.archivo == "vacamarina") largo = 1.5f;
        else if (e.archivo == "mantaraya") largo = 4.0f;
        if (largo == 0.0f || e.modelo->meshes.empty()) continue;
        glm::vec3 minimo(1e30f), maximo(-1e30f);
        for (auto& m : e.modelo->meshes) for (auto& v : m.vertices) {
            minimo = glm::min(minimo, v.Position);
            maximo = glm::max(maximo, v.Position);
        }
        glm::vec3 dim = maximo - minimo;
        float mayor = glm::max(dim.x, glm::max(dim.y, dim.z));
        if (mayor > 0.001f) e.escala = largo / mayor;
    }
    // Separar colonias entre si y de los objetos fijos del fondo.
    for (size_t i = 0; i < elementos.size(); ++i) {
        Elemento& e = elementos[i];
        bool colonia = e.archivo.find("coral_") == 0 || e.archivo == "esponja_tubo";
        if (!colonia) continue;
        glm::vec3 original = e.pos;
        for (int intento = 0; intento < 500; ++intento) {
            if (intento) {
                float angulo = intento * 2.399963f;
                float distancia = 0.45f * sqrtf((float)intento);
                e.pos.x = original.x + cosf(angulo) * distancia;
                e.pos.z = original.z + sinf(angulo) * distancia;
            }
            bool libre = true;
            for (size_t j = 0; j < elementos.size(); ++j) {
                const Elemento& otro = elementos[j];
                bool otraColonia = otro.archivo.find("coral_") == 0 || otro.archivo == "esponja_tubo";
                if (j == i || (otraColonia && j > i) || otro.mov != MOV_FIJO || otro.archivo == "pasto_marino") continue;
                float distancia = glm::length(glm::vec2(e.pos.x - otro.pos.x, e.pos.z - otro.pos.z));
                if (distancia < e.radioBase * e.escala + otro.radioBase * otro.escala + 0.15f) { libre = false; break; }
            }
            if (libre) break;
        }
        e.baseY = e.sobreSuelo ? AlturaSuelo(e.pos.x, e.pos.z) : 0.0f;
    }
}

inline void Escenario::ConstruirArrecifes()
{
    cfg = ZonaConfig();
    cfg.nombre = "Arrecifes";
    // [CAMBIAR] AMBIENTE DE LA ZONA: niebla = color del agua (R,G,B de 0 a 1); densidad = que tan turbia (0.01 clara, 0.05 turbia);
    //            solColor = color e intensidad del sol; solDir = direccion de la luz; ambArriba/ambAbajo = luz ambiental;
    //            causticas = reflejos de luz en el fondo (0 a 1); superficieY = altura de la superficie; inicioCamara = donde aparece el buzo.
    cfg.niebla = glm::vec3(0.07f, 0.42f, 0.52f);
    cfg.densidad = 0.018f;
    cfg.solColor = glm::vec3(1.0f, 0.96f, 0.86f) * 1.35f;
    cfg.solDir = glm::vec3(-0.3f, -1.0f, -0.25f);
    cfg.ambArriba = glm::vec3(0.38f, 0.58f, 0.64f);
    cfg.ambAbajo = glm::vec3(0.13f, 0.23f, 0.27f);
    cfg.causticas = 1.0f;
    cfg.superficieY = 14.0f;                       // arrecife somero, lleno de luz
    cfg.inicioCamara = glm::vec3(0.0f, 8.0f, 20.0f);

    // [CAMBIAR] NADO, CAMARA Y FAUNA DE ESTA ZONA (solo afectan a esta zona)
    cfg.nadoAceleracion = 3.5f;   // rapidez del buzo (velocidad maxima ~ aceleracion / resistencia)
    cfg.nadoResistencia = 2.5f;   // resistencia del agua: mas alto = se detiene antes
    cfg.campoVision = 45.0f;      // campo de vision en grados (45 normal, 60 gran angular)
    cfg.velFauna = 0.5f;          // velocidad de los animales (1.0 normal, 0.5 mitad, 2.0 doble)
    // [CAMBIAR] MATERIALES DE ESTA ZONA: glm::vec2(intensidad del brillo, concentracion del brillo)
    cfg.matOpaco = glm::vec2(0.08f, 24.0f);        // roca, madera, arena
    cfg.matPlastico = glm::vec2(0.60f, 32.0f);     // plasticos, caucho
    cfg.matTraslucido = glm::vec2(0.30f, 64.0f);   // medusas, bolsas, botellas
    cfg.matMetalico = glm::vec2(1.00f, 96.0f);     // latas, ancla, submarino, monedas
    terrenoArchivo = "terreno_arrecifes"; terrenoCarpeta = "arrecifes";
    const char* c = "arrecifes";

    std::vector<glm::vec3> vivos = { {1.0f,0.42f,0.58f}, {0.72f,0.40f,0.95f}, {1.0f,0.56f,0.26f}, {1.0f,0.84f,0.32f}, {0.28f,0.85f,0.78f}, {0.95f,0.33f,0.30f} };
    Elemento roca = Fijo("roca_arrecife", c, glm::vec3(0.0f, -0.4f, 0.0f), 0, 1); roca.etiqueta = "Roca"; roca.interactivo = true;
    Esparcir(roca, 14, glm::vec2(0.0f), 4.0f, 32.0f, 0.8f, 2.6f, 11);

    // 22 Fondo + 10/11/12 corales en cuatro parches de arrecife
    glm::vec2 parches[] = { {-6.0f, -4.0f}, {7.0f, -9.0f}, {-1.0f, 8.0f}, {12.0f, 4.0f} };
    for (int p = 0; p < 4; p++) {
        Elemento r1 = Fijo("coral_rama1", c, glm::vec3(0.0f, -0.1f, 0.0f), 0, 1);
        r1.interactivo = true; r1.etiqueta = "Coral ramificado"; r1.esCoral = true;
        Elemento r2 = r1; r2.archivo = "coral_rama2";
        Elemento ce = Fijo("coral_cerebro", c, glm::vec3(0.0f, -0.15f, 0.0f), 0, 1);
        ce.interactivo = true; ce.etiqueta = "Coral cerebro"; ce.esCoral = true;
        Elemento ab = Fijo("coral_abanico", c, glm::vec3(0.0f, -0.1f, 0.0f), 0, 1);
        ab.interactivo = true; ab.etiqueta = "Abanico de mar"; ab.mecer = 0.15f; ab.esCoral = true;
        Elemento es = Fijo("esponja_tubo", c, glm::vec3(0.0f, -0.05f, 0.0f), 0, 1);
        es.interactivo = true; es.etiqueta = "Esponja de tubo";
        Esparcir(r1, 7, parches[p], 0.0f, 5.0f, 1.0f, 2.4f, 20 + p * 10, vivos);
        Esparcir(r2, 6, parches[p], 0.0f, 5.5f, 1.0f, 2.2f, 21 + p * 10, vivos);
        Esparcir(ce, 5, parches[p], 0.5f, 5.0f, 0.8f, 1.8f, 22 + p * 10, vivos);
        Esparcir(ab, 3, parches[p], 1.0f, 5.0f, 1.2f, 2.0f, 23 + p * 10, { {0.85f,0.30f,0.75f}, {1.0f,0.55f,0.30f} });
        Esparcir(es, 2, parches[p], 1.5f, 4.5f, 0.8f, 1.3f, 24 + p * 10, { {0.75f,0.45f,0.95f}, {1.0f,0.78f,0.30f}, {0.95f,0.50f,0.35f} });
        Elemento rs = r1; rs.archivo = "coral_rama_santiago";        // coral ramificado modelado por Santiago
        Esparcir(rs, 4, parches[p], 0.5f, 5.0f, 0.9f, 1.5f, 25 + p * 10, vivos);
    }
    for (size_t i = 0; i < elementos.size(); i++)
        if (elementos[i].esCoral) elementos[i].sensibilidad = 0.6f + 0.9f * Aleatorio(91, (int)i);

    // 16 Anemonas con 18 peces payaso
    glm::vec3 anem[] = { {-5.4f, 0.0f, -3.0f}, {-6.9f, 0.0f, -4.6f}, {-5.7f, 0.0f, -5.6f} };
    glm::vec3 tAnem[] = { {1.0f, 0.55f, 0.75f}, {0.6f, 1.0f, 0.65f}, {0.85f, 0.6f, 1.0f} };
    for (int i = 0; i < 3; i++) {
        Elemento a = Fijo("anemona", c, anem[i], (float)(i * 70), 1.3f);
        a.mecer = 0.8f; a.tinte = tAnem[i]; a.interactivo = true; a.etiqueta = "Anemona"; a.esCoral = true; a.sensibilidad = 0.9f; a.fase = (float)i;
        elementos.push_back(a);
    }
    Elemento payasos = Nadador("PezPayaso", c, MOV_CARDUMEN, glm::vec3(-6.0f, 0.9f, -4.4f), 1.1f, 0.6f, "Pez payaso");
		payasos.rotExtra = glm::vec3(0.0f, 180.0f, 0.0f);
    payasos.miembros = 6; payasos.dispersion = 0.5f; payasos.velNado = 12.0f; payasos.sobreSuelo = true; payasos.oscAmp = 0.2f;
    elementos.push_back(payasos);

    // Pez loro recorre el claro central con seis cuadros clave.
    Elemento pezLoro = Nadador("PezLoro", c, MOV_KF, glm::vec3(0.0f, 4.2f, 0.0f), 0.0f, 0.0f, "Pez loro");
    pezLoro.kf = GuardarKF(KF_PezLoro(pezLoro.pos));
    pezLoro.ondula = 0.05f; pezLoro.velNado = 4.0f; pezLoro.fase = 1.5f;
    elementos.push_back(pezLoro);

    // Estrella de corona de espinas (cerca de los parches de coral pero sin chocar)
    Elemento corona1 = Fijo("EstrellaCorona", c, glm::vec3(0.0f, 0.00f, 0.0f), 45, 1.0f);
    corona1.rotExtra = glm::vec3(-90.0f, 0.0f, 0.0f); // Acuesta la estrella horizontalmente
    corona1.interactivo = true;
    corona1.etiqueta = "Estrella corona de espinas";
		MatOpaco(corona1);
    elementos.push_back(corona1);

    Elemento corona2 = Fijo("EstrellaCorona", c, glm::vec3(-2.5f, 0.05f, 6.5f), 120, 0.9f);
    corona2.rotExtra = glm::vec3(-90.0f, 0.0f, 0.0f); // Acuesta la estrella horizontalmente
    corona2.interactivo = true;
    corona2.etiqueta = "Estrella corona de espinas";
		MatOpaco(corona2);
    elementos.push_back(corona2);

    // Pepino de mar (en zonas más arenosas / cerca del pasto marino)
    Elemento pepino1 = Fijo("PepinoMar", c, glm::vec3(-12.0f, 0.0f, 8.5f), 75, 1.0f);
		pepino1.rotExtra = glm::vec3(-90.0f, 0.0f, 0.0f); // Acuesta el pepino horizontalmente
    pepino1.interactivo = true;
    pepino1.etiqueta = "Pepino de mar";
		MatPlastico(pepino1);
    elementos.push_back(pepino1);

    Elemento pepino2 = Fijo("PepinoMar", c, glm::vec3(3.0f, 0.0f, 10.0f), 210, 1.1f);
		pepino2.rotExtra = glm::vec3(-90.0f, 0.0f, 0.0f); // Acuesta el pepino horizontalmente
    pepino2.interactivo = true;
    pepino2.etiqueta = "Pepino de mar";
		MatPlastico(pepino2);
    elementos.push_back(pepino2);

    // 17 Pradera de pasto marino con 08 manati y 03 caballitos de mar
    Elemento pasto = Fijo("pasto_marino", c, glm::vec3(0.0f, -0.05f, 0.0f), 0, 1);
    pasto.mecer = 1.6f; pasto.interactivo = true; pasto.etiqueta = "Pasto marino";
    Esparcir(pasto, 70, glm::vec2(-17.0f, 10.0f), 0.0f, 9.0f, 0.8f, 1.6f, 60, { {1.0f,1.0f,1.0f}, {0.85f,1.0f,0.8f}, {1.1f,1.05f,0.85f} });
    Elemento manati = Nadador("manati", c, MOV_ORBITA, glm::vec3(-17.0f, 1.8f, 10.0f), 6.0f, -0.07f, "Manati");
    manati.ondula = 0.05f; manati.velNado = 2.0f; manati.sobreSuelo = true; manati.ondulaVertical = true; elementos.push_back(manati);
    Elemento caballito = Fijo("CaballitoMar", c, glm::vec3(-14.0f, 0.7f, 7.5f), 70, 1.0f);
    caballito.interactivo = true; caballito.etiqueta = "Caballito de mar"; caballito.oscAmp = 0.2f; caballito.oscVel = 0.8f;
    elementos.push_back(caballito);
    caballito.pos = glm::vec3(-19.5f, 0.9f, 12.0f); caballito.yaw = 250; caballito.fase = 2.0f; elementos.push_back(caballito);
    caballito.pos = glm::vec3(-16.0f, 0.6f, 14.0f); caballito.yaw = 130; caballito.fase = 4.0f; elementos.push_back(caballito);

    // 07 Almejas, 05 pulpo y 15 langostas en el fondo
    Elemento almeja = Fijo("Almeja", c, glm::vec3(7.5f, 0.2f, -6.0f), 30, 1.0f);
    almeja.interactivo = true; almeja.etiqueta = "Almeja gigante"; almeja.esCoral = true; almeja.sensibilidad = 0.7f;
    almeja.tinte = glm::vec3(0.75f, 0.85f, 1.15f);
    {   // KF: la almeja abre y cierra la concha (animacion por cuadros clave de Julian)
        Elemento ak = almeja;
        ak.mov = MOV_KF; ak.kf = GuardarKF(KF_AlmejaAbre(glm::vec3(7.5f, AlturaSuelo(7.5f, -6.0f) + 0.2f, -6.0f)));
        elementos.push_back(ak);
    }
    almeja.pos = glm::vec3(-2.5f, 0.2f, 11.5f); almeja.yaw = 200; almeja.escala = 0.8f; elementos.push_back(almeja);
    Elemento pulpo = Fijo("pulpo", c, glm::vec3(2.0f, 0.0f, 4.0f), 220, 1.0f);
    pulpo.interactivo = true; pulpo.etiqueta = "Pulpo"; pulpo.camuflaje = true; pulpo.mecer = 0.3f; pulpo.fase = 1.0f;
    elementos.push_back(pulpo);
    Elemento langosta = Fijo("langosta", c, glm::vec3(9.0f, 0.02f, -6.5f), 160, 1.0f);
		langosta.rotExtra = glm::vec3(-90.0f, 0.0f, 0.0f);
    langosta.interactivo = true; langosta.etiqueta = "Langosta";
    elementos.push_back(langosta);
    langosta.pos = glm::vec3(-3.5f, 0.02f, 10.5f); langosta.yaw = 20; elementos.push_back(langosta);
    langosta.pos = glm::vec3(13.5f, 0.02f, 2.0f); langosta.yaw = 300; elementos.push_back(langosta);

    // 21 Respiradero hidrotermal
    Elemento resp = Fijo("respiradero", c, glm::vec3(18.0f, -0.3f, -15.0f), 20, 1.2f);
    resp.interactivo = true; resp.etiqueta = "Respiradero hidrotermal";
    elementos.push_back(resp);

    // Fauna que nada
    Elemento cirujanos = Nadador("Pezcirujano", c, MOV_CARDUMEN, glm::vec3(7.0f, 2.6f, -9.0f), 4.0f, -0.4f, "Pez cirujano azul");
    cirujanos.miembros = 7; cirujanos.dispersion = 1.0f; cirujanos.velNado = 11.0f; cirujanos.sobreSuelo = true;
    elementos.push_back(cirujanos);
    Elemento globo = Nadador("PezGlobo", c, MOV_ORBITA, glm::vec3(-1.0f, 1.8f, 8.0f), 2.5f, 0.25f, "Pez globo");
		globo.rotExtra = glm::vec3(-90.0f, 0.0f, 0.0f);
    globo.inflable = true; globo.sobreSuelo = true; globo.ondula = 0.03f; globo.oscAmp = 0.3f; elementos.push_back(globo);
    globo.pos = glm::vec3(12.0f, 2.2f, 4.0f); globo.vel = -0.2f; globo.fase = 2.0f; elementos.push_back(globo);
    globo.pos = glm::vec3(1.0f, 2.5f, -1.0f); globo.radio = 4.0f; globo.vel = 0.15f; globo.fase = 4.0f; elementos.push_back(globo);
    Elemento tortuga = Nadador("tortuga", c, MOV_KF, glm::vec3(0.0f, 4.5f, 0.0f), 10.5f, 0.09f, "Tortuga marina");
    tortuga.ondula = 0.0f; tortuga.oscAmp = 0.0f; tortuga.oscVel = 1.0f;
    tortuga.kf = GuardarKF(KF_TortugaArrecife(tortuga.pos, 10.5f, 1.0f));
    elementos.push_back(tortuga);
    tortuga.pos = glm::vec3(-12.0f, 3.5f, 6.0f); tortuga.fase = 2.5f; tortuga.escala = 0.85f;
    tortuga.kf = GuardarKF(KF_TortugaArrecife(tortuga.pos, 6.0f, 0.85f));
    elementos.push_back(tortuga);
    Elemento manta = Nadador("mantaraya", c, MOV_LIBRE, glm::vec3(0.0f, 8.0f, -6.0f), 22.0f, 0.06f, "Mantarraya");
    manta.ondula = 0.0f; manta.oscAmp = 1.5f; manta.oscVel = 0.25f; elementos.push_back(manta);
    Elemento delfines = Nadador("delfin", c, MOV_CARDUMEN, glm::vec3(0.0f, 10.5f, 0.0f), 24.0f, 0.22f, "Delfin nariz de botella");
    delfines.miembros = 3; delfines.dispersion = 2.0f; delfines.ondula = 0.12f; delfines.velNado = 5.0f; delfines.ondulaVertical = true;
    delfines.oscAmp = 1.2f; delfines.oscVel = 0.9f; elementos.push_back(delfines);
    Elemento barracudas = Nadador("barracuda", "arrecifes", MOV_CARDUMEN, glm::vec3(4.0f, 6.0f, 2.0f), 16.0f, -0.16f, "Barracuda");
    barracudas.miembros = 4; barracudas.dispersion = 1.5f; elementos.push_back(barracudas);
    Elemento vaquita = Nadador("vacamarina", "arrecifes", MOV_ORBITA, glm::vec3(6.0f, 3.5f, 10.0f), 9.0f, 0.16f, "Vaquita marina");
    vaquita.ondula = 0.08f; vaquita.velNado = 4.0f; vaquita.sobreSuelo = true; vaquita.ondulaVertical = true; elementos.push_back(vaquita);

    // 23 Burbujas: respiradero y grietas del arrecife
    fuentes = { glm::vec3(18.0f, 3.6f, -15.0f), glm::vec3(18.4f, 2.4f, -14.4f), glm::vec3(-6.0f, 0.4f, -4.0f) };

    // [AGREGAR] UN MODELO NUEVO A ESTA ZONA (copia, quita las // y cambia los valores):
    //   1) Pon el archivo .fbx (y su textura .jpg/.png) en bin/models/zonas/arrecifes/
    //   2) Objeto quieto:
    //        Elemento nuevo = Fijo("mi_modelo", "arrecifes", glm::vec3(x, 0.0f, z), giroY, escala);
    //        nuevo.interactivo = true; nuevo.etiqueta = "Mi modelo";   // para que la mira lo seleccione
    //        MatPlastico(nuevo);                                       // material: MatOpaco / MatPlastico / MatTraslucido / MatMetalico
    //        elementos.push_back(nuevo);
    //   3) Animal que nada en circulo:
    //        Elemento pez = Nadador("mi_pez", "arrecifes", MOV_ORBITA, glm::vec3(x, altura, z), radio, velocidad, "Mi pez");
    //        elementos.push_back(pez);
    //   4) (opcional) su ficha educativa en interfaz/fichas.h
}

#endif
