# Genera en Blender los modelos de la Zona 1 que el equipo no tenia:
# pulpo, pez globo, delfin nariz de botella, esponja de tubo, langosta,
# anemona, pasto marino y respiradero hidrotermal.
#   blender --background --python herramientas/exporta_zona1_extras.py
# Usa las mismas convenciones que exporta_zonas.py (cabeza a +X, lomo a +Y, metros).
import os, sys, math, random
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bpy, bmesh
import numpy as np
from mathutils import Vector, Matrix
from PIL import Image, ImageDraw, ImageFilter
from exporta_zonas import (SALIDA, limpiar_escena, guardar_jpg, material_con_textura, uv_caja, uv_lateral,
                           exportar, nuevo_objeto, suavizar, ruido, tex_ruido, normalizar)

CARPETA = os.path.join(SALIDA, 'arrecifes')

# ---------------------------------------------------------------- utilidades
def skin(nombre, verts, edges, radios, subdiv=1, raiz=0):
    me = bpy.data.meshes.new(nombre); me.from_pydata(verts, edges, [])
    ob = bpy.data.objects.new(nombre, me); bpy.context.scene.collection.objects.link(ob)
    ob.modifiers.new('skin', 'SKIN')
    for i, r in enumerate(radios):
        ob.data.skin_vertices[0].data[i].radius = r if isinstance(r, tuple) else (r, r)
    ob.data.skin_vertices[0].data[raiz].use_root = True
    if subdiv:
        s = ob.modifiers.new('sub', 'SUBSURF'); s.levels = subdiv; s.render_levels = subdiv
    ev = ob.evaluated_get(bpy.context.evaluated_depsgraph_get()); me2 = bpy.data.meshes.new_from_object(ev)
    ob.modifiers.clear(); ob.data = me2
    return ob

def combinar(nombre, objetos):
    bm = bmesh.new()
    for o in objetos:
        bm.from_mesh(o.data)
    for o in objetos: bpy.data.objects.remove(o)
    return nuevo_objeto(nombre, bm)

def uv_constante(ob, u, v):
    """Todas las caras usan un mismo punto de la textura (ojos, detalles)."""
    bm = bmesh.new(); bm.from_mesh(ob.data); uv = bm.loops.layers.uv.verify()
    for f in bm.faces:
        for l in f.loops: l[uv].uv = (u, v)
    bm.to_mesh(ob.data); bm.free()

def esfera(nombre, r, centro, escala=(1, 1, 1), sub=3):
    bm = bmesh.new(); bmesh.ops.create_icosphere(bm, subdivisions=sub, radius=r)
    for v in bm.verts: v.co = Vector((v.co.x * escala[0], v.co.y * escala[1], v.co.z * escala[2])) + Vector(centro)
    return nuevo_objeto(nombre, bm)

def cuna(nombre, base_a, base_b, punta, grosor, eje='y'):
    """Aleta: triangulo con grosor (base_a, base_b, punta en 3D)."""
    bm = bmesh.new()
    d = Vector((0, grosor, 0)) if eje == 'y' else Vector((0, 0, grosor))
    vs = [bm.verts.new(Vector(p) + s * d) for s in (-1, 1) for p in (base_a, base_b, punta)]
    for f in ((0, 1, 2), (5, 4, 3), (0, 3, 4, 1), (1, 4, 5, 2), (2, 5, 3, 0)):
        bm.faces.new([vs[i] for i in f])
    return nuevo_objeto(nombre, bm)

def lienzo(w=512, h=512, color=(128, 128, 128)):
    return Image.new('RGB', (w, h), color)

def a_pixel(ob, x, z, W, H):
    """Convierte una posicion (x, z) del modelo a pixel de su textura de proyeccion lateral."""
    xs = [v.co.x for v in ob.data.vertices]; zs = [v.co.z for v in ob.data.vertices]
    u = (x - min(xs)) / (max(xs) - min(xs)); v = (z - min(zs)) / (max(zs) - min(zs))
    return u * W, (1 - v) * H

