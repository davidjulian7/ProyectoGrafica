# Modelos de la Zona 3 (Naufragios) que el equipo no tenia, y los letreros informativos.
#   blender --background --python herramientas/exporta_zona3_extras.py
import os, sys, math, random, shutil
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bpy, bmesh
import numpy as np
from mathutils import Vector, Matrix
from PIL import Image, ImageDraw, ImageFont
import exporta_zona1_extras as ext
from exporta_zonas import SALIDA, limpiar_escena, guardar_jpg, material_con_textura, uv_caja, uv_lateral, exportar, nuevo_objeto, suavizar, ruido, tex_ruido, tex_oxido, normalizar
from exporta_zona1_extras import skin, combinar, uv_constante, esfera, cuna, lienzo

CARPETA = os.path.join(SALIDA, 'naufragios'); ext.CARPETA = CARPETA
terminar = ext.terminar
FD = '/usr/share/fonts/opentype/noto/'
def fuente(peso, t): return ImageFont.truetype(FD + f'NotoSansCJK-{peso}.ttc', t, index=0)

def uv_cilindro(ob, z0=None, z1=None):
    bm = bmesh.new(); bm.from_mesh(ob.data); uv = bm.loops.layers.uv.verify()
    zs = [v.co.z for v in bm.verts]; z0 = min(zs) if z0 is None else z0; z1 = max(zs) if z1 is None else z1
    for f in bm.faces:
        us = []
        for l in f.loops:
            a = (math.atan2(l.vert.co.y, l.vert.co.x) / (2 * math.pi)) % 1.0; us.append(a)
        if max(us) - min(us) > 0.5: us = [u + 1.0 if u < 0.5 else u for u in us]
        for l, u in zip(f.loops, us): l[uv].uv = (u, (l.vert.co.z - z0) / (z1 - z0 + 1e-6))
    bm.to_mesh(ob.data); bm.free()

def caja(nombre, tam, centro):
    bm = bmesh.new(); bmesh.ops.create_cube(bm, size=1.0)
    for v in bm.verts: v.co = Vector((v.co.x * tam[0], v.co.y * tam[1], v.co.z * tam[2])) + Vector(centro)
    return nuevo_objeto(nombre, bm)

def cilindro(nombre, r, h, centro, seg=20, r2=None):
    bm = bmesh.new(); bmesh.ops.create_cone(bm, cap_ends=True, segments=seg, radius1=r, radius2=r if r2 is None else r2, depth=h)
    bmesh.ops.translate(bm, vec=Vector(centro), verts=bm.verts)
    return nuevo_objeto(nombre, bm)

def rotar(ob, eje, grados):
    ob.data.transform(Matrix.Rotation(math.radians(grados), 4, eje))

def mover(ob, v): ob.data.transform(Matrix.Translation(Vector(v)))

# ---------------------------------------------------------------- pez nativo (roncador)
def roncador():
    tk = [0, 0.1, 0.35, 0.6, 0.8, 0.93, 1.0]
    alto = [0.04, 0.08, 0.26, 0.30, 0.25, 0.15, 0.03]
    ancho = [0.02, 0.03, 0.09, 0.11, 0.09, 0.06, 0.02]
    bm = bmesh.new(); an = []; NR, NS = 26, 14
    for i in range(NR):
        t = i / (NR - 1); x = t * 2 - 1
        a, b = np.interp(t, tk, alto), np.interp(t, tk, ancho)
        an.append([bm.verts.new((x, b * math.cos(th), a * math.sin(th))) for th in np.linspace(0, 2 * math.pi, NS, endpoint=False)])
    for i in range(NR - 1):
        for s in range(NS): bm.faces.new((an[i][s], an[i][(s + 1) % NS], an[i + 1][(s + 1) % NS], an[i + 1][s]))
    for idx, x in ((0, -1.005), (NR - 1, 1.005)):
        c = bm.verts.new((x, 0, 0))
        for s in range(NS):
            q = (an[idx][s], an[idx][(s + 1) % NS], c); bm.faces.new(q if x > 0 else q[::-1])
    cuerpo = nuevo_objeto('c', bm)
    aletas = [cuna('c1', (-0.95, 0, 0.03), (-0.95, 0, 0.0), (-1.35, 0, 0.32), 0.012), cuna('c2', (-0.95, 0, 0.0), (-0.95, 0, -0.03), (-1.35, 0, -0.32), 0.012),
              cuna('d', (0.35, 0, 0.27), (-0.55, 0, 0.22), (-0.15, 0, 0.47), 0.012), cuna('an', (-0.25, 0, -0.2), (-0.6, 0, -0.15), (-0.6, 0, -0.35), 0.012)]
    aletas += [cuna(f'p{s}', (0.45, s * 0.1, -0.05), (0.35, s * 0.1, -0.08), (0.15, s * 0.26, -0.12), 0.01, 'z') for s in (-1, 1)]
    ojos = [esfera(f'o{s}', 0.05, (0.72, s * 0.075, 0.07), (1, 1, 1), 2) for s in (-1, 1)]
    for o in ojos: uv_constante(o, 0.995, 0.995)
    resto = combinar('r', [cuerpo] + aletas); uv_lateral(resto)
    ob = combinar('roncador', [resto] + ojos); suavizar(ob)
    W = H = 512; v = np.linspace(1, 0, H)[:, None, None]
    arr = np.array((245, 205, 70)) * np.clip((v - 0.25) / 0.3, 0, 1) + np.array((240, 235, 200)) * (1 - np.clip((v - 0.25) / 0.3, 0, 1))
    arr = np.repeat(arr, W, 1)
    img = Image.fromarray(np.clip(arr + np.random.default_rng(1).normal(0, 4, arr.shape), 0, 255).astype(np.uint8)); d = ImageDraw.Draw(img)
    for k in range(7):                                             # rayas azules del roncador
        y0 = H * (0.18 + k * 0.085)
        d.line([(x, y0 + 6 * math.sin(x / 40 + k)) for x in range(0, W, 8)], fill=(80, 140, 210), width=7)
    d.rectangle([W - 6, 0, W, 6], fill=(15, 15, 15))
    terminar(ob, img, 'roncador.jpg', 0.32)

