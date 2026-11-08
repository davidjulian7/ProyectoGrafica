#version 330 core
// ============================================================================================
// ZONA 1 · ARRECIFES · este shader SOLO lo usa esta zona: cambiarlo no afecta a las otras zonas.
// (las otras zonas tienen su propia copia en shaders/zona1_arrecifes, zona2_aguas_frias y zona3_naufragios)
// ============================================================================================
layout (location = 0) in vec3 aPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec3 vPos;
void main()
{
    vec4 m = model * vec4(aPos, 1.0);
    vPos = m.xyz;
    gl_Position = projection * view * m;
}