def terminar(ob, img, archivo, tam=None, base=False, rot=0):
    if tam: normalizar(ob, rot, tam, centrar=not base)
    if isinstance(img, Image.Image): img.save(os.path.join(CARPETA, archivo), quality=90)
    else: guardar_jpg(img, archivo, CARPETA)
    material_con_textura(ob, archivo); exportar(ob, CARPETA, ob.name)
    xs = [v.co for v in ob.data.vertices]
    dims = [max(c[i] for c in xs) - min(c[i] for c in xs) for i in range(3)]
    print('GENERADO', ob.name, len(ob.data.polygons), 'caras', [round(d, 2) for d in dims])

# ---------------------------------------------------------------- pez globo
def pez_globo():
    cuerpo = esfera('c', 1.0, (0, 0, 0), (1.15, 0.9, 0.92), 3)
    picos = []
    g = random.Random(5)
    for v in list(cuerpo.data.vertices)[::9]:
        n = v.co.normalized()
        if n.x > 0.75: continue                                       # sin picos en la cara
        bm = bmesh.new(); bmesh.ops.create_cone(bm, cap_ends=True, segments=5, radius1=0.05, radius2=0.0, depth=0.22)
        rot = n.to_track_quat('Z', 'Y').to_matrix().to_4x4()
        bmesh.ops.transform(bm, matrix=Matrix.Translation(v.co + n * 0.08) @ rot, verts=bm.verts)
        picos.append(nuevo_objeto('p', bm))
    cola = cuna('cola', (-1.0, 0, 0.12), (-1.0, 0, -0.12), (-1.55, 0, 0.0), 0.03)
    cola2 = cuna('cola2', (-1.4, 0, 0.0), (-1.62, 0, 0.32), (-1.62, 0, -0.32), 0.03)
    aletas = [cuna(f'a{s}', (0.35, s * 0.82, 0.05), (0.2, s * 0.82, -0.12), (0.05, s * 1.15, 0.0), 0.02, 'z') for s in (-1, 1)]
    dorsal = cuna('d', (-0.55, 0, 0.75), (-0.85, 0, 0.6), (-1.0, 0, 0.95), 0.03)
    ojos = [esfera(f'o{s}', 0.17, (0.72, s * 0.52, 0.32), (1, 1, 1), 2) for s in (-1, 1)]
    for o in ojos: uv_constante(o, 0.995, 0.995)
    boca = esfera('b', 0.09, (1.13, 0, -0.08), (0.6, 1.2, 0.8), 2); uv_constante(boca, 0.995, 0.005)
    resto = combinar('pg', [cuerpo] + picos + [cola, cola2, dorsal] + aletas); uv_lateral(resto)
    ob = combinar('pez_globo', [resto] + ojos + [boca]); suavizar(ob)
    W = H = 512; img = lienzo(W, H)
    y = np.linspace(0, 1, H)[:, None]
    arr = np.array((205, 180, 95)) * (1 - np.clip((y - 0.45) / 0.25, 0, 1)[..., None]) + np.array((240, 236, 225)) * np.clip((y - 0.45) / 0.25, 0, 1)[..., None]
    arr = np.repeat(arr, W, 1) + np.random.default_rng(1).normal(0, 5, (H, W, 3))
    img = Image.fromarray(np.clip(arr, 0, 255).astype(np.uint8)); d = ImageDraw.Draw(img); g = random.Random(2)
    for _ in range(140):                                                # manchas oscuras en el lomo
        x, yy = g.uniform(0, W), g.uniform(0, H * 0.5); r = g.uniform(5, 13)
        d.ellipse([x - r, yy - r, x + r, yy + r], fill=(90, 70, 40))
    d.rectangle([W - 6, 0, W, 6], fill=(10, 10, 12)); d.rectangle([W - 6, H - 6, W, H], fill=(60, 30, 30))   # ojos y boca
    terminar(ob, img, 'pez_globo.jpg', 0.35)

