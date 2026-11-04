#version 330 core
// Rectangulos de interfaz en coordenadas de pixeles (origen arriba a la izquierda).
layout (location = 0) in vec2 aPos;   // 0..1

uniform vec4 uRect;     // x, y, ancho, alto en pixeles
uniform vec2 uScreen;   // tamano de la ventana en pixeles
uniform vec4 uUV;       // region de la textura: u, v, ancho, alto (0,0,1,1 = completa)

out vec2 vUV;           // coordenada local 0..1 (para esquinas redondeadas)
out vec2 vTex;          // coordenada de textura

void main()
{
    vec2 px = uRect.xy + aPos * uRect.zw;
    vec2 ndc = vec2(px.x / uScreen.x * 2.0 - 1.0, 1.0 - px.y / uScreen.y * 2.0);
    vUV = aPos;
    vTex = uUV.xy + aPos * uUV.zw;
    gl_Position = vec4(ndc, 0.0, 1.0);
}
