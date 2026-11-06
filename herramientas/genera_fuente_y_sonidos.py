# Genera el atlas de la fuente del HUD y los sonidos submarinos (procedurales, sin derechos de terceros).
#   python herramientas/genera_fuente_y_sonidos.py src/bin
import sys, os, wave
import numpy as np
from PIL import Image, ImageDraw, ImageFont
BIN = sys.argv[1] if len(sys.argv) > 1 else 'out'

# ---------------------------------------------------------------- atlas de fuente
FD = '/usr/share/fonts/opentype/noto/'
CHARS = [chr(c) for c in range(32, 127)] + list('áéíóúÁÉÍÓÚñÑüÜ¿¡°·–—…↑↓←→✓')
def atlas(peso, nombre, base=56):
    f = ImageFont.truetype(FD + f'NotoSansCJK-{peso}.ttc', base, index=0)
    asc, desc = f.getmetrics()
    W, x, y, fila = 1024, 2, 2, 0
    img = Image.new('RGBA', (W, 1024), (255, 255, 255, 0)); d = ImageDraw.Draw(img)
    lineas = [f'{base} {asc} {desc}']
    for ch in CHARS:
        l, t, r, b = f.getbbox(ch) if ch != ' ' else (0, 0, 1, 1)
        w, h = r - l + 4, b - t + 4
        if x + w > W: x = 2; y += fila + 2; fila = 0
        if ch != ' ': d.text((x - l + 2, y - t + 2), ch, font=f, fill=(255, 255, 255, 255))
        lineas.append(f'{ord(ch)} {x} {y} {w} {h} {l - 2} {t - 2} {f.getlength(ch):.2f}')
        x += w + 2; fila = max(fila, h)
    alto = y + fila + 4; img = img.crop((0, 0, W, alto))
    os.makedirs(f'{BIN}/textures/menu', exist_ok=True)
    img.save(f'{BIN}/textures/menu/{nombre}.png')
    open(f'{BIN}/textures/menu/{nombre}.txt', 'w').write('\n'.join(lineas) + '\n')
    print('atlas', nombre, img.size)
atlas('Bold', 'fuente_negrita'); atlas('Regular', 'fuente_normal')

# ---------------------------------------------------------------- sonidos
SR = 22050
def guardar(nombre, x):
    x = np.clip(x / (np.max(np.abs(x)) + 1e-9) * 0.85, -1, 1)
    os.makedirs(f'{BIN}/sound', exist_ok=True)
    with wave.open(f'{BIN}/sound/{nombre}.wav', 'wb') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR); w.writeframes((x * 32767).astype(np.int16).tobytes())
    print('sonido', nombre, round(len(x) / SR, 1), 's')

def pasabajos(x, a):
    y = np.zeros_like(x); acc = 0.0
    for i in range(len(x)): acc += a * (x[i] - acc); y[i] = acc
    return y

def lazo(x, cruce=1.5):
    n = int(cruce * SR); f = np.linspace(0, 1, n)
    x[:n] = x[:n] * f + x[-n:] * (1 - f)
    return x[:-n]

def burbuja(f0, dur, sube=1.8):
    t = np.arange(int(dur * SR)) / SR
    return np.sin(2 * np.pi * f0 * (1 + (sube - 1) * t / dur) * t) * np.exp(-t * 18)