# ---------------------------------------------------------------- delfin
def delfin():
    tk = [0, 0.08, 0.3, 0.55, 0.75, 0.86, 0.91, 0.96, 1.0]
    alto = [0.03, 0.05, 0.14, 0.20, 0.18, 0.155, 0.08, 0.045, 0.02]
    ancho = [0.012, 0.035, 0.11, 0.17, 0.15, 0.12, 0.06, 0.035, 0.015]
    zc = [0.0, 0.0, 0.0, 0.01, 0.015, 0.01, -0.035, -0.045, -0.045]
    bm = bmesh.new(); anillos = []; NR, NS = 40, 18
    for i in range(NR):
        t = i / (NR - 1); x = t * 2 - 1
        a, b, c = np.interp(t, tk, alto), np.interp(t, tk, ancho), np.interp(t, tk, zc)
        anillos.append([bm.verts.new((x, b * math.cos(th), c + a * math.sin(th))) for th in np.linspace(0, 2 * math.pi, NS, endpoint=False)])
    for i in range(NR - 1):
        for s in range(NS):
            bm.faces.new((anillos[i][s], anillos[i][(s + 1) % NS], anillos[i + 1][(s + 1) % NS], anillos[i + 1][s]))
    for idx, x in ((0, -1.0), (NR - 1, 1.0)):
        c = bm.verts.new((x + (0.005 if x > 0 else -0.005), 0, zc[-1] if x > 0 else 0))
        for s in range(NS):
            q = (anillos[idx][s], anillos[idx][(s + 1) % NS], c)
            bm.faces.new(q if x > 0 else q[::-1])
    cuerpo = nuevo_objeto('cuerpo', bm)
    dorsal = cuna('dorsal', (0.24, 0, 0.18), (-0.12, 0, 0.19), (-0.24, 0, 0.44), 0.018)
    pects = [cuna(f'p{s}', (0.52, s * 0.12, -0.08), (0.40, s * 0.12, -0.10), (0.30, s * 0.40, -0.24), 0.014, 'z') for s in (-1, 1)]
    fluke = []
    for s in (-1, 1):
        fluke.append(cuna(f'f{s}', (-0.96, 0, 0.0), (-1.13, s * 0.03, 0.0), (-1.22, s * 0.36, 0.0), 0.012, 'z'))
    ob = combinar('delfin', [cuerpo, dorsal] + pects + fluke); uv_lateral(ob); suavizar(ob)
    W = H = 512; v = np.linspace(1, 0, H)[:, None]
    oscuro, medio, claro = np.array((72, 82, 94)), np.array((128, 140, 152)), np.array((228, 222, 224))
    t1 = np.clip((v - 0.40) / 0.18, 0, 1)[..., None]; t2 = np.clip((v - 0.30) / 0.12, 0, 1)[..., None]
    arr = claro * (1 - t2) + (medio * (1 - t1) + oscuro * t1) * t2
    arr = np.repeat(arr, W, 1) + np.random.default_rng(3).normal(0, 3, (H, W, 3))
    img = Image.fromarray(np.clip(arr, 0, 255).astype(np.uint8)); d = ImageDraw.Draw(img)
    ex, ey = a_pixel(ob, 0.72, 0.04, W, H); d.ellipse([ex - 5, ey - 4, ex + 5, ey + 4], fill=(15, 15, 18))
    img = img.filter(ImageFilter.GaussianBlur(1.2))
    terminar(ob, img, 'delfin.jpg', 2.6)

