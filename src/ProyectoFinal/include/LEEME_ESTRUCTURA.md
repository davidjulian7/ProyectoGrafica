# Estructura del código · Acuario Virtual Interactivo

Cada carpeta corresponde a un tema del curso. En Visual Studio aparecen con el número de la práctica.
**Truco:** `Ctrl + Shift + F` y busca **`[CAMBIAR]`** o **`[AGREGAR]`**: son los puntos preparados para modificar.

## Cada zona es independiente

Todo lo que se ve y se siente en una zona está en **su** carpeta. Si cambias algo ahí, **no afecta a las otras zonas**:

| Qué | Zona 1 · Arrecifes | Zona 2 · Aguas Frías | Zona 3 · Naufragios |
|---|---|---|---|
| Objetos, luz, niebla, nado, cámara, velocidad de animales y materiales | `include/zonas/arrecifes/arrecifes_escena.h` | `include/zonas/aguas_frias/aguas_frias_escena.h` | `include/zonas/naufragios/naufragios_escena.h` |
| Reglas del juego y HUD | `arrecifes_mecanica.h` | `aguas_frias_mecanica.h` | `naufragios_mecanica.h` |
| Shaders (luces, Phong, linterna, niebla, ondulación, Fresnel) | `bin/shaders/zona1_arrecifes/` | `bin/shaders/zona2_aguas_frias/` | `bin/shaders/zona3_naufragios/` |
| Modelos y texturas | `bin/models/zonas/arrecifes/` | `bin/models/zonas/aguas_frias/` | `bin/models/zonas/naufragios/` |

En cada carpeta de shaders: `objetos.vs/.fs` (los objetos), `fondo.vs/.fs` (el agua del fondo) y `superficie.vs/.fs` (la superficie vista desde abajo).

**Lo único común a todas las zonas** (cámbialo solo si te piden algo para todo el programa): `ProyectoFinal.cpp` (teclas, proyección, ventana), `camara/camera.h` (sensibilidad del ratón), el menú (`interfaz/`) y el código de `escena/`, `animacion/`, `interaccion/` e `iluminacion/` (cómo se calcula, no los valores).

```
src/ProyectoFinal/
├── ProyectoFinal.cpp ............ ventana, bucle principal, menú ↔ zonas, teclado y ratón
└── include/
    ├── base_opengl/  (prácticas 00, 01, 05) shader_m.h · model.h · mesh.h · modelstructs.h
    ├── camara/       (práctica 02)          camera.h
    ├── iluminacion/  (prácticas 07, 08, 14) luces_y_materiales.h · light.h · material.h
    ├── texturas/     (práctica 06)          cubemap.h
    ├── animacion/    (prácticas 09–11)      keyframes.h · movimientos.h · particulas_zona.h · animatedmodel.h
    ├── interaccion/  (práctica 13)          picking.h · modo_edicion.h
    ├── interfaz/                            menu.h · hud_comun.h · fichas.h
    ├── escena/                              escenario.h · estructuras.h · colocar_modelos.h · render.h
    └── zonas/
        ├── arrecifes/    arrecifes_escena.h (qué hay) · arrecifes_mecanica.h (reglas del juego)
        ├── aguas_frias/  aguas_frias_escena.h · aguas_frias_mecanica.h
        └── naufragios/   naufragios_escena.h · naufragios_mecanica.h
src/bin/shaders/<zona>/   objetos.vs (movimiento de vértices) · objetos.fs (luces, niebla, materiales) · superficie.fs (Fresnel)
src/bin/models/zonas/<zona>/   modelos .fbx y sus texturas
```

## Recorrido, indicaciones y pantalla limpia

