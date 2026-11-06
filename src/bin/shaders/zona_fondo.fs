#version 330 core
// Agua de fondo: color segun la direccion de la mirada, brillo del sol y rayos de luz.
out vec4 FragColor;

uniform mat4  uInvVP;      // inversa de (proyeccion * vista sin traslacion)
uniform vec2  uRes;
uniform vec3  uNiebla;
uniform vec3  uSolDir;
uniform vec3  uSolColor;
uniform float uTiempo;
uniform float uProfCam;    // metros bajo la superficie
uniform int   uArriba;     // 1 si la camara esta sobre el agua

vec3 colorAgua(vec3 d)
{
    vec3 c = mix(uNiebla * 0.35, uNiebla, smoothstep(-0.7, 0.05, d.y));
    return mix(c, uNiebla * 1.35 + vec3(0.03), smoothstep(0.05, 0.9, d.y));
}

vec3 cielo(vec3 d, vec3 L)
{
    vec3 horizonte = vec3(0.80, 0.90, 0.96), cenit = vec3(0.26, 0.48, 0.80);
    vec3 c = mix(horizonte, cenit, pow(clamp(d.y, 0.0, 1.0), 0.55));
    float s = max(dot(d, L), 0.0);
    c += vec3(1.0, 0.95, 0.85) * (pow(s, 600.0) * 4.0 + pow(s, 12.0) * 0.18);
    if (d.y < 0.0) c = mix(horizonte, vec3(0.10, 0.30, 0.38), smoothstep(0.0, 0.25, -d.y));   // mar lejano
    return c;
}

void main()
{
    if (uArriba == 1) {
        vec2 nd = gl_FragCoord.xy / uRes * 2.0 - 1.0;
        vec4 pp = uInvVP * vec4(nd, 1.0, 1.0);
        FragColor = vec4(cielo(normalize(pp.xyz / pp.w), normalize(-uSolDir)), 1.0);
        return;
    }
    vec2 ndc = gl_FragCoord.xy / uRes * 2.0 - 1.0;
    vec4 p = uInvVP * vec4(ndc, 1.0, 1.0);
    vec3 d = normalize(p.xyz / p.w);
    vec3 c = colorAgua(d);

    vec3 L = normalize(-uSolDir);
    float s = max(dot(d, L), 0.0);
    float vis = exp(-uProfCam * 0.05);
    c += uSolColor * (pow(s, 90.0) * 0.6 + pow(s, 10.0) * 0.12) * vis;

    // rayos de luz que convergen hacia el sol
    vec3 a = normalize(cross(L, vec3(0.0, 0.0, 1.0)));
    vec3 b = cross(L, a);
    float ang = atan(dot(d, b), dot(d, a));
    float r = pow(0.5 + 0.5 * sin(ang * 17.0 + uTiempo * 0.30), 8.0) * 0.6
            + pow(0.5 + 0.5 * sin(ang * 9.0 - uTiempo * 0.21 + 1.0), 10.0) * 0.5;
    r *= smoothstep(0.15, 0.95, s);
    c += uSolColor * r * 0.10 * vis;

    FragColor = vec4(c, 1.0);
}
