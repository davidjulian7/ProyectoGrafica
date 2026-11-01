# Genera el arte del menu de inicio del Acuario Virtual Interactivo.
# Todo es procedural (PIL + numpy) para no depender de imagenes con derechos.
import math, random, os, sys
import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont, ImageChops

OUT = sys.argv[1] if len(sys.argv) > 1 else "out"
os.makedirs(OUT, exist_ok=True)

FD = "/usr/share/fonts/opentype/noto/"
def font(weight, size):
    return ImageFont.truetype(FD + f"NotoSansCJK-{weight}.ttc", size, index=0)

SS = 2                      # supermuestreo para antialias
CW, CH = 600, 750           # tamano final de cada tarjeta
W, H = CW * SS, CH * SS

# ---------------------------------------------------------------- utilidades
def lerp(a, b, t): return a + (b - a) * t
def mix(c1, c2, t): return tuple(int(lerp(c1[i], c2[i], t)) for i in range(len(c1)))

def gradient(top, bottom, w=W, h=H, power=1.0):
    t = np.linspace(0, 1, h)[:, None] ** power
    top = np.array(top, float); bottom = np.array(bottom, float)
    img = top[None, None, :] * (1 - t[..., None]) + bottom[None, None, :] * t[..., None]
    img = np.repeat(img, w, axis=1)
    a = np.full((h, w, 1), 255.0)
    return Image.fromarray(np.concatenate([img, a], 2).astype(np.uint8), "RGBA")

def layer(): return Image.new("RGBA", (W, H), (0, 0, 0, 0))

def paste(base, lay, blur=0):
    if blur: lay = lay.filter(ImageFilter.GaussianBlur(blur))
    base.alpha_composite(lay)

def add_light(base, lay, blur=0, strength=1.0):
    if blur: lay = lay.filter(ImageFilter.GaussianBlur(blur))
    a = np.asarray(base).astype(float); l = np.asarray(lay).astype(float)
    k = (l[..., 3:4] / 255.0) * strength
    a[..., :3] = np.clip(a[..., :3] + l[..., :3] * k, 0, 255)
    base.paste(Image.fromarray(a.astype(np.uint8), "RGBA"))

def rays(base, origin, color, n=9, spread=0.9, length=1.6, alpha=70, blur=26, seed=1):
    rnd = random.Random(seed); lay = layer(); d = ImageDraw.Draw(lay)
    ox, oy = origin[0] * W, origin[1] * H
    for i in range(n):
        ang = math.pi / 2 + (i / (n - 1) - 0.5) * spread + rnd.uniform(-0.05, 0.05)
        wdt = rnd.uniform(0.015, 0.05)
        L = length * H
        p1 = (ox + math.cos(ang - wdt) * L, oy + math.sin(ang - wdt) * L)
        p2 = (ox + math.cos(ang + wdt) * L, oy + math.sin(ang + wdt) * L)
        d.polygon([(ox, oy), p1, p2], fill=color + (int(alpha * rnd.uniform(0.5, 1.0)),))
    # se desvanecen con la profundidad
    m = np.linspace(1, 0, H)[:, None] ** 1.3
    arr = np.asarray(lay).astype(float); arr[..., 3] *= m
    add_light(base, Image.fromarray(arr.astype(np.uint8), "RGBA"), blur)

def particles(base, n, color, rmin, rmax, alpha=(40, 140), seed=2, blur=1):
    rnd = random.Random(seed); lay = layer(); d = ImageDraw.Draw(lay)
    for _ in range(n):
        x, y = rnd.uniform(0, W), rnd.uniform(0, H); r = rnd.uniform(rmin, rmax)
        d.ellipse([x - r, y - r, x + r, y + r], fill=color + (rnd.randint(*alpha),))
    paste(base, lay, blur)

def bubbles(base, n, area, seed=3):
    rnd = random.Random(seed); lay = layer(); d = ImageDraw.Draw(lay)
    x0, y0, x1, y1 = [v * (W if i % 2 == 0 else H) for i, v in enumerate(area)]
    for _ in range(n):
        x, y = rnd.uniform(x0, x1), rnd.uniform(y0, y1); r = rnd.uniform(4, 16) * SS / 2
        d.ellipse([x - r, y - r, x + r, y + r], outline=(220, 245, 255, 170), width=max(2, int(r / 4)))
        d.ellipse([x - r * 0.5, y - r * 0.6, x - r * 0.1, y - r * 0.2], fill=(255, 255, 255, 160))
    paste(base, lay, 0.6)