# ---------------------------------------------------------------- dron submarino
def dron():
    cuerpo = caja('cu', (0.9, 0.62, 0.42), (0, 0, 0))
    bm = bmesh.new(); bm.from_mesh(cuerpo.data)
    bmesh.ops.bevel(bm, geom=bm.edges[:], offset=0.06, segments=3, affect='EDGES'); bm.to_mesh(cuerpo.data); bm.free()
    piezas = [cuerpo]
    for sx in (-1, 1):
        for sy in (-1, 1):
            t = cilindro('t', 0.09, 0.32, (0, 0, 0), 16); rotar(t, 'Y', 90); mover(t, (sx * 0.34, sy * 0.42, -0.1)); piezas.append(t)
    vert = cilindro('v', 0.08, 0.2, (0, 0, 0.3), 16); piezas.append(vert)
    piezas.append(caja('pat', (0.95, 0.05, 0.05), (0, 0.3, -0.26))); piezas.append(caja('pat2', (0.95, 0.05, 0.05), (0, -0.3, -0.26)))
    domo = esfera('domo', 0.16, (0.45, 0, 0.0), (1, 1, 1), 3); uv_constante(domo, 0.005, 0.995)
    luces = [esfera(f'l{s}', 0.05, (0.47, s * 0.22, 0.12), (1, 1, 1), 2) for s in (-1, 1)]
    for l in luces: uv_constante(l, 0.995, 0.995)
    resto = combinar('dr', piezas); uv_caja(resto, 2.0)
    ob = combinar('dron', [resto, domo] + luces); suavizar(ob)
    img = Image.new('RGB', (512, 512), (240, 190, 30)); d = ImageDraw.Draw(img)
    for k in range(0, 512, 64): d.polygon([(k, 0), (k + 32, 0), (k - 32, 512), (k - 64, 512)], fill=(30, 30, 30)) if k % 128 == 0 else None
    d.rectangle([0, 200, 512, 312], fill=(240, 190, 30)); d.text((60, 225), 'ROV-01', font=fuente('Black', 60), fill=(25, 25, 25))
    d.rectangle([0, 0, 6, 6], fill=(20, 40, 60)); d.rectangle([506, 0, 512, 6], fill=(255, 255, 235))
    terminar(ob, img, 'dron.jpg', 1.0)

# ---------------------------------------------------------------- ancla
def ancla():
    piezas = [skin('cana', [(0, 0, 0.2), (0, 0, 1.0), (0, 0, 1.9)], [(0, 1), (1, 2)], [0.07, 0.065, 0.06], 1)]
    pts = [(0, 0, 0.22)]; edges = []
    for s in (-1, 1):
        base = len(pts) - 1 if s == -1 else 0
        prev = 0
        for k in range(1, 7):
            a = k / 6 * math.pi * 0.42
            pts.append((s * math.sin(a) * 0.75, 0, 0.22 + 0.75 - math.cos(a) * 0.75)); edges.append((prev, len(pts) - 1)); prev = len(pts) - 1
    piezas.append(skin('brazos', pts, edges, [0.07] + [0.065 - k * 0.004 for k in range(12)], 1))
    for s in (-1, 1):
        a = math.pi * 0.42; tip = Vector((s * math.sin(a) * 0.75, 0, 0.97 - math.cos(a) * 0.75))
        piezas.append(cuna(f'u{s}', tip + Vector((-s * 0.2, 0, -0.1)), tip + Vector((s * 0.05, 0, -0.25)), tip + Vector((s * 0.12, 0, 0.18)), 0.04))
    piezas.append(skin('cepo', [(0, -0.55, 1.75), (0, 0, 1.75), (0, 0.55, 1.75)], [(0, 1), (1, 2)], [0.045, 0.05, 0.045], 1))
    aro = [(0, 0.0, 1.9 + 0.16 + 0.16 * math.cos(a)) if False else (0.16 * math.sin(a), 0, 2.06 - 0.16 * math.cos(a)) for a in np.linspace(0, 2 * math.pi, 13)[:-1]]
    piezas.append(skin('aro', aro, [(i, (i + 1) % 12) for i in range(12)], [0.03] * 12, 1))
    ob = combinar('ancla', piezas); uv_caja(ob, 1.5); suavizar(ob)
    terminar(ob, tex_oxido(23), 'ancla.jpg', 2.2, base=True)

