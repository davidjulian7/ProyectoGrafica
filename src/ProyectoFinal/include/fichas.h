#ifndef FICHAS_H
#define FICHAS_H
/*
* Fichas educativas que aparecen al apuntar con la mira.
* La clave es la 'etiqueta' del Elemento en zonas.h.
* bitacora = true: cuenta para "Especies registradas" de su zona.
*/
#include <string>
#include <map>

struct Ficha {
    std::string nombre;
    std::string categoria;   // FAUNA, ARRECIFE, HABITAT, BASURA, EN PELIGRO...
    std::string estado;      // estado de conservacion (UICN) o tipo de elemento
    std::string dato;        // dato breve para el visitante
    bool bitacora;
};

inline const std::map<std::string, Ficha>& Fichas()
{
    static const std::map<std::string, Ficha> f = {
        // ---------------- Zona 1: Arrecifes
        { "Coral ramificado", { "Coral ramificado", "ARRECIFE", "Animal colonial",
          "Sus colores vienen de microalgas (zooxantelas) que viven en su tejido y le dan hasta 90% de su alimento. Con calor prolongado las expulsa y se blanquea.", true } },
        { "Coral cerebro", { "Coral cerebro", "ARRECIFE", "Animal colonial",
          "Crece apenas milímetros al año: un coral cerebro grande puede tener siglos de edad. Si se blanquea por mucho tiempo, muere de hambre.", true } },
        { "Abanico de mar", { "Abanico de mar", "ARRECIFE", "Gorgonia",
          "Es una colonia de pólipos que se orienta contra la corriente para atrapar el plancton que pasa.", true } },
        { "Anemona", { "Anémona", "FAUNA", "Hogar del pez payaso",
          "Tiene tentáculos urticantes y también vive con zooxantelas, por eso se blanquea con el calor igual que los corales.", true } },
        { "Esponja de tubo", { "Esponja de tubo", "FAUNA", "Animal filtrador",
          "Las esponjas filtran el agua para alimentarse; una sola puede filtrar miles de litros al día y ayuda a mantenerla limpia.", true } },
        { "Pasto marino", { "Pasto marino", "HÁBITAT", "Planta con flor",
          "Las praderas marinas capturan mucho carbono (carbono azul) y son el comedor de manatíes y tortugas verdes.", true } },
        { "Pez payaso", { "Pez payaso", "FAUNA", "Preocupación menor",
          "Vive entre los tentáculos de la anémona sin ser picado gracias a una capa de moco. Si la anémona se blanquea, pierde su refugio.", true } },
        { "Pez cirujano azul", { "Pez cirujano azul", "FAUNA", "Preocupación menor",
          "Tiene una espina afilada como bisturí junto a la cola. Come algas y evita que cubran a los corales.", true } },
        { "Caballito de mar", { "Caballito de mar", "FAUNA", "Varias especies vulnerables",
          "Es el macho quien guarda los huevos en una bolsa en su vientre hasta que nacen las crías.", true } },
        { "Tortuga marina", { "Tortuga marina", "EN PELIGRO", "Tortuga verde: En peligro (UICN)",
          "La temperatura de la arena decide el sexo de las crías: con más calor nacen más hembras.", true } },
        { "Pulpo", { "Pulpo", "FAUNA", "Preocupación menor",
          "Tiene tres corazones y sangre azul. Cambia de color y textura en fracciones de segundo para camuflarse.", true } },
        { "Vaquita marina", { "Vaquita marina", "EN PELIGRO", "En peligro crítico (UICN)",
          "Solo vive en el Alto Golfo de California. Es el mamífero marino más amenazado: quedan menos de 10 ejemplares.", true } },
        { "Almeja gigante", { "Almeja gigante", "FAUNA", "Varias especies vulnerables",
          "Puede vivir más de 100 años y, como los corales, alberga microalgas en su manto: también se blanquea con el calor.", true } },
        { "Manati", { "Manatí", "FAUNA", "Vulnerable (UICN)",
          "Es herbívoro: puede comer hasta el 10% de su peso al día en pasto marino. Su pariente vivo más cercano es el elefante.", true } },
        { "Pez globo", { "Pez globo", "FAUNA", "Preocupación menor",
          "Se infla tragando agua cuando se siente amenazado. Acércate a uno. Muchos contienen tetrodotoxina, un veneno muy potente.", true } },
        { "Delfin nariz de botella", { "Delfín nariz de botella", "FAUNA", "Preocupación menor",
          "Se orienta con ecolocalización: emite chasquidos y escucha el eco para 'ver' con el sonido.", true } },
        { "Langosta", { "Langosta", "FAUNA", "Varía por especie",
          "Para crecer muda su exoesqueleto; mientras el nuevo se endurece se esconde entre las rocas del arrecife.", true } },
        { "Barracuda", { "Barracuda", "FAUNA", "Preocupación menor",
          "Depredador de emboscada: espera inmóvil y ataca con aceleraciones muy rápidas.", true } },
        { "Mantarraya", { "Manta raya", "EN PELIGRO", "Manta gigante: En peligro (UICN)",
          "Puede superar los 6 m de envergadura y se alimenta filtrando plancton mientras nada.", true } },
        { "Respiradero hidrotermal", { "Respiradero hidrotermal", "HÁBITAT", "Formación geológica",
          "Expulsa agua calentada por el magma, rica en minerales. Alrededor viven organismos que obtienen energía de químicos, no del sol.", false } },
        { "Roca", { "Roca de arrecife", "HÁBITAT", "Sustrato", "Las larvas de coral necesitan superficies duras como esta para fijarse y empezar una colonia.", false } },

        // ---------------- Zona 2: Aguas Frias
        { "Ballena azul", { "Ballena azul", "EN PELIGRO", "En peligro (UICN)", "Es el animal más grande que ha existido, y se alimenta casi solo de krill.", true } },
        { "Beluga", { "Beluga", "FAUNA", "Preocupación menor", "La llaman 'canario del mar' por la variedad de silbidos y chasquidos que produce.", true } },
        { "Foca", { "Foca", "FAUNA", "Varía por especie", "Usa el hielo marino para descansar y criar; sin hielo pierde su refugio.", true } },
        { "Pinguino", { "Pingüino", "FAUNA", "Varía por especie", "No vuela en el aire, pero bajo el agua 'vuela' con sus aletas.", true } },
        { "Oso polar", { "Oso polar", "FAUNA", "Vulnerable (UICN)", "Caza focas desde el hielo marino; cuando el hielo se derrite, pasa hambre.", true } },
        { "Morsa", { "Morsa", "FAUNA", "Vulnerable (UICN)", "Usa sus colmillos para salir del agua y subir al hielo.", true } },
        { "Krill", { "Krill", "FAUNA", "Base de la cadena trófica", "Este pequeño crustáceo alimenta a ballenas, focas y pingüinos. Necesita el hielo para crecer.", true } },
        { "Bacalao", { "Bacalao", "FAUNA", "Vulnerable (UICN)", "Fue tan pescado en el Atlántico Norte que algunas poblaciones colapsaron en los años 90.", true } },
        { "Plataforma de hielo", { "Plataforma de hielo", "HÁBITAT", "Hielo marino", "El hielo marino del Ártico en verano se ha reducido drásticamente desde 1979.", false } },
        { "Iceberg", { "Iceberg", "HÁBITAT", "Hielo de glaciar", "Solo vemos cerca de una décima parte: el resto queda bajo el agua.", false } },
        { "Orca", { "Orca", "FAUNA", "Datos insuficientes (UICN)", "Es el depredador tope del océano: caza en manada y también se alimenta de focas y pingüinos.", true } },
        { "Narval", { "Narval", "FAUNA", "Casi amenazado (UICN)", "Su 'cuerno' es un diente largo. Vive bajo el hielo del Ártico y es muy sensible a su pérdida.", true } },
        { "Cachalote", { "Cachalote", "FAUNA", "Vulnerable (UICN)", "Bucea más de mil metros para cazar calamares gigantes en la oscuridad.", true } },
        { "Calamar colosal", { "Calamar colosal", "FAUNA", "Preocupación menor (UICN)", "Es el invertebrado más grande que existe y casi nunca se ha visto vivo.", true } },
        { "Leon marino", { "León marino", "FAUNA", "Varía por especie", "Descansa en costas y plataformas de hielo; si cambia su comida por el calentamiento, pasa hambre.", true } },
        { "Pez dracoide", { "Pez dracoide", "FAUNA", "Varía por especie", "Algunos peces de hielo antárticos tienen la sangre transparente porque no tienen hemoglobina.", true } },
        { "Pescado", { "Pescado", "FAUNA", "Alimento", "Los peces como este son el alimento de focas, pingüinos y osos polares.", false } },

        // ---------------- Zona 3: Naufragios
        { "Submarino hundido", { "Submarino hundido", "NAUFRAGIO", "Huella humana", "Los restos de barcos y submarinos pueden liberar combustible y metales durante décadas.", false } },
        { "Ruinas", { "Ruinas sumergidas", "NAUFRAGIO", "Huella humana", "Las construcciones hundidas se vuelven arrecifes artificiales, pero también acumulan basura.", false } },
        { "Lata de aluminio", { "Lata de aluminio", "BASURA", "Metal", "Una lata puede tardar alrededor de 200 años en degradarse en el mar.", false } },
        { "Lata aplastada", { "Lata aplastada", "BASURA", "Metal", "El metal se oxida y libera compuestos al agua mientras se degrada.", false } },
        { "Llanta", { "Neumático", "BASURA", "Caucho", "Las llantas liberan microplásticos y sustancias tóxicas al desgastarse.", false } },
        { "Bolsa de plastico", { "Bolsa de plástico", "BASURA", "Plástico", "Las tortugas las confunden con medusas y se las comen.", false } },
        { "Pez leon (invasor)", { "Pez león", "ESPECIE INVASORA", "Invasor en el Caribe y el Golfo de México", "Llegó desde el Indo-Pacífico, muy probablemente por liberación de acuarios. Aquí no tiene depredadores y devora peces nativos.", true } },
        { "Tiburon", { "Tiburón blanco", "FAUNA", "Vulnerable (UICN)", "Es un superdepredador: regula las poblaciones de focas y peces. Detecta los campos eléctricos de sus presas con órganos especiales en el hocico.", true } },
        { "Roncador", { "Roncador", "FAUNA", "Pez nativo del Caribe", "Debe su nombre al sonido que hace al rechinar los dientes. Sus crías son una de las presas favoritas del pez león invasor.", true } },
        { "Medusa", { "Medusa", "FAUNA", "Varía por especie", "Es 95% agua y no tiene cerebro. Las tortugas marinas las comen y confunden con ellas las bolsas de plástico.", true } },
        { "Erizo de mar", { "Erizo de mar", "FAUNA", "Varía por especie", "Raspa las algas de las rocas con una boca de cinco dientes llamada linterna de Aristóteles.", true } },
        { "Estrella de mar", { "Estrella de mar", "FAUNA", "Varía por especie", "Puede regenerar un brazo perdido y come sacando su estómago fuera del cuerpo.", true } },
        { "Cangrejo ermitano", { "Cangrejo ermitaño", "FAUNA", "Concha natural", "No fabrica su caparazón: usa conchas vacías y se muda a una más grande al crecer.", true } },
        { "Cangrejo con lata", { "Cangrejo ermitaño", "FAUNA", "Usa basura como refugio", "Cuando faltan conchas, los cangrejos ermitaños usan latas y tapas de plástico, que pueden atraparlos o cortarlos.", false } },
        { "Tortuga atrapada", { "Tortuga atrapada", "EN PELIGRO", "Enredada en una red fantasma", "Sin ayuda no puede subir a respirar. Mantén presionado el clic sobre la red para cortarla y liberarla.", false } },
        { "Red fantasma", { "Red fantasma", "BASURA", "Red de pesca abandonada", "Sigue atrapando animales durante años. Las redes perdidas son cerca del 10% de la basura marina.", false } },
        { "Botella PET", { "Botella PET", "BASURA", "Plástico", "Puede tardar unos 450 años en degradarse y se fragmenta en microplásticos.", false } },
        { "Bota de hule", { "Bota de hule", "BASURA", "Plástico y caucho", "Los objetos de caucho liberan aditivos químicos al agua mientras se desgastan.", false } },
        { "Pila", { "Pila", "BASURA", "Residuo tóxico", "Libera metales pesados como mercurio, cadmio o plomo. Las pilas se llevan a centros de acopio, nunca a la basura.", false } },
        { "Barril de residuos toxicos", { "Barril de residuos tóxicos", "BASURA", "Residuo peligroso", "Al corroerse filtra químicos que envenenan el agua y el fondo marino. Retíralo con un clic.", false } },
        { "Ancla oxidada", { "Ancla oxidada", "NAUFRAGIO", "Restos de un barco", "El hierro de anclas y cascos se oxida lentamente; con los años, corales y esponjas pueden crecer sobre él.", false } },
        { "Barco hundido", { "Barco de madera hundido", "NAUFRAGIO", "Restos de un velero", "Con los años, los naufragios se vuelven arrecifes artificiales: corales, esponjas y peces los colonizan.", false } },
        { "Timon", { "Timón de barco", "NAUFRAGIO", "Restos de un barco de madera", "La madera hundida es devorada por moluscos llamados teredos o 'gusanos de barco'.", false } },
        { "Columna", { "Columna clásica", "NAUFRAGIO", "Ruinas sumergidas", "Hay ciudades antiguas bajo el mar, como Baia en Italia, hundidas por la actividad volcánica de la zona.", false } },
        { "Cofre del tesoro", { "Cofre del tesoro", "NAUFRAGIO", "Recompensa final", "Guarda la recompensa por limpiar el naufragio. Termina la misión y acércate para abrirlo.", false } },
        { "Senal de transito", { "Señal de tránsito", "BASURA", "Huella humana", "Objetos de la ciudad llegan al mar arrastrados por ríos, drenajes y tormentas.", false } },
        { "Dron de exploracion", { "Dron submarino ROV-01", "GUÍA", "Vehículo de exploración", "Te acompaña y señala con un marcador la basura más cercana. Los ROV reales se usan para estudiar y limpiar el fondo marino.", false } },
        { "Letrero: como limpiar", { "Cómo limpiar este fondo", "INSTRUCCIONES", "Letrero", "Apunta y haz clic en los residuos. Retira a los peces león. Mantén presionado el clic sobre la red para liberar a la tortuga. Retira el barril tóxico.", false } },
        { "Letrero: plasticos", { "Plásticos en el mar", "INFORMACIÓN", "Letrero", "Cada año entran al océano millones de toneladas de plástico. Se rompe en microplásticos que llegan a nuestra comida.", false } },
        { "Letrero: redes fantasma", { "Redes fantasma", "INFORMACIÓN", "Letrero", "Redes perdidas o abandonadas que siguen atrapando animales durante años. Son cerca del 10% de la basura marina.", false } },
        { "Letrero: pez leon", { "Pez león invasor", "INFORMACIÓN", "Letrero", "Llegó al Caribe desde el Indo-Pacífico, muy probablemente por liberación de acuarios. En México se promueve su pesca y consumo para controlarlo.", false } },
        { "Letrero: residuos toxicos", { "Residuos tóxicos", "INFORMACIÓN", "Letrero", "Barriles, pilas y combustibles liberan metales pesados que se acumulan del plancton a los peces que comemos.", false } },
    };
    return f;
}

inline const Ficha* BuscarFicha(const std::string& etiqueta)
{
    auto it = Fichas().find(etiqueta);
    return it == Fichas().end() ? nullptr : &it->second;
}

#endif
