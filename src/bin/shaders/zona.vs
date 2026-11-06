#version 330 core
// Objetos de las zonas: ondulacion del cuerpo al nadar y vaiven de algas.
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 5) in vec4 bIDs1;
layout (location = 6) in vec4 bIDs2;
layout (location = 7) in vec4 bIDs3;
layout (location = 8) in vec4 bWeights1;
layout (location = 9) in vec4 bWeights2;
layout (location = 10) in vec4 bWeights3;

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
uniform int   uSkinning;   // 1 = aplicar huesos de la armadura
uniform mat4  gBones[50];

out vec3 vPosMundo;
out vec3 vNormal;
out vec2 vUV;

void main()
{
    vec3 p = aPos;
    vec3 normal = aNormal;
    if (uSkinning != 0) {
        mat4 huesos = gBones[int(bIDs1[0])] * bWeights1[0];
        huesos += gBones[int(bIDs1[1])] * bWeights1[1];
        huesos += gBones[int(bIDs1[2])] * bWeights1[2];
        huesos += gBones[int(bIDs1[3])] * bWeights1[3];
        huesos += gBones[int(bIDs2[0])] * bWeights2[0];
        huesos += gBones[int(bIDs2[1])] * bWeights2[1];
        huesos += gBones[int(bIDs2[2])] * bWeights2[2];
        huesos += gBones[int(bIDs2[3])] * bWeights2[3];
        huesos += gBones[int(bIDs3[0])] * bWeights3[0];
        huesos += gBones[int(bIDs3[1])] * bWeights3[1];
        huesos += gBones[int(bIDs3[2])] * bWeights3[2];
        huesos += gBones[int(bIDs3[3])] * bWeights3[3];
        float peso = dot(bWeights1, vec4(1.0)) + dot(bWeights2, vec4(1.0)) + dot(bWeights3, vec4(1.0));
        if (peso > 0.0001) {
            huesos *= 1.0 / peso;
            p = (huesos * vec4(aPos, 1.0)).xyz;
            normal = mat3(huesos) * aNormal;
        }
    }
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
    vNormal = normalize(mat3(transpose(inverse(model))) * normal);
    vUV = aTexCoords;
    gl_Position = projection * view * mundo;
}
