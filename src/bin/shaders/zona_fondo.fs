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

vec3 colorAgua(vec3 d)
{
    vec3 c = mix(uNiebla * 0.35, uNiebla, smoothstep(-0.7, 0.05, d.y));
    return mix(c, uNiebla * 1.35 + vec3(0.03), smoothstep(0.05, 0.9, d.y));
}

void main()
{
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