def vignette(base, strength=0.55):
    y, x = np.mgrid[0:H, 0:W]
    dx = (x / W - 0.5) * 1.6; dy = (y / H - 0.5) * 1.4
    v = np.clip(1 - (dx * dx + dy * dy) * strength, 0, 1)
    a = np.asarray(base).astype(float); a[..., :3] *= v[..., None]
    base.paste(Image.fromarray(a.astype(np.uint8), "RGBA"))

def haze(base, color, y0, y1, amount):
    """Neblina de agua: tiñe hacia el color del agua en una banda vertical."""
    a = np.asarray(base).astype(float)
    t = np.clip((np.arange(H) / H - y0) / max(1e-5, (y1 - y0)), 0, 1)[:, None, None] * amount
    a[..., :3] = a[..., :3] * (1 - t) + np.array(color, float) * t
    base.paste(Image.fromarray(a.astype(np.uint8), "RGBA"))

def P(x, y): return (x * W, y * H)

def fish(d, cx, cy, size, body, stripes=None, facing=1, alpha=255, fin=None, stripe_c=(250, 250, 250)):
    s = size * SS
    bx0, bx1 = cx - s, cx + s
    d.ellipse([bx0, cy - s * 0.55, bx1, cy + s * 0.55], fill=body + (alpha,))
    tx = cx - facing * s * 0.9
    d.polygon([(tx, cy), (tx - facing * s * 0.7, cy - s * 0.5), (tx - facing * s * 0.7, cy + s * 0.5)], fill=(fin or body) + (alpha,))
    d.polygon([(cx - facing * s * 0.2, cy - s * 0.5), (cx + facing * s * 0.25, cy - s * 0.85), (cx + facing * s * 0.45, cy - s * 0.45)], fill=(fin or body) + (alpha,))
    if stripes:
        for k in stripes:
            sx = cx + facing * s * k
            d.rectangle([sx - s * 0.09, cy - s * 0.5, sx + s * 0.09, cy + s * 0.5], fill=stripe_c + (alpha,))
    ex = cx + facing * s * 0.6
    d.ellipse([ex - s * 0.09, cy - s * 0.2, ex + s * 0.09, cy - s * 0.02], fill=(15, 15, 20, alpha))

# ---------------------------------------------------------------- zona 1
def coral_branch(d, x, y, ang, length, width, depth, color, rnd):
    if depth == 0 or length < 6: return
    x2 = x + math.cos(ang) * length; y2 = y - math.sin(ang) * length
    d.line([(x, y), (x2, y2)], fill=color, width=int(width))
    d.ellipse([x2 - width / 2, y2 - width / 2, x2 + width / 2, y2 + width / 2], fill=color)
    for s in (-1, 1):
        coral_branch(d, x2, y2, ang + s * rnd.uniform(0.25, 0.6), length * rnd.uniform(0.62, 0.8),
                     width * 0.75, depth - 1, color, rnd)

def brain_coral(d, cx, cy, rx, ry, base_c, line_c):
    d.pieslice([cx - rx, cy - ry, cx + rx, cy + ry], 180, 360, fill=base_c)
    for k in range(1, 6):
        f = k / 6
        d.arc([cx - rx * f, cy - ry * f, cx + rx * f, cy + ry * f], 185, 355, fill=line_c, width=SS * 3)

