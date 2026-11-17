#version 330 core
// Modos: 0 = imagen con esquinas redondeadas y borde
//        1 = color solido redondeado (paneles y transiciones)
//        2 = resplandor alrededor de un rectangulo
//        3 = letra (alfa del atlas de fuente, color uColor)
//        4 = foto viva de una tarjeta del menu: degradado para el texto, vineta y destello al enfocar
in vec2 vUV;
in vec2 vTex;
out vec4 FragColor;

uniform sampler2D uTex;
uniform int   uMode;
uniform vec4  uColor;    // modo 0: color del borde; modos 1 y 2: color
uniform vec2  uSize;     // tamano del rectangulo en pixeles
uniform float uRadius;   // radio de las esquinas en pixeles
uniform float uAlpha;    // opacidad general
uniform float uBright;   // brillo de la imagen
uniform float uBorder;   // grosor del borde en pixeles
uniform float uGlowPad;  // margen del resplandor en pixeles
uniform float uTime;     // modo 4: tiempo (destello)
uniform float uFoco;     // modo 4: 0..1 tarjeta enfocada

float sdRoundBox(vec2 p, vec2 b, float r)
{
    vec2 q = abs(p) - b + r;
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
}

void main()
{
    vec2 p = (vUV - 0.5) * uSize;

    if (uMode == 0) {
        float d = sdRoundBox(p, uSize * 0.5, uRadius);
        float aa = 1.0 - smoothstep(-1.0, 0.5, d);
        vec4 c = texture(uTex, vTex);
        c.rgb *= uBright;
        float border = smoothstep(-uBorder - 1.0, -uBorder, d);
        c.rgb = mix(c.rgb, uColor.rgb, border * uColor.a);
        FragColor = vec4(c.rgb, c.a * aa * uAlpha);
    }
    else if (uMode == 1) {
        float d = sdRoundBox(p, uSize * 0.5, uRadius);
        float aa = 1.0 - smoothstep(-1.0, 0.5, d);
        FragColor = vec4(uColor.rgb, uColor.a * aa * uAlpha);
    }
    else if (uMode == 4) {
        float d = sdRoundBox(p, uSize * 0.5, uRadius);
        float aa = 1.0 - smoothstep(-1.0, 0.5, d);
        vec3 c = texture(uTex, vTex).rgb * uBright;
        // [CAMBIAR] degradado inferior (para que se lea el texto) y vineta
        float g = smoothstep(0.38, 1.0, vUV.y);
        c = mix(c, vec3(0.0, 0.025, 0.05), g * 0.88);
        vec2 q = vUV - 0.5;
        c *= 1.0 - dot(q, q) * 0.85;
        // destello diagonal que cruza la tarjeta enfocada
        float ciclo = fract(uTime * 0.35);
        float banda = (vUV.x * 0.8 + vUV.y * 0.6) - (ciclo * 2.4 - 0.5);
        c += vec3(1.0, 0.96, 0.88) * exp(-banda * banda * 70.0) * 0.32 * uFoco;
        // borde
        float border = smoothstep(-uBorder - 1.0, -uBorder, d);
        c = mix(c, uColor.rgb, border * uColor.a);
        FragColor = vec4(c, aa * uAlpha);
    }
    else if (uMode == 3) {
        FragColor = vec4(uColor.rgb, texture(uTex, vTex).a * uColor.a * uAlpha);
    }
    else {
        float d = sdRoundBox(p, uSize * 0.5 - uGlowPad, uRadius);
        float g = d < 0.0 ? 1.0 : exp(-d / (uGlowPad * 0.32));
        FragColor = vec4(uColor.rgb, uColor.a * g * uAlpha);
    }
}