# ---------------------------------------------------------------- ruinas
def tex_piedra(seed):
    img = tex_ruido((120, 118, 100), (175, 170, 150), seed, 10, 9)
    musgo = ruido(512, 512, 6, seed + 3)[..., None]; m = np.clip((musgo - 0.55) * 4, 0, 1)
    img = img * (1 - m * 0.6) + np.array((60, 95, 60)) * m * 0.6
    for k in range(0, 512, 64): img[k:k + 3, :] *= 0.55
    for j in range(8):
        off = 32 if j % 2 else 0
        for k in range(off, 512, 96): img[j * 64:(j + 1) * 64, k:k + 3] *= 0.55
    return np.clip(img, 0, 255)

def muro_arcos():
    piezas = []; g = random.Random(4)
    for i in range(4):                                             # pilares
        x = -4.5 + i * 3.0
        piezas.append(caja(f'p{i}', (0.9, 1.2, 4.0), (x, 0, 2.0)))
    for i in range(3):                                             # arcos de dovelas
        cx = -3.0 + i * 3.0
        for k in range(9):
            if i == 2 and k in (6, 7, 8): continue                  # arco roto
            a = math.pi * (k + 0.5) / 9
            d = caja('d', (0.42, 1.1, 0.38), (0, 0, 0))
            rotar(d, 'Y', -math.degrees(a) + 90); mover(d, (cx + math.cos(a) * 1.25, 0, 4.0 + math.sin(a) * 1.25)); piezas.append(d)
    for i in range(6):                                             # cornisa con huecos
        if i in (4, 5) and g.random() < 0.9: continue
        piezas.append(caja(f'c{i}', (1.6, 1.25, 0.55), (-4.0 + i * 1.6, 0, 5.6)))
    ob = combinar('ruinas_muro', piezas); uv_caja(ob, 0.5)
    terminar(ob, tex_piedra(5), 'piedra_ruinas.jpg')