def zona_arrecifes():
    img = gradient((46, 196, 214), (6, 52, 104), power=1.1)
    rays(img, (0.62, -0.15), (255, 250, 220), n=11, spread=1.1, alpha=95, blur=22, seed=4)
    # caustica sutil arriba
    rnd = random.Random(7); lay = layer(); d = ImageDraw.Draw(lay)
    for _ in range(140):
        x, y = rnd.uniform(0, W), rnd.uniform(0, H * 0.25); r = rnd.uniform(20, 60)
        d.arc([x - r, y - r * 0.5, x + r, y + r * 0.5], rnd.uniform(0, 360), rnd.uniform(0, 360) + 140,
              fill=(255, 255, 255, 60), width=SS * 2)
    paste(img, lay, 2)
    # arrecife lejano (silueta azulada)
    lay = layer(); d = ImageDraw.Draw(lay); rnd = random.Random(11)
    pts = [(0, H)]
    for i in range(0, 41):
        x = i / 40 * W; pts.append((x, H * (0.60 + 0.05 * math.sin(i * 0.9) + rnd.uniform(-0.02, 0.02))))
    pts.append((W, H)); d.polygon(pts, fill=(30, 110, 140, 255))
    for _ in range(26):
        coral_branch(d, rnd.uniform(0, W), H * rnd.uniform(0.6, 0.66), math.pi / 2 + rnd.uniform(-0.3, 0.3),
                     rnd.uniform(40, 80), 14, 4, (35, 120, 150, 255), rnd)
    paste(img, lay, 3)
    # tortuga a media distancia
    lay = layer(); d = ImageDraw.Draw(lay); c = (40, 120, 120, 255)
    tx, ty = W * 0.70, H * 0.36
    d.ellipse([tx - 110, ty - 60, tx + 110, ty + 60], fill=c)
    d.ellipse([tx + 95, ty - 25, tx + 160, ty + 20], fill=c)
    d.polygon([(tx - 20, ty - 40), (tx - 120, ty - 150), (tx + 40, ty - 50)], fill=c)
    d.polygon([(tx - 20, ty + 40), (tx - 110, ty + 130), (tx + 40, ty + 50)], fill=c)
    d.polygon([(tx - 100, ty), (tx - 150, ty - 30), (tx - 150, ty + 25)], fill=c)
    paste(img, lay, 2)
    haze(img, (40, 160, 190), 0.0, 0.7, 0.18)
    # arena del fondo
    lay = layer(); d = ImageDraw.Draw(lay)
    pts = [(0, H)] + [(i / 30 * W, H * (0.78 + 0.025 * math.sin(i * 0.7 + 1))) for i in range(31)] + [(W, H)]
    d.polygon(pts, fill=(200, 185, 140, 255)); paste(img, lay, 2)
    sand = gradient((0, 0, 0), (0, 30, 60)); arr = np.asarray(sand).copy(); arr[..., 3] = 90
    # corales de primer plano: izquierda vivos, derecha blanqueados
    lay = layer(); d = ImageDraw.Draw(lay); rnd = random.Random(21)
    vivos = [(255, 90, 140), (200, 70, 220), (255, 140, 60), (255, 200, 70), (90, 220, 200)]
    for i in range(14):
        x = W * (i / 13); t = i / 13
        bleach = max(0.0, min(1.0, (t - 0.45) / 0.35))
        c = mix(rnd.choice(vivos), (236, 236, 228), bleach) + (255,)
        y = H * rnd.uniform(0.80, 0.86)
        if i % 3 == 0:
            brain_coral(d, x, y, rnd.uniform(60, 95), rnd.uniform(45, 70), c, mix(c[:3], (0, 0, 0), 0.25) + (255,))
        else:
            coral_branch(d, x, y, math.pi / 2 + rnd.uniform(-0.25, 0.25), rnd.uniform(70, 110), 18, 6, c, rnd)
    paste(img, lay, 0.8)
    # abanico de mar
    lay = layer(); d = ImageDraw.Draw(lay)
    fx, fy = W * 0.18, H * 0.83
    for k in range(18):
        a = math.pi * (0.2 + 0.6 * k / 17)
        d.line([(fx, fy), (fx + math.cos(a) * 230, fy - math.sin(a) * 260)], fill=(150, 60, 170, 200), width=SS * 3)
    paste(img, lay, 1)
    # peces payaso y cirujanos
    lay = layer(); d = ImageDraw.Draw(lay)
    fish(d, W * 0.30, H * 0.52, 34, (255, 120, 30), stripes=[0.25, -0.35], facing=1, fin=(240, 90, 20))
    fish(d, W * 0.45, H * 0.60, 26, (255, 120, 30), stripes=[0.25, -0.35], facing=1, fin=(240, 90, 20))
    fish(d, W * 0.20, H * 0.45, 22, (40, 90, 230), facing=1, fin=(250, 210, 40))
    fish(d, W * 0.56, H * 0.47, 20, (40, 90, 230), facing=-1, fin=(250, 210, 40))
    for k in range(18):   # cardumen lejano
        fish(d, W * (0.62 + 0.012 * k + 0.02 * math.sin(k)), H * (0.20 + 0.012 * math.cos(k * 1.7) + 0.004 * k), 7,
             (200, 235, 245), facing=-1, alpha=160)
    paste(img, lay, 0.6)
    bubbles(img, 14, (0.05, 0.1, 0.4, 0.7), seed=5)
    particles(img, 220, (230, 250, 255), 1, 3, seed=6)
    vignette(img, 0.45)
    return img