# ---------------------------------------------------------------- pulpo
def pulpo():
    g = random.Random(9)
    manto = esfera('manto', 0.32, (-0.12, 0, 0.42), (1.35, 1.0, 1.15), 3)
    cabeza = esfera('cabeza', 0.22, (0.08, 0, 0.18), (1.0, 1.15, 0.9), 3)
    brazos = []
    for k in range(8):
        a = (k + 0.5) / 8 * 2 * math.pi
        dx, dy = math.cos(a), math.sin(a)
        pts = [(0.05 + dx * 0.10, dy * 0.10, 0.08)]
        r = 0.10; z = 0.05
        for s in range(1, 14):
            t = s / 13
            ang = a + 0.35 * math.sin(t * 3 + k)
            r += 0.075 * (1 - t * 0.3); z = 0.04 * math.sin(t * math.pi) + (0.10 * t * t if k % 3 == 0 else 0)
            curl = 0.12 * t ** 3
            pts.append((0.05 + math.cos(ang + curl * 6) * r, math.sin(ang + curl * 6) * r, z))
        radios = [0.075 * (1 - i / len(pts)) ** 1.2 + 0.008 for i in range(len(pts))]
        brazos.append(skin(f'b{k}', pts, [(i, i + 1) for i in range(len(pts) - 1)], radios, 1))
    ojos = [esfera(f'o{s}', 0.055, (0.20, s * 0.14, 0.30), (1, 1, 1), 2) for s in (-1, 1)]
    for o in ojos: uv_constante(o, 0.995, 0.995)
    resto = combinar('pr', [manto, cabeza] + brazos); uv_caja(resto, 1.5)
    ob = combinar('pulpo', [resto] + ojos); suavizar(ob)
    img = tex_ruido((150, 55, 40), (215, 120, 80), 31, 8, 12)
    img = Image.fromarray(img.astype(np.uint8)); d = ImageDraw.Draw(img); gg = random.Random(4)
    for _ in range(220):
        x, y = gg.uniform(0, 512), gg.uniform(0, 512); r = gg.uniform(2, 6)
        d.ellipse([x - r, y - r, x + r, y + r], fill=(235, 190, 150))
    d.rectangle([506, 0, 512, 6], fill=(20, 15, 10))
    terminar(ob, img, 'pulpo.jpg', 1.1, base=True)

# ---------------------------------------------------------------- langosta
def langosta():
    piezas = [esfera('capa', 0.16, (0.15, 0, 0.12), (1.6, 0.85, 0.75), 3)]
    x = -0.1
    for i in range(6):                                                 # abdomen segmentado
        r = 0.12 - i * 0.012
        piezas.append(esfera(f's{i}', r, (x, 0, 0.11 - i * 0.012), (0.75, 1.0, 0.62), 2)); x -= r * 1.25
    abanico = [cuna(f'ab{k}', (x + 0.04, 0, 0.05), (x + 0.04, 0, 0.05), (x - 0.12, (k - 2) * 0.06, 0.03), 0.01, 'z') for k in range(5)]
    piezas += abanico
    for s in (-1, 1):
        brazo = [(0.32, s * 0.07, 0.10), (0.42, s * 0.16, 0.10), (0.52, s * 0.20, 0.12)]
        piezas.append(skin(f'br{s}', brazo, [(0, 1), (1, 2)], [0.035, 0.03, 0.03], 1))
        piezas.append(esfera(f'pinza{s}', 0.07, (0.64, s * 0.22, 0.12), (1.6, 0.8, 0.55), 2))
        antena = [(0.35, s * 0.04, 0.16)] + [(0.35 + k * 0.11, s * (0.06 + k * 0.035), 0.18 + 0.05 * math.sin(k * 0.6)) for k in range(1, 9)]
        piezas.append(skin(f'an{s}', antena, [(i, i + 1) for i in range(len(antena) - 1)], [0.018 * (1 - i / 10) + 0.004 for i in range(len(antena))], 0))
        for k in range(4):                                               # patas
            px = 0.22 - k * 0.07
            pata = [(px, s * 0.08, 0.06), (px + 0.02, s * 0.20, 0.08), (px - 0.02, s * 0.26, -0.02)]
            piezas.append(skin(f'pa{s}{k}', pata, [(0, 1), (1, 2)], [0.014, 0.012, 0.008], 0))
    ojos = [esfera(f'o{s}', 0.025, (0.36, s * 0.06, 0.19), (1, 1, 1), 2) for s in (-1, 1)]
    for o in ojos: uv_constante(o, 0.995, 0.995)
    resto = combinar('lr', piezas); uv_caja(resto, 3.0)
    ob = combinar('langosta', [resto] + ojos); suavizar(ob)
    img = tex_ruido((120, 45, 30), (200, 95, 45), 41, 10, 10)
    img = Image.fromarray(img.astype(np.uint8)); d = ImageDraw.Draw(img); gg = random.Random(6)
    for _ in range(160):
        x, y = gg.uniform(0, 512), gg.uniform(0, 512); r = gg.uniform(2, 5)
        d.ellipse([x - r, y - r, x + r, y + r], fill=(245, 225, 190))
    d.rectangle([506, 0, 512, 6], fill=(15, 12, 10))
    terminar(ob, img, 'langosta.jpg', 0.6, base=True)