def torre_rota():
    bm = bmesh.new(); seg = 28; filas = []; g = random.Random(8)
    for i in range(11):
        z = i * 0.6; fila = []
        for s in range(seg):
            a = s / seg * 2 * math.pi; zz = z
            if i == 10: zz = z - (0 if (s // 3) % 2 == 0 else 0.6) - (2.0 if 6 < s < 15 else 0) * g.uniform(0.6, 1.0)
            fila.append(bm.verts.new((math.cos(a) * 2.0, math.sin(a) * 2.0, zz)))
        filas.append(fila)
    dentro = []
    for i in range(11):
        dentro.append([bm.verts.new((v.co.x * 0.78, v.co.y * 0.78, v.co.z)) for v in filas[i]])
    for i in range(10):
        for s in range(seg):
            bm.faces.new((filas[i][s], filas[i][(s + 1) % seg], filas[i + 1][(s + 1) % seg], filas[i + 1][s]))
            bm.faces.new((dentro[i + 1][s], dentro[i + 1][(s + 1) % seg], dentro[i][(s + 1) % seg], dentro[i][s]))
    for s in range(seg): bm.faces.new((filas[-1][s], filas[-1][(s + 1) % seg], dentro[-1][(s + 1) % seg], dentro[-1][s]))
    ob = nuevo_objeto('ruinas_torre', bm); uv_cilindro(ob)
    bm = bmesh.new(); bm.from_mesh(ob.data); uv = bm.loops.layers.uv.verify()
    for f in bm.faces:
        for l in f.loops: l[uv].uv = (l[uv].uv[0] * 6, l[uv].uv[1] * 3)
    bm.to_mesh(ob.data); bm.free()
    terminar(ob, tex_piedra(6), 'piedra_ruinas.jpg')

# ---------------------------------------------------------------- cofre y monedas
def tex_madera(seed, bandas=True):
    y, x = np.mgrid[0:512, 0:512]
    n = ruido(512, 512, 3, seed)
    veta = 0.5 + 0.5 * np.sin(y * 0.09 + n * 12)
    img = np.array((95, 58, 30))[None, None] * (1 - veta[..., None] * 0.45) + np.array((140, 92, 48))[None, None] * veta[..., None] * 0.45
    for k in range(0, 512, 85): img[k:k + 4, :] *= 0.45
    if bandas:
        for k in (60, 450): img[:, k:k + 26] = np.array((110, 100, 70)); img[:, k + 8:k + 18] = np.array((150, 135, 85))
    return np.clip(img + np.random.default_rng(seed).normal(0, 5, img.shape), 0, 255)

def cofre():
    base = caja('cofre', (1.0, 0.6, 0.5), (0, 0, 0.25)); uv_caja(base, 1.2)
    terminar(base, tex_madera(3), 'madera_cofre.jpg')
    limpiar_escena()
    bm = bmesh.new(); seg = 16; filas = []
    for s in range(seg + 1):
        a = math.pi * s / seg
        filas.append([bm.verts.new((x, 0.3 * math.cos(a), 0.3 * math.sin(a) * 0.85)) for x in (-0.5, 0.5)])
    for s in range(seg): bm.faces.new((filas[s][0], filas[s][1], filas[s + 1][1], filas[s + 1][0]))
    bm.faces.new([filas[s][0] for s in range(seg + 1)]); bm.faces.new([filas[s][1] for s in range(seg, -1, -1)])
    bm.faces.new((filas[0][0], filas[0][1], filas[seg][1], filas[seg][0]))
    tapa = nuevo_objeto('cofre_tapa', bm)
    mover(tapa, (0, -0.3, 0))      # bisagra (borde trasero) en el origen
    uv_caja(tapa, 1.2)
    terminar(tapa, tex_madera(3), 'madera_cofre.jpg')
    limpiar_escena()
    m = cilindro('moneda', 0.035, 0.008, (0, 0, 0.004), 20); uv_caja(m, 10.0)
    img = tex_ruido((200, 150, 30), (255, 215, 80), 9, 12, 8)
    terminar(m, img, 'oro.jpg')

# ---------------------------------------------------------------- residuos
def botella():
    perfil = [(0.0, 0.004), (0.032, 0.0), (0.035, 0.02), (0.035, 0.17), (0.033, 0.19), (0.02, 0.24), (0.013, 0.265), (0.013, 0.285), (0.015, 0.287), (0.015, 0.31), (0.0, 0.312)]
    bm = bmesh.new(); seg = 20; filas = []
    for r, z in perfil:
        filas.append([bm.verts.new((math.cos(s / seg * 6.283) * r, math.sin(s / seg * 6.283) * r, z)) for s in range(seg)])
    for i in range(len(filas) - 1):
        for s in range(seg): bm.faces.new((filas[i][s], filas[i][(s + 1) % seg], filas[i + 1][(s + 1) % seg], filas[i + 1][s]))
    ob = nuevo_objeto('botella', bm); uv_cilindro(ob); suavizar(ob)
    img = Image.new('RGB', (256, 256), (190, 225, 240)); d = ImageDraw.Draw(img)
    d.rectangle([0, 256 - int(0.17 / 0.31 * 256), 256, 256 - int(0.09 / 0.31 * 256)], fill=(40, 110, 190))
    d.rectangle([0, 0, 256, int(0.03 / 0.31 * 256)], fill=(30, 80, 160))
    terminar(ob, img, 'botella.jpg', 0.32, base=True)

def red_fantasma():
    g = random.Random(13); nx, nz = 16, 11; W, H = 5.0, 3.4
    pts = []; idx = {}
    for i in range(nx + 1):
        for j in range(nz + 1):
            x = -W / 2 + W * i / nx; z = H * j / nz
            y = 0.35 * math.sin(i * 0.7) * (1 - j / nz) + 0.25 * math.sin(j * 0.9 + i * 0.3)
            z -= 0.5 * math.sin(math.pi * i / nx) * (j / nz) ** 2          # cuelga del centro
            idx[(i, j)] = len(pts); pts.append((x, y, z))
    edges = []
    for i in range(nx + 1):
        for j in range(nz + 1):
            if i < nx and g.random() > 0.07: edges.append((idx[(i, j)], idx[(i + 1, j)]))
            if j < nz and g.random() > 0.07: edges.append((idx[(i, j)], idx[(i, j + 1)]))
    red = skin('red', pts, edges, [0.012] * len(pts), 0)
    flotes = [esfera(f'f{i}', 0.07, (pts[idx[(i, nz)]][0], pts[idx[(i, nz)]][1], pts[idx[(i, nz)]][2] + 0.03), (1.3, 1, 1), 2) for i in range(0, nx + 1, 3)]
    for f in flotes: uv_constante(f, 0.995, 0.995)
    uv_caja(red, 3.0)
    ob = combinar('red_fantasma', [red] + flotes)
    img = Image.fromarray(tex_ruido((45, 70, 55), (85, 105, 80), 15, 10, 10).astype(np.uint8)); ImageDraw.Draw(img).rectangle([500, 0, 512, 12], fill=(230, 110, 30))
    terminar(ob, img, 'red.jpg', base=True)

def barril():
    bm = bmesh.new(); seg = 28; filas = []; perfil = [0.0, 0.05, 0.08, 0.30, 0.33, 0.57, 0.60, 0.82, 0.85, 0.9]
    for z in perfil:
        r = 0.30 + (0.012 if z in (0.08, 0.33, 0.57, 0.82) else 0.0)
        fila = []
        for s in range(seg):
            a = s / seg * 2 * math.pi; rr = r * (1 - (0.12 * math.exp(-((a - 1.0) ** 2) / 0.1) if 0.35 < z < 0.6 else 0))   # abolladura
            fila.append(bm.verts.new((math.cos(a) * rr, math.sin(a) * rr, z)))
        filas.append(fila)
    for i in range(len(filas) - 1):
        for s in range(seg): bm.faces.new((filas[i][s], filas[i][(s + 1) % seg], filas[i + 1][(s + 1) % seg], filas[i + 1][s]))
    bm.faces.new(list(reversed(filas[0]))); bm.faces.new(filas[-1])
    ob = nuevo_objeto('barril', bm); uv_cilindro(ob)
    img = Image.fromarray(tex_oxido(31).astype(np.uint8)).resize((512, 512))
    arr = np.asarray(img).astype(float); arr = arr * 0.35 + np.array((215, 180, 35)) * 0.65
    img = Image.fromarray(np.clip(arr, 0, 255).astype(np.uint8)); d = ImageDraw.Draw(img)
    for k in range(-512, 512, 48): d.polygon([(k, 40), (k + 24, 40), (k + 64, 0), (k + 40, 0)], fill=(25, 25, 20))
    cx, cy = 128, 260
    d.polygon([(cx, cy - 70), (cx + 70, cy), (cx, cy + 70), (cx - 70, cy)], fill=(245, 245, 240), outline=(200, 30, 30))
    d.polygon([(cx, cy - 62), (cx + 62, cy), (cx, cy + 62), (cx - 62, cy)], outline=(200, 30, 30), width=8)
    d.text((cx - 13, cy - 45), '!', font=fuente('Black', 80), fill=(20, 20, 20))
    d.text((cx + 90, cy - 30), 'TÓXICO', font=fuente('Black', 50), fill=(25, 25, 20))
    terminar(ob, img, 'barril.jpg', base=True)

def timon():
    piezas = []
    aro = [(0.45 * math.cos(a), 0, 0.45 * math.sin(a)) for a in np.linspace(0, 2 * math.pi, 25)[:-1]]
    piezas.append(skin('aro', aro, [(i, (i + 1) % 24) for i in range(24)], [0.035] * 24, 1))
    eje = cilindro('eje', 0.09, 0.14, (0, 0, 0), 16); rotar(eje, 'X', 90); piezas.append(eje)
    for k in range(8):
        a = k / 8 * 2 * math.pi
        piezas.append(skin(f'r{k}', [(0, 0, 0), (0.45 * math.cos(a), 0, 0.45 * math.sin(a)), (0.62 * math.cos(a), 0, 0.62 * math.sin(a)), (0.68 * math.cos(a), 0, 0.68 * math.sin(a))],
                           [(0, 1), (1, 2), (2, 3)], [0.025, 0.022, 0.03, 0.022], 1))
    ob = combinar('timon', piezas); uv_caja(ob, 2.0); suavizar(ob)
    terminar(ob, tex_madera(7, False), 'madera_timon.jpg')

def bota():
    pie = caja('pie', (0.30, 0.11, 0.08), (0.05, 0, 0.04))
    bm = bmesh.new(); bm.from_mesh(pie.data); bmesh.ops.bevel(bm, geom=bm.edges[:], offset=0.03, segments=3, affect='EDGES'); bm.to_mesh(pie.data); bm.free()
    cana = cilindro('c', 0.065, 0.32, (-0.06, 0, 0.22), 18, 0.07)
    ob = combinar('bota', [pie, cana]); uv_caja(ob, 4.0); suavizar(ob)
    img = tex_ruido((22, 40, 28), (45, 70, 45), 41, 6, 6)
    lodo = ruido(512, 512, 5, 42)[..., None]; m = np.clip((lodo - 0.6) * 4, 0, 1)
    terminar(ob, np.clip(img * (1 - m) + np.array((90, 75, 50)) * m, 0, 255), 'bota.jpg', 0.38, base=True)

def pila():
    cuerpo = cilindro('c', 0.007, 0.046, (0, 0, 0.023), 16)
    polo = cilindro('p', 0.003, 0.003, (0, 0, 0.0475), 10)
    ob = combinar('pila', [cuerpo, polo]); uv_cilindro(ob); suavizar(ob)
    img = Image.new('RGB', (256, 256), (25, 25, 28)); d = ImageDraw.Draw(img)
    d.rectangle([0, 0, 256, 70], fill=(200, 120, 40)); d.rectangle([0, 0, 256, 10], fill=(190, 190, 195)); d.rectangle([0, 246, 256, 256], fill=(190, 190, 195))
    d.text((20, 120), '1.5V', font=fuente('Bold', 48), fill=(230, 230, 230))
    terminar(ob, img, 'pila.jpg', 0.12, base=True)

def senal():
    poste = cilindro('poste', 0.03, 2.2, (0, 0, 1.1), 12); uv_constante(poste, 0.002, 0.002)
    bm = bmesh.new(); pts = [(0.38 * math.cos(math.pi / 8 + k * math.pi / 4), 0.38 * math.sin(math.pi / 8 + k * math.pi / 4)) for k in range(8)]
    frente = [bm.verts.new((x, -0.05, 2.0 + z)) for x, z in pts]; atras = [bm.verts.new((x, -0.03, 2.0 + z)) for x, z in pts]
    ff = bm.faces.new(frente[::-1]); bm.faces.new(atras)
    for k in range(8): bm.faces.new((frente[k], frente[(k + 1) % 8], atras[(k + 1) % 8], atras[k]))
    uv = bm.loops.layers.uv.verify()
    for f in bm.faces:
        for l in f.loops: l[uv].uv = ((l.vert.co.x + 0.4) / 0.8, (l.vert.co.z - 1.6) / 0.8) if f == ff else (0.002, 0.002)
    placa = nuevo_objeto('placa', bm)
    ob = combinar('senal', [poste, placa])
    img = Image.new('RGB', (512, 512), (150, 150, 150)); d = ImageDraw.Draw(img)
    octa = [(256 + 240 * math.cos(math.pi / 8 + k * math.pi / 4), 256 + 240 * math.sin(math.pi / 8 + k * math.pi / 4)) for k in range(8)]
    d.polygon(octa, fill=(240, 240, 240)); d.polygon([(256 + (x - 256) * 0.9, 256 + (y - 256) * 0.9) for x, y in octa], fill=(190, 30, 30))
    f = fuente('Black', 150); w = d.textlength('ALTO', font=f); d.text((256 - w / 2, 160), 'ALTO', font=f, fill=(245, 245, 245))
    arr = np.asarray(img).astype(float); m = np.clip((ruido(512, 512, 6, 7) - 0.55) * 3, 0, 1)[..., None]
    arr = arr * (1 - m * 0.6) + np.array((110, 70, 40)) * m * 0.6
    terminar(ob, np.clip(arr, 0, 255), 'senal.jpg', base=True)

# ---------------------------------------------------------------- fauna
def medusa():
    bm = bmesh.new(); bmesh.ops.create_uvsphere(bm, u_segments=28, v_segments=14, radius=0.25)
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if v.co.z < -0.02], context='VERTS')
    for v in bm.verts: v.co.z *= 0.75
    campana = nuevo_objeto('cam', bm); piezas = [campana]; g = random.Random(17)
    for k in range(16):
        a = k / 16 * 2 * math.pi; r = 0.22
        pts = [(math.cos(a) * r, math.sin(a) * r, 0.0)] + [(math.cos(a) * r * (1 - t * 0.2) + 0.03 * math.sin(t * 9 + k), math.sin(a) * r * (1 - t * 0.2), -t * g.uniform(0.6, 0.95)) for t in np.linspace(0.1, 1, 8)]
        piezas.append(skin(f't{k}', pts, [(i, i + 1) for i in range(len(pts) - 1)], [0.006] * len(pts), 0))
    for k in range(4):
        a = k / 4 * 2 * math.pi
        pts = [(0.03 * math.cos(a + t * 5), 0.03 * math.sin(a + t * 5), -t * 0.5) for t in np.linspace(0, 1, 10)]
        piezas.append(skin(f'b{k}', pts, [(i, i + 1) for i in range(9)], [0.03 * (1 - i / 12) for i in range(10)], 1))
    ob = combinar('medusa', piezas); uv_lateral(ob); suavizar(ob)
    v = np.linspace(1, 0, 256)[:, None, None]
    arr = np.array((250, 205, 230)) * v + np.array((210, 150, 200)) * (1 - v)
    arr = np.repeat(arr, 256, 1); img = Image.fromarray(np.clip(arr, 0, 255).astype(np.uint8)); d = ImageDraw.Draw(img)
    for k in range(4): d.ellipse([60 + k * 36, 30, 90 + k * 36, 70], outline=(255, 140, 190), width=4)
    terminar(ob, img, 'medusa.jpg', 1.1)

