#!/usr/bin/env bash
# Borra del repositorio archivos que el proyecto ya no usa (ejecutar una sola vez, en Git Bash, dentro de ProyectoGrafica).
cd "$(dirname "$0")"
git rm -q --ignore-unmatch \
  src/bin/models/zonas/arrecifes/ajolote.fbx src/bin/models/zonas/arrecifes/ajolote.jpg \
  src/bin/models/zonas/arrecifes/alga.fbx src/bin/models/zonas/arrecifes/alga.jpg \
  src/bin/models/zonas/arrecifes/pescado.fbx src/bin/models/zonas/arrecifes/pescado.jpg \
  src/bin/models/zonas/arrecifes/pezdracoide.fbx src/bin/models/zonas/arrecifes/pezdracoide.jpg \
  src/bin/models/zonas/naufragios/oxido.jpg src/bin/models/zonas/naufragios/madera_cofre.jpg \
  src/bin/models/zonas/naufragios/ruinas_muro.fbx src/bin/models/zonas/naufragios/ruinas_torre.fbx \
  src/bin/models/zonas/naufragios/columna_rota.fbx src/bin/models/zonas/naufragios/piedra_ruinas.jpg \
  src/bin/textures/menu/etiquetas/pez_dragon.png src/bin/textures/menu/etiquetas/atun.png \
  src/bin/textures/menu/etiquetas/alga.png src/bin/textures/menu/etiquetas/coral.png \
  LEEME_MENU.txt LEEME_ZONA1.txt LEEME_ZONA3.txt LEEME_COMPILAR.txt
echo "Listo: archivos sin uso eliminados. Ahora haz git commit y git push."
