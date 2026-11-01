#version 330 core
// Iluminacion submarina: luz solar que se absorbe con la profundidad (el rojo primero),
// causticas, luz ambiental de hemisferio, linterna del buzo y niebla exponencial.
// Incluye efectos preparados para las mecanicas: blanqueamiento y fantasma.
in vec3 vPosMundo;
in vec3 vNormal;
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D texture_diffuse1;
uniform vec3  uTinte;
uniform float uAlpha;
uniform float uEmision;
uniform float uResaltado;   // brillo del objeto bajo la mira
uniform float uBlanqueo;    // 0..1 coral sano -> blanqueado (Zona 1)
uniform float uFantasma;    // 0..1 animal real -> fantasma (Zona 2)

uniform vec3  uCamPos;
uniform vec3  uSolDir;
uniform vec3  uSolColor;
uniform vec3  uAmbArriba;
uniform vec3  uAmbAbajo;
uniform vec3  uNiebla;
uniform float uDensidad;
uniform float uCausticas;
uniform float uSuperficieY;
uniform float uTiempo;

uniform int   uLinterna;
uniform vec3  uLinternaPos;
uniform vec3  uLinternaDir;

vec3 colorAgua(vec3 d)
{
    vec3 c = mix(uNiebla * 0.35, uNiebla, smoothstep(-0.7, 0.05, d.y));
    return mix(c, uNiebla * 1.35 + vec3(0.03), smoothstep(0.05, 0.9, d.y));
}

float causticas(vec2 p, float t)
{
    vec2 q = p * 0.55;
    float c = 0.0;
    for (int i = 0; i < 3; i++) {
        q += vec2(sin(q.y * 1.7 + t * 0.8), cos(q.x * 1.3 + t * 0.6)) * 0.9;
        c += 1.0 / (1.0 + 14.0 * abs(sin(q.x + q.y) * cos(q.x - q.y)));
    }
    return pow(c / 3.0, 2.5);
}

void main()
{
    vec4 tex = texture(texture_diffuse1, vUV);
    vec3 base = tex.rgb * uTinte;
    float gris = dot(tex.rgb, vec3(0.333));
    base = mix(base, vec3(0.95, 0.95, 0.92) * (0.75 + 0.35 * gris), uBlanqueo);

    vec3 N = normalize(vNormal);
    if (!gl_FrontFacing) N = -N;
    vec3 V = normalize(uCamPos - vPosMundo);
    vec3 L = normalize(-uSolDir);

    // el agua absorbe primero el rojo, luego el verde
    float prof = max(uSuperficieY - vPosMundo.y, 0.0);
    vec3 absorcion = exp(-prof * vec3(0.09, 0.035, 0.022));

    float dif = max(dot(N, L), 0.0);
    float caus = uCausticas * causticas(vPosMundo.xz, uTiempo) * max(N.y, 0.0);
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), 40.0) * 0.25;
    vec3 amb = mix(uAmbAbajo, uAmbArriba, N.y * 0.5 + 0.5);

    vec3 color = base * (amb + uSolColor * absorcion * (dif * 0.85 + caus * 1.6)) + uSolColor * absorcion * spec;

    if (uLinterna == 1) {
        vec3 Lf = uLinternaPos - vPosMundo;
        float d = length(Lf);
        Lf /= d;
        float cono = smoothstep(0.80, 0.95, dot(-Lf, normalize(uLinternaDir)));
        float aten = 1.0 / (1.0 + 0.006 * d * d);
        color += base * vec3(1.0, 0.93, 0.78) * cono * aten * (0.3 + max(dot(N, Lf), 0.0)) * 3.2;
    }
    color += base * uEmision;

    float alpha = tex.a * uAlpha;
    if (uFantasma > 0.0) {
        float borde = pow(1.0 - max(dot(N, V), 0.0), 2.0);
        vec3 g = vec3(0.55, 0.90, 1.0) * (0.20 + borde * 1.6);
        color = mix(color, g, uFantasma);
        alpha = mix(alpha, 0.20 + borde * 0.7, uFantasma);
    }
    color += vec3(0.25, 0.55, 0.65) * uResaltado;

    // niebla submarina: el color hacia el que se pierde depende de hacia donde se mira
    float dist = length(uCamPos - vPosMundo);
    float f = 1.0 - exp(-pow(dist * uDensidad, 1.5));
    color = mix(color, colorAgua(-V), f);

    FragColor = vec4(color, alpha);
}