- **Recorrido:** se elige una zona en el menú (Enter = Zona 1). Al completarla aparece "Siguiente zona…" y pasa sola a la siguiente (1 → 2 → 3 → pantalla final). Está en `ProyectoFinal.cpp`: `AvanzarRecorrido()`, `ESPERA_SIGUIENTE` (segundos de espera) y el bloque `zonaHecha` (qué significa "zona completada" en cada zona).
- **Indicaciones al entrar:** cada zona tiene su tarjeta en `zonas/<zona>/<zona>_mecanica.h` → función `Intro()` (título, historia, objetivos y controles). Se dibujan con `HUDIntro` en `interfaz/hud_comun.h`.
- **Pantalla limpia:** por defecto solo se ve el objetivo actual, el termómetro o barra compacta y un aviso. **H** muestra todos los paneles e **I** la ficha completa. El estado está en `interfaz/estado_hud.h` (`HUD().detallado`, `HUD().fichaAbierta`); las versiones compactas son `HUDObjetivo`, `HUDTermometroCompacto` y `HUDBarraCompacta` (`interfaz/hud_comun.h`), y la etiqueta bajo la mira es `EtiquetaCompacta` (`interfaz/menu.h`).
- **Animación del buzo:** patada de piernas y aletas, brazos y respiración en `bin/shaders/<zona>/objetos.vs` (bloque `if (uBuzo == 1)`); la fase y la rapidez se mandan en `escena/render.h`.

## Niveles de uso (Básico, Intermedio, Avanzado)

Salen del documento *Usuarios e interacción*: el novato recibe indicaciones al inicio de cada zona y una pista amarilla en lo interactivo; el intermedio solo ve las teclas en el borde y pistas en algunos modelos; el experto solo ve su avance y las teclas aparecen si se queda quieto.

| Qué | Dónde |
|---|---|
| Nivel elegido, nombres, descripciones, distancia de la pista (12 m) y segundos quieto (8 s) | `interfaz/estado_hud.h` (`HUD().nivel`, `PISTA_DISTANCIA_INTERMEDIO`, `AYUDA_QUIETO_AVANZADO`) |
| Botones del menú (clic, ↑ / ↓) | `interfaz/menu.h` → `DibujarSelectorNivel()`, `OnKey()`, `OnClick()`, `Layout()` |
| Tarjeta al entrar (solo Básico) | `ProyectoFinal.cpp` → `EntrarZona()` |
| Tecla **U** (cambiar de nivel en la zona) y tiempo quieto | `ProyectoFinal.cpp` → `key_callback`, `Update()`, `mouse_callback` |
| Teclas en el borde | `interfaz/hud_comun.h` → `HUDTeclasBorde()` · las líneas en `ProyectoFinal.cpp` → `TeclasZona()` |
| Solo el avance (Avanzado) | `interfaz/hud_comun.h` → `HUDObjetivo()` |
| Pista amarilla en objetos interactivos | `escena/render.h` → `PistaNivel()` · color en `bin/shaders/<zona>/objetos.fs` (`uPista`) |

## ¿Qué me pueden pedir y dónde lo cambio?