# ---------------------------------------------------------------- anemona
def anemona():
    g = random.Random(12)
    bm = bmesh.new(); bmesh.ops.create_cone(bm, cap_ends=True, segments=24, radius1=0.22, radius2=0.26, depth=0.22)
    bmesh.ops.translate(bm, vec=(0, 0, 0.11), verts=bm.verts)
    base = nuevo_objeto('base', bm); piezas = [base]
    for k in range(70):
        a = g.uniform(0, 2 * math.pi); r0 = g.uniform(0.03, 0.24)
        p0 = (math.cos(a) * r0, math.sin(a) * r0, 0.22)
        largo = g.uniform(0.18, 0.32); sal = 0.4 + r0 * 2.5
        pts = [p0]
        for s in range(1, 6):
            t = s / 5
            pts.append((p0[0] + math.cos(a) * sal * largo * t, p0[1] + math.sin(a) * sal * largo * t, p0[2] + largo * (t - 0.25 * t * t * sal)))
        piezas.append(skin(f't{k}', pts, [(i, i + 1) for i in range(5)], [0.022, 0.02, 0.018, 0.015, 0.012, 0.014], 0))
    ob = combinar('anemona', piezas); uv_lateral(ob); suavizar(ob)
    v = np.linspace(1, 0, 256)[:, None, None]
    arr = np.array((250, 245, 235)) * np.clip((v - 0.2) / 0.5, 0, 1) + np.array((185, 175, 165)) * (1 - np.clip((v - 0.2) / 0.5, 0, 1))
    arr = np.repeat(arr, 256, 1) + np.random.default_rng(2).normal(0, 4, (256, 256, 3))
    terminar(ob, np.clip(arr, 0, 255), 'anemona.jpg', 0.65, base=True)

# ---------------------------------------------------------------- esponja de tubo
def esponja():
    g = random.Random(21); piezas = []
    for k in range(4):
        a = k / 4 * 2 * math.pi + g.uniform(-0.3, 0.3); d = 0.0 if k == 0 else g.uniform(0.15, 0.28)
        cx, cy = math.cos(a) * d, math.sin(a) * d; alto = g.uniform(0.5, 1.1) if k else 1.2; r = g.uniform(0.08, 0.12)
        inc = Vector((cx, cy, 0)) * 0.25
        bm = bmesh.new(); seg = 18; filas = []
        for i in range(9):
            t = i / 8; z = alto * t; rr = r * (0.85 + 0.25 * t) * (1 + 0.05 * math.sin(t * 9 + k))
            filas.append([bm.verts.new((cx + inc.x * z + math.cos(s / seg * 2 * math.pi) * rr, cy + inc.y * z + math.sin(s / seg * 2 * math.pi) * rr, z)) for s in range(seg)])
        dentro = [bm.verts.new((v.co.x - (v.co.x - cx - inc.x * alto) * 0.25, v.co.y - (v.co.y - cy - inc.y * alto) * 0.25, alto - 0.25)) for v in filas[-1]]
        for i in range(8):
            for s in range(seg): bm.faces.new((filas[i][s], filas[i][(s + 1) % seg], filas[i + 1][(s + 1) % seg], filas[i + 1][s]))
        for s in range(seg): bm.faces.new((filas[-1][s], filas[-1][(s + 1) % seg], dentro[(s + 1) % seg], dentro[s]))
        bm.faces.new(dentro[::-1])
        piezas.append(nuevo_objeto(f'e{k}', bm))
    ob = combinar('esponja_tubo', piezas); uv_caja(ob, 3.0); suavizar(ob)
    terminar(ob, tex_ruido((205, 200, 195), (255, 252, 248), 22, 24, 16), 'esponja.jpg', 1.2, base=True)