# ---------------------------------------------------------------- zona 2
def whale_shape(cx, cy, s, facing=-1):
    """Silueta de ballena azul: cabeza ancha (x=+1), cuerpo que se adelgaza hacia la cola."""
    top, bot = [], []
    for i in range(60):
        x = lerp(-1.0, 1.0, i / 59)
        k = (x + 1) / 2                       # 0 en la cola, 1 en la cabeza
        th = 0.20 * math.sqrt(max(0.0, 1 - x ** 6)) * (0.35 + 0.65 * k ** 0.6)
        top.append((x, -th * 1.05)); bot.append((x, th * 0.85))
    pts = top + bot[::-1]
    poly = [(cx + facing * x * s, cy + y * s) for x, y in pts]
    tx = cx - facing * s * 1.0
    tail = [(tx + facing * s * 0.05, cy), (tx - facing * s * 0.30, cy - s * 0.30), (tx - facing * s * 0.40, cy - s * 0.24),
            (tx - facing * s * 0.18, cy), (tx - facing * s * 0.40, cy + s * 0.24), (tx - facing * s * 0.30, cy + s * 0.30)]
    fin = [(cx + facing * s * 0.45, cy + s * 0.10), (cx + facing * s * 0.05, cy + s * 0.42), (cx + facing * s * 0.20, cy + s * 0.12)]
    grooves = [[(cx + facing * s * (0.55 + 0.03 * j), cy + s * (0.05 + 0.025 * j)), (cx + facing * s * (0.95 - 0.02 * j), cy + s * (0.02 + 0.02 * j))] for j in range(4)]
    eye = (cx + facing * s * 0.78, cy - s * 0.01)
    return poly, tail, fin, grooves, eye

def penguin(d, cx, cy, s, ang):
    ca, sa = math.cos(ang), math.sin(ang)
    def R(x, y): return (cx + (x * ca - y * sa) * s, cy + (x * sa + y * ca) * s)
    body = [R(math.cos(t) * 1.0, math.sin(t) * 0.38) for t in np.linspace(0, 2 * math.pi, 40)]
    belly = [R(math.cos(t) * 0.8, math.sin(t) * 0.22 + 0.12) for t in np.linspace(0, 2 * math.pi, 40)]
    d.polygon(body, fill=(20, 28, 40, 255)); d.polygon(belly, fill=(235, 240, 245, 255))
    d.polygon([R(0.95, -0.05), R(1.3, 0.02), R(0.95, 0.1)], fill=(240, 170, 40, 255))
    d.polygon([R(0.1, 0.2), R(-0.4, 0.75), R(-0.1, 0.25)], fill=(20, 28, 40, 255))

