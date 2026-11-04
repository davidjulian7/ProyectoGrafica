# Cubemap submarino procedural: superficie luminosa arriba, degradado de profundidad,
# rayos de luz, relieve oscuro en el horizonte y fondo marino abajo.
import numpy as np, sys, os
from PIL import Image
OUT = sys.argv[1]; os.makedirs(OUT, exist_ok=True)
N = 512

def noise3(p, seed=0):
    # ruido de valor 3D barato con interpolacion suave
    rng = np.random.default_rng(seed); perm = rng.random(4096)
    def h(ix, iy, iz): return perm[(ix * 73856093 ^ iy * 19349663 ^ iz * 83492791) % 4096]
    i = np.floor(p).astype(np.int64); f = p - i; u = f * f * (3 - 2 * f)
    x, y, z = i[..., 0], i[..., 1], i[..., 2]
    def L(a, b, t): return a + (b - a) * t
    n000, n100, n010, n110 = h(x, y, z), h(x + 1, y, z), h(x, y + 1, z), h(x + 1, y + 1, z)
    n001, n101, n011, n111 = h(x, y, z + 1), h(x + 1, y, z + 1), h(x, y + 1, z + 1), h(x + 1, y + 1, z + 1)
    return L(L(L(n000, n100, u[..., 0]), L(n010, n110, u[..., 0]), u[..., 1]),
             L(L(n001, n101, u[..., 0]), L(n011, n111, u[..., 0]), u[..., 1]), u[..., 2])

def fbm(p, oct=4, seed=0):
    v = 0; a = 0.5
    for o in range(oct): v = v + a * noise3(p, seed + o); p = p * 2.0; a *= 0.5
    return v

def color(d):
    y = d[..., 1]
    deep = np.array([0.01, 0.05, 0.10]); mid = np.array([0.03, 0.22, 0.34]); top = np.array([0.20, 0.62, 0.74])
    t = np.clip((y + 1) / 2, 0, 1)[..., None]
    c = np.where(t < 0.55, deep + (mid - deep) * (t / 0.55), mid + (top - mid) * ((t - 0.55) / 0.45))
    # superficie: ondas y causticas
    surf = np.clip((y - 0.55) / 0.45, 0, 1)[..., None]
    p = d[..., [0, 2]] / np.maximum(y[..., None], 0.2) * 3.0
    w = fbm(np.dstack([p[..., 0], p[..., 1], np.zeros_like(y)]) * 1.0, 4, 3)[..., None]
    c = c + surf * (0.25 * w + 0.35 * np.clip(1 - np.abs(np.sin(w * 18.0)), 0, 1) ** 6)
    # sol difuso en la superficie
    sun = np.array([0.25, 0.95, 0.15]); sun = sun / np.linalg.norm(sun)
    s = np.clip((d * sun).sum(-1), 0, 1)[..., None]
    c = c + np.array([0.8, 0.95, 1.0]) * (s ** 40 * 0.9 + s ** 6 * 0.15)
    # rayos de luz hacia abajo
    ang = np.arctan2(d[..., 0], d[..., 2])
    rays = (0.5 + 0.5 * np.sin(ang * 23 + 1.3)) ** 8 * 0.6 + (0.5 + 0.5 * np.sin(ang * 11)) ** 10 * 0.5
    rv = np.clip((y + 0.2) / 0.9, 0, 1) * np.clip((0.85 - y) / 0.4, 0, 1)
    c = c + (np.array([0.35, 0.65, 0.75]) * (rays * rv)[..., None] * 0.18)
    # relieve oscuro en el horizonte
    hz = fbm(np.dstack([d[..., 0] * 2.5, d[..., 2] * 2.5, np.zeros_like(y)]), 4, 9)
    ridge = -0.12 + 0.22 * hz
    m = np.clip((ridge - y) / 0.03, 0, 1)[..., None]
    c = c * (1 - m * 0.65) + np.array([0.01, 0.06, 0.09]) * m * 0.65
    # fondo marino
    sea = np.clip((-y - 0.35) / 0.4, 0, 1)[..., None]
    sand = 0.55 + 0.45 * fbm(d * 6.0, 4, 21)[..., None]
    c = c * (1 - sea) + np.array([0.04, 0.10, 0.12]) * sand * sea
    return np.clip(c, 0, 1)

v, u = np.mgrid[0:N, 0:N]
u = 2 * (u + 0.5) / N - 1; v = 2 * (v + 0.5) / N - 1
one = np.ones_like(u)
faces = {
    "posx": np.dstack([one, -v, -u]), "negx": np.dstack([-one, -v, u]),
    "posy": np.dstack([u, one, v]),   "negy": np.dstack([u, -one, -v]),
    "posz": np.dstack([u, -v, one]),  "negz": np.dstack([-u, -v, -one]),
}
for name, d in faces.items():
    d = d / np.linalg.norm(d, axis=-1, keepdims=True)
    img = (color(d) ** (1 / 1.05) * 255).astype(np.uint8)
    Image.fromarray(img, "RGB").save(f"{OUT}/{name}.png")
print("ok")
