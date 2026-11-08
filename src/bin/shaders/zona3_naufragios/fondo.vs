#version 330 core
// ============================================================================================
// ZONA 3 · NAUFRAGIOS · este shader SOLO lo usa esta zona: cambiarlo no afecta a las otras zonas.
// (las otras zonas tienen su propia copia en shaders/zona1_arrecifes, zona2_aguas_frias y zona3_naufragios)
// ============================================================================================
// Triangulo que cubre toda la pantalla, sin buffers de vertices.
void main()
{
    vec2 pos = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(pos * 2.0 - 1.0, 0.0, 1.0);
}