def zona_aguas_frias():
    img = gradient((170, 225, 240), (6, 26, 64), power=0.9)
    rays(img, (0.35, -0.2), (230, 250, 255), n=9, spread=0.9, alpha=80, blur=24, seed=9)
    # ballena fantasma al fondo
    poly, tail, fin, grooves, eye = whale_shape(W * 0.50, H * 0.60, W * 0.46, facing=-1)
    lay = layer(); d = ImageDraw.Draw(lay)
    for shp in (poly, tail, fin): d.polygon(shp, fill=(120, 220, 255, 60))
    paste(img, lay, 4)
    glow = layer(); d = ImageDraw.Draw(glow)
    for shp in (poly, tail, fin): d.line(shp + [shp[0]], fill=(170, 245, 255, 255), width=SS * 4)
    for g in grooves: d.line(g, fill=(170, 245, 255, 160), width=SS * 2)
    d.ellipse([eye[0] - 8, eye[1] - 8, eye[0] + 8, eye[1] + 8], fill=(200, 250, 255, 255))
    add_light(img, glow, 16, 0.9); add_light(img, glow, 2, 0.8)
    # plataforma de hielo vista desde abajo: masa blanca con borde inferior facetado
    lay = layer(); d = ImageDraw.Draw(lay); rnd = random.Random(31)
    edge = []
    x = W
    while x > -60:
        edge.append((x, H * (0.17 + rnd.uniform(-0.035, 0.035) + (0.05 if W * 0.35 < x < W * 0.6 else 0))))
        x -= rnd.uniform(50, 120)
    edge.append((0, H * 0.18))
    d.polygon([(0, 0), (W, 0)] + edge, fill=(222, 244, 250, 255))
    paste(img, lay, 1)
    # cara inferior azulada (luz atravesando el hielo) y grietas
    under = layer(); d = ImageDraw.Draw(under)
    for k in range(len(edge) - 1):
        (x1, y1), (x2, y2) = edge[k], edge[k + 1]
        d.polygon([(x1, y1), (x2, y2), (x2, y2 - 70), (x1, y1 - 70)], fill=(95, 190, 230, 210))
        d.line([(x1, y1 - 70), (x1 + rnd.uniform(-20, 20), y1 - 160 - rnd.uniform(0, 80))], fill=(150, 210, 235, 200), width=SS * 2)
    paste(img, under, 3)
    edge_l = layer(); d = ImageDraw.Draw(edge_l); d.line(edge, fill=(255, 255, 255, 220), width=SS * 3)
    add_light(img, edge_l, 3, 0.7)
    # bloques de hielo flotando y derritiendose
    lay = layer(); d = ImageDraw.Draw(lay)
    for (x, y, s) in [(0.12, 0.33, 60), (0.82, 0.30, 80), (0.62, 0.36, 40)]:
        cx, cy = x * W, y * H
        d.polygon([(cx - s, cy - s * 0.3), (cx + s * 0.8, cy - s * 0.4), (cx + s, cy + s * 0.2), (cx + s * 0.2, cy + s * 0.9),
                   (cx - s * 0.7, cy + s * 0.5)], fill=(215, 242, 250, 235))
    paste(img, lay, 1.2)
    # goteo de deshielo
    lay = layer(); d = ImageDraw.Draw(lay); rnd = random.Random(41)
    for _ in range(30):
        x = rnd.uniform(0.05, 0.95) * W; y = rnd.uniform(0.22, 0.42) * H; L = rnd.uniform(20, 60)
        d.line([(x, y), (x, y + L)], fill=(220, 250, 255, 150), width=SS * 2)
    paste(img, lay, 1)
    # pinguinos buceando con estela de burbujas
    lay = layer(); d = ImageDraw.Draw(lay)
    penguin(d, W * 0.30, H * 0.47, 70, 0.9)
    penguin(d, W * 0.72, H * 0.50, 52, 2.1)
    penguin(d, W * 0.50, H * 0.40, 40, 1.2)
    paste(img, lay, 0.7)
    bubbles(img, 18, (0.22, 0.30, 0.34, 0.45), seed=12)
    bubbles(img, 12, (0.66, 0.36, 0.78, 0.47), seed=13)
    # foca en primer plano (silueta)
    lay = layer(); d = ImageDraw.Draw(lay)
    sx, sy = W * 0.22, H * 0.72
    body = [(sx + math.cos(t) * 200, sy + math.sin(t) * 55) for t in np.linspace(0, 2 * math.pi, 50)]
    d.polygon(body, fill=(55, 75, 95, 255)); d.ellipse([sx + 150, sy - 55, sx + 250, sy + 35], fill=(55, 75, 95, 255))
    d.polygon([(sx - 190, sy), (sx - 270, sy - 50), (sx - 250, sy + 40)], fill=(55, 75, 95, 255))
    paste(img, lay, 1.5)
    # fondo rocoso oscuro
    lay = layer(); d = ImageDraw.Draw(lay); rnd = random.Random(51)
    pts = [(0, H)] + [(i / 24 * W, H * (0.86 + rnd.uniform(-0.03, 0.03))) for i in range(25)] + [(W, H)]
    d.polygon(pts, fill=(14, 30, 52, 255)); paste(img, lay, 2)
    particles(img, 300, (240, 250, 255), 1, 3, seed=14)
    vignette(img, 0.5)
    return img

