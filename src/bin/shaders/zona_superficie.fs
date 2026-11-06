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

vec3 cieloArriba(vec3 d)
{
    vec3 horizonte = vec3(0.80, 0.90, 0.96), cenit = vec3(0.26, 0.48, 0.80);
    vec3 c = mix(horizonte, cenit, pow(clamp(d.y, 0.0, 1.0), 0.55));
    float s = max(dot(d, normalize(-uSolDir)), 0.0);
    return c + vec3(1.0, 0.95, 0.85) * (pow(s, 600.0) * 4.0 + pow(s, 12.0) * 0.18);
}

void main()
{
    vec3 I = normalize(vPos - uCamPos);
    if (uCamPos.y > vPos.y) {   // vista desde arriba: agua verdeazulada con reflejo del cielo
        vec2 q2 = vPos.xz * 0.35;
        vec3 n2 = normalize(vec3(sin(q2.x * 1.7 + uTiempo * 1.1) * 0.06 + sin(q2.y * 2.3 - uTiempo * 0.9) * 0.04, 1.0,
                                 cos(q2.y * 1.9 + uTiempo * 0.8) * 0.06 + sin(q2.x * 2.9 + uTiempo * 1.3) * 0.03));
        vec3 R2 = reflect(I, n2);
        float f2 = 0.03 + 0.97 * pow(1.0 - clamp(dot(-I, n2), 0.0, 1.0), 5.0);
        vec3 prof = mix(vec3(0.05, 0.22, 0.30), vec3(0.12, 0.42, 0.50), 0.5 + 0.5 * sin(q2.x * 3.0 + uTiempo * 0.5) * sin(q2.y * 3.0 - uTiempo * 0.4));
        vec3 c2 = mix(prof, cieloArriba(vec3(R2.x, abs(R2.y), R2.z)), clamp(f2 * 1.4, 0.0, 1.0));
        float d2 = length(vPos - uCamPos);
        c2 = mix(c2, vec3(0.80, 0.90, 0.96), 1.0 - exp(-pow(d2 * 0.004, 1.5)));
        FragColor = vec4(c2, 0.82);
        return;
    }
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
