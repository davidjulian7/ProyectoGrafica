# Proyecto Final - Computación Gráfica

**Escena acuática interactiva en OpenGL/C++**

Proyecto final del curso de Computación Gráfica e Interacción Humano-Computadora.
Simula un mundo submarino con peces nadando, burbujas subiendo y un piso con efecto espejo.

## Técnicas integradas

| Técnica | Práctica origen | Descripción |
|---------|----------------|-------------|
| Carga de modelos 3D | 05 | Assimp carga FBX (PezPayaso, plano) |
| Texturas | 06 | Mapeo UV en modelos |
| Iluminación Phong múltiple | 07-08 | 4 luces con colores acuáticos |
| Cubemap | 09-10 | Fondo submarino 360° |
| Animación de partículas | 11 | Burbujas que suben (gravedad invertida) |
| Reflexión (espejo) | 12 | Piso del océano refleja los peces |
| Picking | 13 | Click para seleccionar peces |
| Fresnel | 14 | Superficie del agua con refracción |
| Animación procedural | 09-10 | Peces nadando en órbitas circulares |
| Audio | 11 | Sonido ambiental con irrKlang (F1) |

## Estructura del proyecto

```
ComputoGrafica/
├── src/
│   ├── ProyectoFinal/          ← PROYECTO FINAL
│   │   ├── ProyectoFinal.cpp   ← Código principal
│   │   ├── stb_image.cpp
│   │   ├── include/            ← Headers (cámara, shaders, modelos, etc.)
│   │   └── Proyecto/
│   │       └── ProyectoFinal/
│   │           ├── ProyectoFinal.sln
│   │           └── ProyectoFinal.vcxproj
│   ├── 00-14_*                 ← Prácticas del curso
│   ├── bin/                    ← Ejecutables + assets (models, shaders, textures, sound)
│   └── deps/                   ← Dependencias (glad, glfw, glm, assimp, irrklang)
└── ModelosProyecto/            ← Modelos .blend originales
```

## Requisitos

- **Visual Studio 2022** con workload "Desarrollo de escritorio en C++"
- **Windows 10/11** x64
- No se necesitan dependencias externas: todo está en `src/deps/`

## Compilar y ejecutar

1. Clonar el repositorio:
   ```bash
   git clone <URL_DEL_REPO>
   cd ComputoGrafica
   ```

2. Abrir la solución:
   ```
   src/ProyectoFinal/Proyecto/ProyectoFinal/ProyectoFinal.sln
   ```

3. Seleccionar configuración **Debug | x64** (o Release | x64)

4. Presionar **F5** (Debug) o **Ctrl+F5** (sin debug)

5. El ejecutable se genera en `src/bin/ProyectoFinal.exe` y se ejecuta con working directory en `src/bin/` (donde están los assets)

## Controles

| Tecla/Acción | Función |
|--------------|---------|
| **W A S D** | Mover cámara |
| **Mouse** | Rotar cámara (arrastrar) |
| **Scroll** | Zoom |
| **Click izquierdo** | Seleccionar pez (se agranda) |
| **M / N / B** | Wireframe / Fill / Points |
| **F1** | Activar sonido ambiental |
| **ESC** | Salir |

## Solución de problemas

- **"No se encuentran shaders/models"**: Verificar que el working directory sea `src/bin/` (configurado en `.vcxproj.user`)
- **Errores de librerías**: Asegurarse de usar **x64**, no Win32
- **Modelo PezPayaso no aparece**: Verificar que exista `src/bin/models/PezPayaso.fbx`

## Equipo

- Muñoz Norberto David Julian
- [Nombres de compañeros]

## Licencia

Proyecto académico - Curso de Computación Gráfica
