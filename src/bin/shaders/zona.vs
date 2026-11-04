#version 330 core
// Objetos de las zonas: ondulacion del cuerpo al nadar y vaiven de algas.
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform float uTiempo;
uniform float uOndula;    // amplitud de la ondulacion (metros); 0 = rigido
uniform float uLargo;     // medio largo del modelo (la cabeza esta en +X)
uniform float uVelNado;   // velocidad de la ondulacion
uniform float uMecer;     // vaiven de algas y abanicos
uniform float uFase;
uniform int   uOndulaEje;  // 0 = cola de lado a lado (peces), 1 = de arriba a abajo (mamiferos)

out vec3 vPosMundo;
out vec3 vNormal;
out vec2 vUV;

void main()
{
    vec3 p = aPos;
    if (uOndula > 0.0) {
        // 0 en la cabeza, 1 en la cola: la cola se mueve mas que la cabeza
        float t = clamp((uLargo - p.x) / (2.0 * uLargo), 0.0, 1.0);
        float w = uOndula * pow(t, 1.6) * uLargo;
        float onda = w * sin(p.x / uLargo * 3.0 - uTiempo * uVelNado + uFase);
        if (uOndulaEje == 1) p.y += onda; else p.z += onda;
    }
    if (uMecer > 0.0) {
        float h = max(p.y, 0.0);
        p.x += sin(uTiempo * 0.9 + uFase + h * 0.7) * uMecer * h * 0.10;
        p.z += cos(uTiempo * 0.7 + uFase + h * 0.5) * uMecer * h * 0.07;
    }
    vec4 mundo = model * vec4(p, 1.0);
    vPosMundo = mundo.xyz;
    vNormal = mat3(transpose(inverse(model))) * aNormal;
    vUV = aTexCoords;
    gl_Position = projection * view * mundo;
}