def erizo():
    cuerpo = esfera('c', 0.07, (0, 0, 0.05), (1, 1, 0.75), 3); piezas = [cuerpo]
    for v in list(cuerpo.data.vertices)[::2]:
        n = (v.co - Vector((0, 0, 0.05))).normalized()
        if n.z < -0.4: continue
        bm = bmesh.new(); bmesh.ops.create_cone(bm, cap_ends=True, segments=4, radius1=0.006, radius2=0.0, depth=0.16)
        rot = n.to_track_quat('Z', 'Y').to_matrix().to_4x4()
        bmesh.ops.transform(bm, matrix=Matrix.Translation(v.co + n * 0.075) @ rot, verts=bm.verts)
        piezas.append(nuevo_objeto('p', bm))
    ob = combinar('erizo', piezas); uv_caja(ob, 5.0)
    terminar(ob, tex_ruido((35, 15, 45), (80, 40, 95), 51, 10, 8), 'erizo.jpg', 0.32, base=True)

def estrella():
    bm = bmesh.new(); arriba = bm.verts.new((0, 0, 0.045)); abajo = bm.verts.new((0, 0, 0.0))
    ext_r, int_r = [], []
    anillo_a, anillo_b = [], []
    for k in range(10):
        a = k / 10 * 2 * math.pi; r = 0.15 if k % 2 == 0 else 0.055
        h = 0.012 if k % 2 == 0 else 0.03
        anillo_a.append(bm.verts.new((math.cos(a) * r, math.sin(a) * r, h)))
        anillo_b.append(bm.verts.new((math.cos(a) * r, math.sin(a) * r, 0.0)))
    for k in range(10):
        n = (k + 1) % 10
        bm.faces.new((arriba, anillo_a[k], anillo_a[n])); bm.faces.new((abajo, anillo_b[n], anillo_b[k]))
        bm.faces.new((anillo_a[k], anillo_b[k], anillo_b[n], anillo_a[n]))
    bmesh.ops.subdivide_edges(bm, edges=bm.edges[:], cuts=2, use_grid_fill=True)
    ob = nuevo_objeto('estrella_mar', bm); uv_caja(ob, 6.0); suavizar(ob)
    img = Image.fromarray(tex_ruido((210, 85, 35), (245, 140, 60), 61, 8, 8).astype(np.uint8)); d = ImageDraw.Draw(img); gg = random.Random(3)
    for _ in range(400):
        x, y = gg.uniform(0, 512), gg.uniform(0, 512); d.ellipse([x - 3, y - 3, x + 3, y + 3], fill=(255, 205, 140))
    terminar(ob, img, 'estrella.jpg', 0.3, base=True)