# ---------------------------------------------------------------- zona 3
def zona_naufragios():
    img = gradient((26, 92, 92), (2, 10, 14), power=0.8)
    rays(img, (0.5, -0.3), (160, 230, 200), n=7, spread=0.6, alpha=45, blur=30, seed=17)
    # ruinas: columnas rotas a la izquierda
    lay = layer(); d = ImageDraw.Draw(lay)
    for (x, h, tilt) in [(0.08, 0.30, 0), (0.17, 0.18, 0.15), (0.26, 0.24, -0.05)]:
        cx = x * W; base = H * 0.84; top = base - h * H; w = 46
        d.polygon([(cx - w, base), (cx + w, base), (cx + w + tilt * 200, top), (cx - w + tilt * 200, top)], fill=(40, 70, 70, 255))
        for k in range(1, 4):
            xx = cx - w + k * (2 * w / 4)
            d.line([(xx, base), (xx + tilt * 200, top)], fill=(30, 55, 55, 255), width=SS * 2)
    d.rectangle([W * 0.04, H * 0.53, W * 0.13, H * 0.555], fill=(40, 70, 70, 255))
    paste(img, lay, 2.5)
    haze(img, (20, 70, 72), 0.3, 0.9, 0.25)
    # casco del barco hundido, inclinado
    lay = layer(); d = ImageDraw.Draw(lay)
    cx, cy, ang = W * 0.60, H * 0.64, -0.18
    ca, sa = math.cos(ang), math.sin(ang)
    def R(x, y): return (cx + (x * ca - y * sa), cy + (x * sa + y * ca))
    hull = [R(-420, -60), R(380, -90), R(470, -150), R(430, 40), R(330, 140), R(-360, 150), R(-450, 40)]
    d.polygon(hull, fill=(32, 46, 44, 255))
    d.polygon([R(-200, -60), R(-200, -210), R(60, -230), R(80, -80)], fill=(36, 52, 50, 255))
    d.line([R(-60, -230), R(-40, -560)], fill=(36, 52, 50, 255), width=SS * 10)
    d.line([R(-50, -460), R(130, -380)], fill=(36, 52, 50, 255), width=SS * 6)
    for k in range(7):
        px, py = R(-330 + k * 110, 20)
        d.ellipse([px - 18, py - 18, px + 18, py + 18], fill=(70, 90, 80, 255))
    d.line([R(-420, -60), R(380, -90)], fill=(80, 70, 50, 255), width=SS * 4)
    paste(img, lay, 1.2)
    rust = layer(); d = ImageDraw.Draw(rust); rnd = random.Random(61)
    for _ in range(40):
        px, py = R(rnd.uniform(-400, 400), rnd.uniform(-60, 140)); r = rnd.uniform(6, 22)
        d.ellipse([px - r, py - r, px + r, py + r], fill=(110, 70, 40, 120))
    paste(img, rust, 3)
    # sedimento
    lay = layer(); d = ImageDraw.Draw(lay); rnd = random.Random(71)
    pts = [(0, H)] + [(i / 20 * W, H * (0.84 + 0.02 * math.sin(i * 0.8) + rnd.uniform(-0.01, 0.01))) for i in range(21)] + [(W, H)]
    d.polygon(pts, fill=(18, 30, 28, 255)); paste(img, lay, 2)
    # haz de linterna del buzo
    beam = layer(); d = ImageDraw.Draw(beam)
    ox, oy = -0.05 * W, 0.18 * H
    d.polygon([(ox, oy), (W * 0.95, H * 0.45), (W * 0.70, H * 0.95)], fill=(255, 235, 180, 110))
    arr = np.asarray(beam).astype(float)
    y, x = np.mgrid[0:H, 0:W]; dist = np.sqrt((x - ox) ** 2 + (y - oy) ** 2) / W
    arr[..., 3] *= np.clip(1.3 - dist, 0, 1)
    add_light(img, Image.fromarray(arr.astype(np.uint8), "RGBA"), 40, 0.6)
    # basura: bolsas, botellas, latas
    lay = layer(); d = ImageDraw.Draw(lay); rnd = random.Random(81)
    for (x, y, s) in [(0.30, 0.30, 70), (0.78, 0.22, 55), (0.47, 0.44, 50)]:
        bx, by = x * W, y * H
        pts = [(bx + math.cos(t) * s * (1 + 0.3 * math.sin(3 * t + x * 9)), by + math.sin(t) * s * 1.3 * (1 + 0.25 * math.cos(2 * t))) for t in np.linspace(0, 2 * math.pi, 30)]
        d.polygon(pts, fill=(235, 240, 235, 90)); d.line(pts + [pts[0]], fill=(255, 255, 255, 140), width=SS * 2)
    for (x, y, a, col) in [(0.36, 0.80, 0.4, (60, 140, 90)), (0.86, 0.83, -0.9, (90, 150, 170)), (0.20, 0.62, 1.2, (60, 140, 90))]:
        bx, by = x * W, y * H; ca2, sa2 = math.cos(a), math.sin(a)
        def Q(u, v): return (bx + u * ca2 - v * sa2, by + u * sa2 + v * ca2)
        d.polygon([Q(-60, -22), Q(40, -22), Q(60, -10), Q(90, -10), Q(90, 10), Q(60, 10), Q(40, 22), Q(-60, 22)], fill=col + (200,))
    for (x, y, a) in [(0.55, 0.86, 0.3), (0.12, 0.88, -0.2), (0.70, 0.77, 1.4)]:
        bx, by = x * W, y * H; ca2, sa2 = math.cos(a), math.sin(a)
        def Q(u, v): return (bx + u * ca2 - v * sa2, by + u * sa2 + v * ca2)
        d.polygon([Q(-28, -40), Q(28, -40), Q(28, 40), Q(-28, 40)], fill=(190, 40, 40, 255))
        d.polygon([Q(-28, -12), Q(28, -12), Q(28, 8), Q(-28, 8)], fill=(220, 220, 220, 255))
    paste(img, lay, 0.8)
    # pez leon (especie invasora) cerca del casco
    lay = layer(); d = ImageDraw.Draw(lay)
    lx, ly = W * 0.80, H * 0.57
    for k in range(9):
        a = -0.9 + k * 0.22
        d.line([(lx, ly), (lx - math.cos(a) * 120, ly - math.sin(a) * 110)], fill=(230, 120, 80, 200), width=SS * 2)
    fish(d, lx, ly, 34, (240, 215, 190), stripes=[0.45, 0.2, -0.05, -0.3, -0.55], facing=-1, fin=(200, 80, 60), stripe_c=(170, 50, 35))
    paste(img, lay, 0.8)
    particles(img, 420, (200, 220, 200), 1, 3, alpha=(40, 120), seed=19)
    vignette(img, 0.65)
    return img

