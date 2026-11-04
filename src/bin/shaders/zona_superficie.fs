#version 330 core
// Superficie del agua vista desde abajo.
// Refraccion agua -> aire con la ley de Snell: dentro del "cono de Snell" (~48.6 grados)
// se ve el cielo; fuera de el hay reflexion total interna (se refleja el fondo, cubemap).
// La mezcla entre ambos usa la aproximacion de Schlick del termino de Fresnel.
in vec3 vPos;
out vec4 FragColor;

uniform vec3  uCamPos;
uniform vec3  uNiebla;
uniform float uDensidad;
uniform vec3  uSolDir;
uniform vec3  uSolColor;
uniform float uTiempo;
uniform samplerCube uCubo;

vec3 colorAgua(vec3 d)
{
    vec3 c = mix(uNiebla * 0.35, uNiebla, smoothstep(-0.7, 0.05, d.y));
    return mix(c, uNiebla * 1.35 + vec3(0.03), smoothstep(0.05, 0.9, d.y));
}

void main()
{
    vec3 I = normalize(vPos - uCamPos);
    vec2 q = vPos.xz * 0.35;
    float t = uTiempo;
    vec3 n = normalize(vec3(sin(q.x * 1.7 + t * 1.1) * 0.10 + sin(q.y * 2.3 - t * 0.9) * 0.07,
                            -1.0,
                            cos(q.y * 1.9 + t * 0.8) * 0.10 + sin(q.x * 2.9 + t * 1.3) * 0.05));

    vec3 T = refract(I, n, 1.333);           // de agua (1.333) a aire (1.0)
    vec3 R = reflect(I, n);
    float cosi = clamp(dot(-I, n), 0.0, 1.0);
    float fres = 0.02 + 0.98 * pow(1.0 - cosi, 5.0);

    vec3 reflejo = mix(colorAgua(R), texture(uCubo, R).rgb, 0.35) * 0.9;
    vec3 cielo = vec3(0.62, 0.84, 0.95) * uSolColor * 1.1
               + uSolColor * pow(max(dot(normalize(T + vec3(1e-5)), -normalize(uSolDir)), 0.0), 40.0) * 1.5;
    vec3 c = dot(T, T) < 1e-6 ? reflejo : mix(cielo, reflejo, fres);

    float dist = length(vPos - uCamPos);
    float f = 1.0 - exp(-pow(dist * uDensidad, 1.5));
    c = mix(c, colorAgua(I), f);
    FragColor = vec4(c, 1.0);
}