# ---------------------------------------------------------------- pasto marino
def pasto_marino():
    g = random.Random(31); bm = bmesh.new(); uv = bm.loops.layers.uv.verify()
    for k in range(45):
        a = g.uniform(0, 2 * math.pi); r0 = g.uniform(0, 0.35); bx, by = math.cos(a) * r0, math.sin(a) * r0
        alto = g.uniform(0.45, 0.95); ang = g.uniform(0, math.pi); w = 0.018
        lean = Vector((g.uniform(-0.25, 0.25), g.uniform(-0.25, 0.25), 0))
        filas = []
        for s in range(9):
            t = s / 8; z = alto * t; o = lean * t * t * alto
            dx, dy = math.cos(ang) * w * (1 - 0.5 * t), math.sin(ang) * w * (1 - 0.5 * t)
            filas.append((bm.verts.new((bx + o.x - dx, by + o.y - dy, z)), bm.verts.new((bx + o.x + dx, by + o.y + dy, z)), t))
        for s in range(8):
            f = bm.faces.new((filas[s][0], filas[s][1], filas[s + 1][1], filas[s + 1][0]))
            for l, (uu, vv) in zip(f.loops, ((0, filas[s][2]), (1, filas[s][2]), (1, filas[s + 1][2]), (0, filas[s + 1][2]))):
                l[uv].uv = (uu, vv)
    ob = nuevo_objeto('pasto_marino', bm); suavizar(ob)
    v = np.linspace(1, 0, 256)[:, None, None]
    arr = np.array((120, 175, 70)) * v + np.array((45, 85, 35)) * (1 - v)
    arr = np.repeat(arr, 64, 1) + np.random.default_rng(5).normal(0, 4, (256, 64, 3))
    terminar(ob, np.clip(arr, 0, 255), 'pasto_marino.jpg')

# ---------------------------------------------------------------- respiradero hidrotermal
def respiradero():
    g = random.Random(41); piezas = []
    for (cx, cy, alto, r) in [(0, 0, 3.2, 0.55), (0.6, 0.3, 1.8, 0.32), (-0.5, 0.45, 1.3, 0.28), (0.2, -0.6, 2.2, 0.3)]:
        bm = bmesh.new(); seg = 20; filas = []
        for i in range(13):
            t = i / 12; z = alto * t
            rr = r * (1.25 - 0.55 * t) * (1 + 0.18 * math.sin(t * 17 + cx * 5))
            filas.append([bm.verts.new((cx + math.cos(s / seg * 6.283) * rr * (1 + g.uniform(-0.08, 0.08)),
                                        cy + math.sin(s / seg * 6.283) * rr * (1 + g.uniform(-0.08, 0.08)), z)) for s in range(seg)])
        boca = [bm.verts.new((v.co.x * 0.6 + cx * 0.4, v.co.y * 0.6 + cy * 0.4, alto - 0.15)) for v in filas[-1]]
        for i in range(12):
            for s in range(seg): bm.faces.new((filas[i][s], filas[i][(s + 1) % seg], filas[i + 1][(s + 1) % seg], filas[i + 1][s]))
        for s in range(seg): bm.faces.new((filas[-1][s], filas[-1][(s + 1) % seg], boca[(s + 1) % seg], boca[s]))
        bm.faces.new(boca[::-1])
        piezas.append(nuevo_objeto('ch', bm))
    ob = combinar('respiradero', piezas); uv_caja(ob, 0.8); suavizar(ob)
    img = tex_ruido((22, 20, 20), (70, 58, 48), 51, 7, 8)
    vetas = ruido(512, 512, 10, 52)[..., None]
    m = np.clip((vetas - 0.58) * 6, 0, 1)
    img = img * (1 - m) + np.array((205, 150, 60)) * m
    terminar(ob, np.clip(img, 0, 255), 'respiradero.jpg')

if __name__ == '__main__':
    limpiar_escena()
    for fn in (pez_globo, delfin, pulpo, langosta, anemona, esponja, pasto_marino, respiradero):
        limpiar_escena(); fn()
    print('LISTO')