def cangrejo_lata():
    for ruta in (os.path.join(SALIDA, 'arrecifes', 'cangrejoermitano.fbx'), os.path.join(CARPETA, 'lata.fbx')):
        bpy.ops.import_scene.fbx(filepath=ruta)
    obs = [o for o in bpy.context.scene.objects if o.type == 'MESH']
    crab = [o for o in obs if 'cangrejo' in o.name.lower()][0]; lata = [o for o in obs if o is not crab][0]
    for o in (crab, lata):
        o.data.transform(o.matrix_world); o.matrix_world = Matrix.Identity(4)
    lata.data.transform(Matrix.Rotation(math.radians(90), 4, 'Y'))          # lata acostada
    lata.data.transform(Matrix.Scale(0.85, 4))
    xs = [v.co for v in crab.data.vertices]
    mn = Vector([min(c[i] for c in xs) for i in range(3)]); mx = Vector([max(c[i] for c in xs) for i in range(3)])
    L, A = mx - mn, None
    # la concha es la parte trasera y alta del modelo: se quita y la lata ocupa su lugar
    bm = bmesh.new(); bm.from_mesh(crab.data)
    concha = [v for v in bm.verts if v.co.x < mn.x + L.x * 0.55 and v.co.z > mn.z + L.z * 0.30]
    centro = sum((v.co for v in concha), Vector()) / max(1, len(concha))
    bmesh.ops.delete(bm, geom=concha, context='VERTS'); bm.to_mesh(crab.data); bm.free()
    ls = [v.co for v in lata.data.vertices]
    lc = Vector([(min(c[i] for c in ls) + max(c[i] for c in ls)) / 2 for i in range(3)])
    lata.data.transform(Matrix.Translation(centro - lc + Vector((-0.01, 0, 0.0))))
    for o in bpy.context.scene.objects: o.select_set(False)
    crab.name = 'cangrejo_lata_cuerpo'; crab.select_set(True); lata.select_set(True); bpy.context.view_layer.objects.active = crab
    shutil.copy(os.path.join(SALIDA, 'arrecifes', 'cangrejoermitano.jpg'), os.path.join(CARPETA, 'cangrejoermitano.jpg'))
    bpy.ops.export_scene.fbx(filepath=os.path.join(CARPETA, 'cangrejo_lata.fbx'), use_selection=True, object_types={'MESH'},
                             apply_scale_options='FBX_SCALE_ALL', axis_forward='-Z', axis_up='Y', bake_space_transform=True,
                             path_mode='STRIP', embed_textures=False, mesh_smooth_type='FACE', add_leaf_bones=False, bake_anim=False)
    print('GENERADO cangrejo_lata')

