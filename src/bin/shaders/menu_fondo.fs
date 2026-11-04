#version 330 core
// Fondo submarino animado del menu de inicio.
// Capas: degradado de profundidad, causticas, rayos de luz, relieve del fondo,
// algas que se mecen, nieve marina, burbujas y vineta.
out vec4 FragColor;

uniform vec2  uRes;     // tamano de la ventana en pixeles
uniform float uTime;    // tiempo en segundos
uniform vec3  uTint;    // tono de la zona enfocada
uniform vec2  uMouse;   // posicion del raton (0..1) para paralaje

float hash(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }

float noise(vec2 p)
{
    vec2 i = floor(p), f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i), hash(i + vec2(1, 0)), u.x),
               mix(hash(i + vec2(0, 1)), hash(i + vec2(1, 1)), u.x), u.y);
}

float fbm(vec2 p)
{
    float v = 0.0, a = 0.5;
    for (int i = 0; i < 5; i++) { v += a * noise(p); p *= 2.0; a *= 0.5; }
    return v;
}

// Causticas: iteracion clasica de patrones de luz refractada
float caustic(vec2 q, float t)
{
    vec2 p = q * 6.28318 - 250.0;
    vec2 i = p;
    float c = 1.0;
    float inten = 0.005;
    for (int n = 0; n < 5; n++) {
        float tt = t * (1.0 - (3.5 / float(n + 1)));
        i = p + vec2(cos(tt - i.x) + sin(tt + i.y), sin(tt - i.y) + cos(tt + i.x));
        c += 1.0 / length(vec2(p.x / (sin(i.x + tt) / inten), p.y / (cos(i.y + tt) / inten)));
    }
    c /= 5.0;
    c = 1.17 - pow(c, 1.4);
    return pow(abs(c), 8.0);
}

void main()
{
    vec2 uv = gl_FragCoord.xy / uRes;          // 0 abajo, 1 arriba
    float aspect = uRes.x / uRes.y;
    vec2 p = vec2((uv.x - 0.5) * aspect, uv.y);
    vec2 par = (uMouse - 0.5) * vec2(0.03, -0.02);

    // 1. Degradado de profundidad
    vec3 deep = vec3(0.004, 0.025, 0.06);
    vec3 mid  = vec3(0.02, 0.17, 0.28);
    vec3 top  = vec3(0.09, 0.50, 0.66);
    vec3 col = mix(deep, mid, smoothstep(0.0, 0.62, uv.y));
    col = mix(col, top, smoothstep(0.55, 1.0, uv.y));
    col = mix(col, col * uTint * 1.7, 0.30);

    // 2. Causticas cerca de la superficie
    float c = caustic(vec2(p.x * 0.55, uv.y * 0.55) + par * 2.0, uTime * 0.25);
    col += vec3(0.55, 0.88, 1.0) * clamp(c, 0.0, 1.0) * 0.35 * smoothstep(0.60, 1.0, uv.y);

    // 3. Rayos de luz desde la superficie
    vec2 src = vec2(0.25 * sin(uTime * 0.04) - par.x * 4.0, 1.35);
    vec2 d = p - src;
    float ang = atan(d.x, -d.y);
    float r = 0.0;
    r += pow(0.5 + 0.5 * sin(ang * 13.0 + uTime * 0.21), 6.0) * 0.55;
    r += pow(0.5 + 0.5 * sin(ang * 7.0 - uTime * 0.15 + 1.3), 8.0) * 0.65;
    r += pow(0.5 + 0.5 * sin(ang * 23.0 + uTime * 0.10 + 2.1), 10.0) * 0.35;
    r *= smoothstep(0.05, 0.95, uv.y) * (0.55 + 0.45 * fbm(vec2(ang * 5.0, uTime * 0.08)));
    col += vec3(0.50, 0.82, 0.95) * r * 0.20;

    // 4. Relieve lejano
    float px = p.x + par.x * 1.5;
    float ridge = 0.24 + 0.04 * sin(px * 2.3 + 2.4) + 0.06 * fbm(vec2(px * 1.4 + 7.0, 3.0));
    col = mix(col, mix(col, vec3(0.01, 0.07, 0.11), 0.65), smoothstep(ridge + 0.004, ridge - 0.004, uv.y));

    // 5. Algas que se mecen
    float dune = 0.12 + 0.03 * sin(p.x * 3.1 + 0.7) + 0.02 * sin(p.x * 7.3 + 2.0) + 0.04 * fbm(vec2(p.x * 2.0, 1.0));
    float kelp = 0.0;
    for (int k = 0; k < 11; k++) {
        float fk = float(k);
        float x0 = (hash(vec2(fk, 3.1)) - 0.5) * aspect * 1.05;
        float h = 0.10 + 0.20 * hash(vec2(fk, 7.7));
        float yy = uv.y - dune + 0.02;
        if (yy > 0.0 && yy < h) {
            float sway = sin(uTime * 0.8 + fk * 1.7 + yy * 6.0) * 0.035 * (yy / h) + sin(yy * 14.0 + fk) * 0.006;
            float w = 0.016 * (1.0 - yy / h) + 0.003;
            float dx = abs(p.x - x0 - sway);
            kelp = max(kelp, smoothstep(w, w * 0.35, dx));
        }
    }
    col = mix(col, vec3(0.012, 0.085, 0.075), kelp * 0.85);

    // 6. Fondo marino cercano
    col = mix(col, vec3(0.006, 0.025, 0.04), smoothstep(dune + 0.003, dune - 0.003, uv.y));

    // 7. Nieve marina (tres capas con paralaje)
    for (int l = 0; l < 3; l++) {
        float fl = float(l);
        float scale = 14.0 + fl * 11.0;
        vec2 q = vec2(p.x + par.x * (1.0 + fl), uv.y + uTime * (0.012 + 0.010 * fl)) * scale;
        vec2 id = floor(q);
        vec2 f = fract(q) - 0.5;
        float h = hash(id + fl * 13.0);
        vec2 off = vec2(hash(id + 1.3) - 0.5, hash(id + 7.1) - 0.5) * 0.6;
        float s = smoothstep(0.05 + 0.03 * h, 0.0, length(f - off));
        col += vec3(0.60, 0.80, 0.90) * s * step(0.72, h) * (0.12 + 0.08 * fl) * smoothstep(0.0, 0.3, uv.y);
    }

    // 8. Burbujas subiendo
    for (int b = 0; b < 16; b++) {
        float fb = float(b);
        float bx = (hash(vec2(fb, 1.0)) - 0.5) * aspect;
        float speed = 0.04 + 0.06 * hash(vec2(fb, 2.0));
        float by = fract(hash(vec2(fb, 3.0)) + uTime * speed) * 1.3 - 0.1;
        float wob = sin(uTime * 2.0 + fb) * 0.01;
        float rad = 0.004 + 0.009 * hash(vec2(fb, 4.0));
        float dist = length(vec2(p.x - bx - wob, uv.y - by));
        float ring = smoothstep(rad, rad * 0.75, dist) - smoothstep(rad * 0.75, rad * 0.45, dist) * 0.7;
        float shine = smoothstep(rad * 0.35, 0.0, length(vec2(p.x - bx - wob + rad * 0.35, uv.y - by - rad * 0.35)));
        col += vec3(0.70, 0.92, 1.0) * (ring * 0.45 + shine * 0.5);
    }

    // 9. Vineta
    vec2 vq = (uv - 0.5) * vec2(1.1, 1.25);
    col *= 1.0 - dot(vq, vq) * 0.85;

    FragColor = vec4(col, 1.0);
}