# ---------------------------------------------------------------- etiqueta inferior
def label(img, num, name, sub, accent):
    lay = layer(); d = ImageDraw.Draw(lay)
    y0 = int(H * 0.66)
    grad = np.zeros((H, W, 4), np.uint8)
    t = np.clip((np.arange(H) - y0) / (H - y0), 0, 1)
    grad[..., 3] = (np.clip(t * 1.6, 0, 1) * 225)[:, None].astype(np.uint8)
    grad[..., :3] = (2, 12, 22)
    img.alpha_composite(Image.fromarray(grad, "RGBA"))
    d = ImageDraw.Draw(img)
    x = 52 * SS
    d.text((x, H * 0.735), f"ZONA {num}", font=font("Bold", 30 * SS), fill=accent + (255,))
    d.line([(x, H * 0.735 + 46 * SS), (x + 70 * SS, H * 0.735 + 46 * SS)], fill=accent + (255,), width=4 * SS)
    d.text((x, H * 0.80), name, font=font("Black", 66 * SS), fill=(255, 255, 255, 255))
    d.text((x, H * 0.905), sub, font=font("Regular", 27 * SS), fill=(205, 225, 235, 255))

def finish(img, path):
    img.resize((CW, CH), Image.LANCZOS).save(path)

z1 = zona_arrecifes();   label(z1, 1, "ARRECIFES", "Estrés térmico y blanqueamiento", (255, 150, 100));  finish(z1, f"{OUT}/zona1_arrecifes.png")
z2 = zona_aguas_frias(); label(z2, 2, "AGUAS FRÍAS", "Deshielo y cadenas tróficas", (140, 230, 255));     finish(z2, f"{OUT}/zona2_aguas_frias.png")
z3 = zona_naufragios();  label(z3, 3, "NAUFRAGIOS", "Huella humana y contaminación", (255, 200, 110));   finish(z3, f"{OUT}/zona3_naufragios.png")