g = np.random.default_rng(7)
def ambiente(nombre, dur, corte, chasquidos, burbujas, crujidos=0, tono=0):
    n = int(dur * SR)
    ruido = np.cumsum(g.normal(0, 1, n)); ruido -= pasabajos(ruido, 0.002)       # ruido marron (oleaje lejano)
    base = pasabajos(ruido, corte) * (0.6 + 0.4 * np.sin(np.linspace(0, 2 * np.pi * 3, n)))
    x = base / (np.max(np.abs(base)) + 1e-9) * 0.6
    for _ in range(chasquidos):                                                 # camarones pistola del arrecife
        i = g.integers(0, n - 200); k = np.arange(120)
        x[i:i + 120] += g.normal(0, 1, 120) * np.exp(-k / 12) * g.uniform(0.05, 0.22)
    for _ in range(burbujas):
        i = g.integers(0, n - SR); b = burbuja(g.uniform(400, 1100), 0.12) * g.uniform(0.08, 0.2)
        x[i:i + len(b)] += b
    for _ in range(crujidos):                                                   # hielo / metal que cruje
        i = g.integers(0, n - 2 * SR); d = g.uniform(0.6, 1.4); t = np.arange(int(d * SR)) / SR
        f = g.uniform(70, 160) * (1 + 0.3 * np.sin(t * g.uniform(3, 8)))
        c = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.sin(np.pi * t / d) * (0.5 + 0.5 * np.sign(np.sin(t * 40))) * 0.25
        x[i:i + len(c)] += c
    if tono:
        t = np.arange(n) / SR; x += np.sin(2 * np.pi * tono * t) * 0.05 * (0.5 + 0.5 * np.sin(t * 0.3))
    guardar(nombre, lazo(x))

ambiente('ambiente_arrecife', 26, 0.02, 900, 60)
ambiente('ambiente_frio', 26, 0.008, 40, 25, crujidos=9)
ambiente('ambiente_naufragio', 26, 0.004, 60, 30, crujidos=7, tono=55)

# exhalacion del regulador: siseo + burbujas que suben
t = np.arange(int(1.8 * SR)) / SR
siseo = pasabajos(g.normal(0, 1, len(t)), 0.25) * np.clip(np.sin(np.pi * t / 0.7), 0, 1) * 0.35
x = siseo.copy()
for k in range(26):
    i = int(g.uniform(0.15, 1.5) * SR); b = burbuja(g.uniform(300, 900), g.uniform(0.06, 0.14)) * g.uniform(0.3, 0.8)
    x[i:i + len(b)] += b[:len(x) - i]
guardar('burbujas', x)

# alerta: ping de sonar suave con eco
t = np.arange(int(2.2 * SR)) / SR
p = lambda d: np.where(t >= d, np.sin(2 * np.pi * 1046 * (t - d)) * np.exp(-(t - d) * 5), 0)
guardar('alerta', p(0) + 0.45 * p(0.38) + 0.2 * p(0.76))

# descubrimiento: campanita de tres notas
t = np.arange(int(1.4 * SR)) / SR; x = np.zeros_like(t)
for i, f in enumerate((1047, 1319, 1568)):
    d = i * 0.09; e = np.where(t >= d, np.exp(-(t - d) * 4), 0)
    x += (np.sin(2 * np.pi * f * (t - d)) + 0.3 * np.sin(2 * np.pi * 2 * f * (t - d))) * e
guardar('descubrir', x)

# recogida (para Naufragios): golpe corto y brillante
t = np.arange(int(0.5 * SR)) / SR
guardar('recoger', np.sin(2 * np.pi * (660 + 400 * t) * t) * np.exp(-t * 9) + g.normal(0, 0.15, len(t)) * np.exp(-t * 40))

# lluvia de monedas del cofre: muchos tintineos metalicos (parciales inarmonicos) que se van espaciando
t = np.arange(int(3.2 * SR)) / SR; x = np.zeros_like(t)
for k in range(140):
    d = (g.random() ** 1.8) * 2.8; i = int(d * SR)
    f0 = g.uniform(2200, 3600); amp = g.uniform(0.15, 0.5) * (1.0 - d / 3.4)
    n = int(0.25 * SR); tt = np.arange(n) / SR
    clin = sum(np.sin(2 * np.pi * f0 * r * tt) * np.exp(-tt * (30 + 12 * j)) for j, r in enumerate((1.0, 1.53, 2.31, 3.07)))
    x[i:i + n] += clin[:len(x) - i] * amp
guardar('monedas', x)