def textura_tiburon_blanco():
    W, H = 512, 256; v = np.linspace(1, 0, H)[:, None, None]
    corte = 0.42 + 0.04 * np.sin(np.linspace(0, 12, W))[None, :, None]
    t = np.clip((v - corte) / 0.04, 0, 1)
    arr = np.array((236, 238, 240)) * (1 - t) + np.array((92, 100, 110)) * t
    arr = arr + np.random.default_rng(4).normal(0, 4, arr.shape)
    Image.fromarray(np.clip(arr, 0, 255).astype(np.uint8)).save(os.path.join(CARPETA, 'Tiburon.jpg'), quality=90)
    print('GENERADO textura tiburon blanco')

def fondo_grava():
    img = tex_ruido((60, 62, 58), (110, 108, 100), 71, 14, 18)
    piedras = Image.fromarray(img.astype(np.uint8)); d = ImageDraw.Draw(piedras); g = random.Random(5)
    for _ in range(1400):
        x, y = g.uniform(0, 512), g.uniform(0, 512); r = g.uniform(2, 7); c = g.randint(55, 150)
        d.ellipse([x - r, y - r * 0.8, x + r, y + r * 0.8], fill=(c, c - 3, c - 10))
    piedras.save(os.path.join(CARPETA, 'sedimento.jpg'), quality=90)
    print('GENERADO grava')