# ---------------------------------------------------------------- textos del menu
def text_img(path, w, h, draw_fn, glow=True):
    big = Image.new("RGBA", (w * SS, h * SS), (0, 0, 0, 0)); draw_fn(ImageDraw.Draw(big), big)
    if glow:
        sh = big.copy().filter(ImageFilter.GaussianBlur(10 * SS))
        a = np.asarray(sh).astype(float); a[..., :3] = (0, 20, 40); a[..., 3] *= 0.9
        base = Image.fromarray(a.astype(np.uint8), "RGBA"); base.alpha_composite(big); big = base
    big.resize((w, h), Image.LANCZOS).save(path)

def centered(d, y, txt, f, fill, w, spacing=0):
    if spacing:
        total = sum(d.textlength(c, font=f) for c in txt) + spacing * (len(txt) - 1)
        x = (w - total) / 2
        for c in txt:
            d.text((x, y), c, font=f, fill=fill); x += d.textlength(c, font=f) + spacing
    else:
        tw = d.textlength(txt, font=f); d.text(((w - tw) / 2, y), txt, font=f, fill=fill)

def titulo(d, big):
    w = 1600 * SS
    centered(d, 10 * SS, "ACUARIO VIRTUAL INTERACTIVO", font("Bold", 34 * SS), (120, 225, 255, 255), w, spacing=10 * SS)
    centered(d, 62 * SS, "ELIGE TU INMERSIÓN", font("Black", 130 * SS), (255, 255, 255, 255), w, spacing=6 * SS)
    centered(d, 238 * SS, "Selecciona la zona a la que quieres sumergirte", font("Regular", 44 * SS), (200, 230, 240, 255), w)
text_img(f"{OUT}/titulo.png", 1600, 320, titulo)

def pie(d, big):
    f = font("Medium", 34 * SS); fk = font("Bold", 30 * SS); x = 40 * SS; y = 22 * SS
    def key(txt):
        nonlocal x
        tw = d.textlength(txt, font=fk); pad = 18 * SS
        d.rounded_rectangle([x, y - 4 * SS, x + tw + 2 * pad, y + 50 * SS], radius=10 * SS, outline=(160, 225, 245, 255), width=3 * SS)
        d.text((x + pad, y + 2 * SS), txt, font=fk, fill=(255, 255, 255, 255)); x += tw + 2 * pad + 12 * SS
    def txt(t, gap=40):
        nonlocal x
        d.text((x, y + 2 * SS), t, font=f, fill=(200, 230, 240, 255)); x += d.textlength(t, font=f) + gap * SS
    key("1"); key("2"); key("3"); txt("o clic para sumergirte")
    key("←"); key("→"); key("Enter"); txt("navegar")
    key("Esc"); txt("salir", 0)
    # centrar horizontalmente lo dibujado
    bbox = big.getbbox(); crop = big.crop(bbox); big.paste((0, 0, 0, 0), [0, 0, big.width, big.height])
    big.alpha_composite(crop, ((big.width - crop.width) // 2, (big.height - crop.height) // 2))
text_img(f"{OUT}/pie.png", 1600, 90, pie)

def badge(path, num, name, accent):
    def fn(d, big):
        w, h = big.size
        d.rounded_rectangle([0, 0, w - 1, h - 1], radius=24 * SS, fill=(4, 18, 30, 190), outline=accent + (200,), width=3 * SS)
        d.text((28 * SS, 16 * SS), f"ZONA {num}", font=font("Bold", 26 * SS), fill=accent + (255,))
        d.text((28 * SS, 50 * SS), name, font=font("Black", 44 * SS), fill=(255, 255, 255, 255))
        d.text((w - 300 * SS, 64 * SS), "Esc: volver al menú", font=font("Regular", 24 * SS), fill=(190, 215, 225, 255))
    text_img(path, 720, 118, fn, glow=False)
badge(f"{OUT}/hud_zona1.png", 1, "ARRECIFES", (255, 150, 100))
badge(f"{OUT}/hud_zona2.png", 2, "AGUAS FRÍAS", (140, 230, 255))
badge(f"{OUT}/hud_zona3.png", 3, "NAUFRAGIOS", (255, 200, 110))
print("ok")