| Te piden… | Archivo | Qué buscar / cambiar |
|---|---|---|
| **Cámara más rápida o lenta (ratón)** | `camara/camera.h` | `SENSITIVITY` |
| **Campo de visión (zoom)** | `zonas/<zona>/<zona>_escena.h` | `cfg.campoVision` (45 normal) |
| **Velocidad de nado del buzo** | `zonas/<zona>/<zona>_escena.h` | `cfg.nadoAceleracion`, `cfg.nadoResistencia` |
| **Perspectiva: near / far** | `ProyectoFinal.cpp` | `glm::perspective(...)` → `0.1f` y `400.0f` |
| **Proyección ortográfica** | `ProyectoFinal.cpp` | `glm::ortho(...)` (tecla 5) |
| **Tamaño de ventana / viewport** | `ProyectoFinal.cpp` | `SCR_WIDTH`, `SCR_HEIGHT` |
| **Modo líneas / puntos** | `ProyectoFinal.cpp` | teclas `M`, `N`, `B` (`glPolygonMode`) |
| **Color o intensidad del sol, luz ambiental** | `zonas/<zona>/<zona>_escena.h` | bloque `cfg.solColor`, `cfg.ambArriba`… |
| **Color del agua / niebla más o menos densa** | `zonas/<zona>/<zona>_escena.h` | `cfg.niebla`, `cfg.densidad` |
| **Linterna (luz tipo reflector)** | `bin/shaders/<zona>/objetos.fs` | `smoothstep(0.80, 0.95` = apertura; color en la misma sección |
| **Brillo especular / Phong** | `bin/shaders/<zona>/objetos.fs` | `float spec = pow(...)` |
| **Material de un objeto** | `zonas/<zona>/<zona>_escena.h` | `MatOpaco(e)`, `MatPlastico(e)`, `MatTraslucido(e)`, `MatMetalico(e)` |
| **Valores de cada material** | `zonas/<zona>/<zona>_escena.h` | `cfg.matOpaco`, `cfg.matPlastico`, `cfg.matTraslucido`, `cfg.matMetalico` |
| **Transparencia / Fresnel** | objeto: `e.alpha`, `e.fantasma` · superficie: `bin/shaders/<zona>/superficie.fs` | |
| **Cambiar una textura** | `bin/models/zonas/<zona>/` | reemplaza el `.jpg` con el mismo nombre · o tiñe con `e.tinte` |
| **Mover / girar / escalar un objeto** | `zonas/<zona>/<zona>_escena.h` | en su línea `Fijo(...)`: posición, giro Y, escala · o en vivo con la tecla **E** |
| **Agregar un modelo** | `zonas/<zona>/<zona>_escena.h` | plantilla `[AGREGAR]` al final de `ConstruirArrecifes()` |
| **Velocidad de los animales** | `zonas/<zona>/<zona>_escena.h` | `cfg.velFauna` (todos los de la zona) · `.vel` de cada animal |
| **Tipo de movimiento de un animal** | su zona | `MOV_ORBITA`, `MOV_CARDUMEN`, `MOV_LIBRE`, `MOV_FIJO`, `MOV_KF` (ver `escena/estructuras.h`) |
| **Agregar animación por cuadros clave** | `animacion/keyframes.h` | plantilla `KF_Plantilla` al final |
| **Ondulación al nadar (procedural)** | `bin/shaders/<zona>/objetos.vs` | `float onda = ...` |
| **Burbujas (partículas)** | `animacion/particulas_zona.h` | `respiracion`, `fuenteTiempo` |
| **Selección con la mira (picking)** | `interaccion/picking.h` | `R` = área de puntería; un objeto se selecciona si `e.interactivo = true` |
| **Texto de las indicaciones de una zona** | `zonas/<zona>/<zona>_mecanica.h` | función `Intro()` |
| **Tiempo para pasar a la siguiente zona** | `ProyectoFinal.cpp` | `ESPERA_SIGUIENTE` |
| **Movimiento de piernas del buzo** | `bin/shaders/<zona>/objetos.vs` | `if (uBuzo == 1)`: amplitud `0.14 + 0.38` |
| **Fotos de las tarjetas del menú** | `bin/textures/menu/` | reemplaza `foto_zona1_arrecifes.jpg`, etc. (960 × 1200, formato 4:5) |
| **Animación y textos de las tarjetas** | `interfaz/menu.h` | `DibujarTarjetaFoto()`: zoom, paneo, textos · destello en `bin/shaders/menu_ui.fs` (modo 4) |
| **Agregar una tecla** | `ProyectoFinal.cpp` | plantilla `[AGREGAR]` dentro de `key_callback` |
| **Ficha educativa nueva** | `interfaz/fichas.h` | copia una línea del mapa (la clave es la `etiqueta` del objeto) |
| **Temperaturas de la Zona 1 / 2** | `zonas/<zona>/<zona>_mecanica.h` | constantes `T_MIN`, `T_MAX`, `T_UMBRAL`… |
| **Orden de dibujo / un pase nuevo** | `escena/render.h` | `Dibujar()` |

## Cómo fluye un cuadro (para explicarlo)

1. `ProyectoFinal.cpp → Update()`: lee el teclado (`processInput`) y mueve al buzo.
2. `escenario.Actualizar()` (`animacion/particulas_zona.h`): burbujas y monedas.
3. La mecánica de la zona (`zonas/<zona>/<zona>_mecanica.h → Actualizar`): reglas del juego.
4. `escenario.Picking()` (`interaccion/picking.h`): qué señala la mira.
5. `escenario.Dibujar()` (`escena/render.h`): fondo → terreno → objetos (`animacion/movimientos.h` calcula cada matriz y `iluminacion/luces_y_materiales.h` envía luces y material) → superficie → transparentes.
6. HUD (`interfaz/`): mira, ficha, paneles y avisos.

## Por qué `escenario.h` incluye archivos al final

`Escenario` es una sola clase, pero su código está repartido por tema: en `escenario.h` solo está la lista de lo que hace, y cada método vive en el archivo de su carpeta como `inline void Escenario::Metodo() { ... }`. Al final de `escenario.h` se incluyen esos archivos. **No los incluyas en otro lado.**