# ---------------------------------------------------------------- letreros
LETREROS = {
    'letrero_bienvenida': ('ZONA 3 · NAUFRAGIOS', (110, 210, 255), [
        'Este fondo está invadido por nuestra basura.',
        '1. Apunta y haz clic en los residuos para recogerlos.',
        '2. Retira a los peces león invasores.',
        '3. Libera a la tortuga atrapada en la red.',
        '4. Retira el barril de residuos tóxicos.',
        'Mientras más limpies, más clara será el agua.']),
    'letrero_plasticos': ('PLÁSTICOS EN EL MAR', (120, 200, 255), [
        'Cada año entran al océano millones de toneladas',
        'de plástico. Una botella PET puede tardar unos',
        '450 años en degradarse.',
        'El plástico se rompe en microplásticos que comen',
        'los peces... y que terminan en nuestra comida.']),
    'letrero_redes': ('REDES FANTASMA', (255, 170, 110), [
        'Son redes de pesca perdidas o abandonadas que',
        'siguen atrapando animales durante años.',
        'Tortugas, delfines y vaquitas mueren enredados.',
        'Representan cerca del 10% de la basura marina.',
        'Mantén presionado el clic sobre la red para cortarla.']),
    'letrero_invasor': ('PEZ LEÓN: ESPECIE INVASORA', (255, 200, 90), [
        'Llegó al Caribe desde el Indo-Pacífico, muy',
        'probablemente por liberación de acuarios.',
        'Aquí no tiene depredadores y devora peces nativos.',
        'En México se promueve su pesca y consumo',
        'para controlarlo. Retíralo con un clic.']),
    'letrero_toxicos': ('RESIDUOS TÓXICOS', (170, 255, 120), [
        'Barriles, pilas y combustibles liberan metales',
        'pesados y químicos al agua.',
        'Se acumulan en la cadena alimenticia: del',
        'plancton a los peces que comemos.',
        'Nunca tires pilas ni aceites a la basura común.']),
}

def letrero(nombre, titulo, color, lineas):
    postes = [cilindro(f'p{s}', 0.05, 2.1, (s * 0.85, 0.06, 1.05), 10) for s in (-1, 1)]
    for p in postes: uv_constante(p, 0.002, 0.002)
    bm = bmesh.new(); bmesh.ops.create_cube(bm, size=1.0)
    for v in bm.verts: v.co = Vector((v.co.x * 1.9, v.co.y * 0.08, v.co.z * 1.15 + 1.45))
    uv = bm.loops.layers.uv.verify()
    for f in bm.faces:
        frente = f.normal.y < -0.9
        for l in f.loops:
            l[uv].uv = ((l.vert.co.x + 0.95) / 1.9, (l.vert.co.z - 0.875) / 1.15) if frente else (0.002, 0.002)
    panel = nuevo_objeto('panel', bm)
    ob = combinar(nombre, postes + [panel])
    W, H = 1024, 620
    img = Image.new('RGB', (W, H), (8, 34, 46)); d = ImageDraw.Draw(img)
    d.rectangle([0, 0, W, 8], fill=(70, 75, 70)); d.rectangle([14, 14, W - 14, H - 14], outline=color, width=6)
    d.rectangle([14, 14, W - 14, 118], fill=tuple(int(c * 0.35) for c in color))
    f1 = fuente('Black', 58); w = d.textlength(titulo, font=f1); d.text((W / 2 - w / 2, 30), titulo, font=f1, fill=(255, 255, 255))
    f2 = fuente('Medium', 38); y = 150
    for l in lineas:
        bold = l[:2] in ('1.', '2.', '3.', '4.')
        d.text((60, y), l, font=fuente('Bold', 38) if bold else f2, fill=color if bold else (230, 240, 245)); y += 74
    arr = np.asarray(img).astype(float); m = np.clip((ruido(H, W, 5, 9) - 0.62) * 3, 0, 1)[..., None]
    arr = arr * (1 - m * 0.35) + np.array((70, 100, 60)) * m * 0.35                       # algas en el letrero
    ImageDraw.Draw(img); arr[0:6, :] = (90, 90, 90); arr[2:4, 2:4] = (90, 90, 90)
    terminar(ob, np.clip(arr, 0, 255), nombre + '.jpg', base=True)

if __name__ == '__main__':
    solo = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
    for fn in (roncador, dron, ancla, muro_arcos, torre_rota, cofre, botella, red_fantasma, barril, timon, bota, pila, senal,
               medusa, erizo, estrella, cangrejo_lata, textura_tiburon_blanco, fondo_grava):
        if solo and fn.__name__ not in solo: continue
        limpiar_escena(); fn()
    for n, (t, c, l) in LETREROS.items():
        if solo and 'letreros' not in solo: continue
        limpiar_escena(); letrero(n, t, c, l)
    print('LISTO')
