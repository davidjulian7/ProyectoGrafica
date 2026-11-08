# Estructura del código · Acuario Virtual Interactivo

Cada carpeta corresponde a un tema del curso. En Visual Studio aparecen con el número de la práctica.
**Truco:** `Ctrl + Shift + F` y busca **`[CAMBIAR]`** o **`[AGREGAR]`**: son los puntos preparados para modificar.

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
src/bin/shaders/   zona.vs (movimiento de vértices) · zona.fs (luces, niebla, materiales) · zona_superficie.fs (Fresnel)
src/bin/models/zonas/<zona>/   modelos .fbx y sus texturas
```

## ¿Qué me pueden pedir y dónde lo cambio?

| Te piden… | Archivo | Qué buscar / cambiar |
|---|---|---|
| **Cámara más rápida o lenta (ratón)** | `camara/camera.h` | `SENSITIVITY` |
| **Campo de visión (zoom)** | `camara/camera.h` | `ZOOM` (45 normal) |
| **Velocidad de nado del buzo** | `ProyectoFinal.cpp` | `DIVER_ACCEL`, `DIVER_DRAG` |
| **Perspectiva: near / far** | `ProyectoFinal.cpp` | `glm::perspective(...)` → `0.1f` y `400.0f` |
| **Proyección ortográfica** | `ProyectoFinal.cpp` | `glm::ortho(...)` (tecla 5) |
| **Tamaño de ventana / viewport** | `ProyectoFinal.cpp` | `SCR_WIDTH`, `SCR_HEIGHT` |
| **Modo líneas / puntos** | `ProyectoFinal.cpp` | teclas `M`, `N`, `B` (`glPolygonMode`) |
| **Color o intensidad del sol, luz ambiental** | `zonas/<zona>/<zona>_escena.h` | bloque `cfg.solColor`, `cfg.ambArriba`… |
| **Color del agua / niebla más o menos densa** | `zonas/<zona>/<zona>_escena.h` | `cfg.niebla`, `cfg.densidad` |
| **Linterna (luz tipo reflector)** | `bin/shaders/zona.fs` | `smoothstep(0.80, 0.95` = apertura; color en la misma sección |
| **Brillo especular / Phong** | `bin/shaders/zona.fs` | `float spec = pow(...)` |
| **Material de un objeto** | `zonas/<zona>/<zona>_escena.h` | `MatOpaco(e)`, `MatPlastico(e)`, `MatTraslucido(e)`, `MatMetalico(e)` |
| **Valores de cada material** | `iluminacion/luces_y_materiales.h` | `MatOpaco` … `MatMetalico` (`specInt`, `specPower`, `metalico`) |
| **Transparencia / Fresnel** | objeto: `e.alpha`, `e.fantasma` · superficie: `bin/shaders/zona_superficie.fs` | |
| **Cambiar una textura** | `bin/models/zonas/<zona>/` | reemplaza el `.jpg` con el mismo nombre · o tiñe con `e.tinte` |
| **Mover / girar / escalar un objeto** | `zonas/<zona>/<zona>_escena.h` | en su línea `Fijo(...)`: posición, giro Y, escala · o en vivo con la tecla **E** |
| **Agregar un modelo** | `zonas/<zona>/<zona>_escena.h` | plantilla `[AGREGAR]` al final de `ConstruirArrecifes()` |
| **Velocidad de los animales** | `escena/escenario.h` | `velFauna` (todos) · `.vel` de cada animal en su zona |
| **Tipo de movimiento de un animal** | su zona | `MOV_ORBITA`, `MOV_CARDUMEN`, `MOV_LIBRE`, `MOV_FIJO`, `MOV_KF` (ver `escena/estructuras.h`) |
| **Agregar animación por cuadros clave** | `animacion/keyframes.h` | plantilla `KF_Plantilla` al final |
| **Ondulación al nadar (procedural)** | `bin/shaders/zona.vs` | `float onda = ...` |
| **Burbujas (partículas)** | `animacion/particulas_zona.h` | `respiracion`, `fuenteTiempo` |
| **Selección con la mira (picking)** | `interaccion/picking.h` | `R` = área de puntería; un objeto se selecciona si `e.interactivo = true` |
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
