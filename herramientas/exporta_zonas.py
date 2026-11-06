# Exporta los modelos del equipo y genera la escenografia de las 3 zonas.
# Ejecutar con Blender 5.2 (o el modulo bpy 5.2):
#   blender --background --python herramientas/exporta_zonas.py
#   python herramientas/exporta_zonas.py           (si tienes el modulo bpy)
#
# Convenciones de exportacion (iguales para todos los modelos):
#   - la cabeza del animal mira a +X y el lomo a +Y (en OpenGL)
#   - el tamano mayor del modelo queda en metros reales (tabla MODELOS)
#   - el origen queda en el centro de la caja envolvente
#   - FBX con transformaciones horneadas (no hace falta rotar -90 en el codigo)
#   - una textura .jpg de 1024 px junto al .fbx (stb_image no lee .webp)
import bpy, bmesh, os, sys, math, random, glob
import numpy as np
from mathutils import Vector, Matrix
from PIL import Image, ImageDraw, ImageFilter

RAIZ = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
ENTRADA = os.path.join(RAIZ, 'ModelosProyecto')
SALIDA = os.path.join(RAIZ, 'src', 'bin', 'models', 'zonas')
TEX = 1024

# nombre: (zona, rotacion en Z para que la cabeza mire a +X, tamano mayor en metros)
MODELOS = {
    'PezPayaso': ('arrecifes', 180, 0.45), 'Pezcirujano': ('arrecifes', 180, 0.5),
    'PezLoro': ('arrecifes', 180, 0.7),
    'CaballitoMar': ('arrecifes', 180, 0.5), 'tortuga': ('arrecifes', -90, 1.4),
    'mantaraya': ('arrecifes', 90, 3.5), 'cangrejoermitano': ('arrecifes', 90, 0.35),
    'almejagigante': ('arrecifes', 0, 1.0), 'manati': ('arrecifes', 90, 3.2),
    'pescado': ('arrecifes', 90, 1.2), 'pezdracoide': ('arrecifes', 180, 0.8),
    'ajolote': ('arrecifes', 90, 0.4),
    'foca': ('aguas_frias', 90, 1.8), 'pinguino': ('aguas_frias', 90, 1.0),
    'ballena': ('aguas_frias', 90, 12.0), 'beluga': ('aguas_frias', 90, 4.0),
    'morsa': ('aguas_frias', 0, 2.8), 'osito': ('aguas_frias', 90, 2.4),
    'Krill': ('aguas_frias', 90, 0.12), 'bacalao': ('aguas_frias', 0, 1.0),
    'lata': ('naufragios', 0, 0.25), 'lataaplastada': ('naufragios', 0, 0.2),
    'Llanta': ('naufragios', 0, 0.9), 'bolsaplastico': ('naufragios', 0, 0.6),
    'pezleon': ('naufragios', 90, 0.5), 'barracuda': ('naufragios', 180, 1.4),
    'Tiburon': ('naufragios', 90, 3.0), 'vacamarina': ('naufragios', 90, 1.5),
    'BarcoRuinas': ('naufragios', 0, 14.0),            # modelo de Julian
}
# Modelos sin textura utilizable: se les da una proyeccion lateral y una textura pintada
PINTADOS = {
    'PezLoro': ((25, 105, 115), (244, 198, 78), (32, 165, 145)),
    'Pezcirujano': ((40, 80, 200), (25, 50, 150), (250, 205, 40)),
    'barracuda': ((70, 85, 100), (215, 222, 228), None),
    'Tiburon': ((85, 100, 115), (230, 232, 235), None),
}
OMITIR = ('view', 'metarig', 'reference')
# texturas que no vienen empaquetadas en el .blend pero si estan en el repositorio
TEXTURA_EXTERNA = {
    'tortuga': os.path.join(RAIZ, 'src', 'bin', 'models', 'zonas', 'arrecifes', 'texturaTortuga.png'),
}
SOLO = [a for a in sys.argv[sys.argv.index('--') + 1:]] if '--' in sys.argv else []
rnd = random.Random(7)

# ---------------------------------------------------------------- utilidades
def limpiar_escena():
    bpy.ops.wm.read_factory_settings(use_empty=True)

def guardar_jpg(arr, nombre, carpeta):
    os.makedirs(carpeta, exist_ok=True)
    Image.fromarray(arr.astype(np.uint8), 'RGB').save(os.path.join(carpeta, nombre), quality=90)

def imagen_a_jpg(img, nombre, carpeta):
    w, h = img.size
    px = np.empty(w * h * 4, np.float32); img.pixels.foreach_get(px)
    px = px.reshape(h, w, 4)[::-1, :, :3]
    px = np.clip(px, 0, 1) ** (1 / 2.2) if img.colorspace_settings.name == 'Linear Rec.709' else np.clip(px, 0, 1)
    im = Image.fromarray((px * 255).astype(np.uint8), 'RGB')
    if max(w, h) > TEX: im = im.resize((TEX, TEX) if w == h else (TEX, int(TEX * h / w)), Image.LANCZOS)
    os.makedirs(carpeta, exist_ok=True); im.save(os.path.join(carpeta, nombre), quality=90)

def material_con_textura(obj, archivo):
    m = bpy.data.materials.new('M_' + archivo.split('.')[0])
    m.use_nodes = True
    nt = m.node_tree; bsdf = nt.nodes.get('Principled BSDF')
    tx = nt.nodes.new('ShaderNodeTexImage')
    im = bpy.data.images.new(archivo, 4, 4); im.filepath = '//' + archivo; im.source = 'FILE'
    tx.image = im
    nt.links.new(tx.outputs['Color'], bsdf.inputs['Base Color'])
    obj.data.materials.clear(); obj.data.materials.append(m)

def uv_caja(obj, escala=1.0):
    """Mapeo de caja: cada cara se proyecta sobre el eje dominante de su normal."""
    bm = bmesh.new(); bm.from_mesh(obj.data)
    uv = bm.loops.layers.uv.verify()
    for f in bm.faces:
        n = f.normal; a = max(range(3), key=lambda i: abs(n[i]))
        for l in f.loops:
            co = l.vert.co
            u, v = [(co.y, co.z), (co.x, co.z), (co.x, co.y)][a]
            l[uv].uv = (u * escala, v * escala)
    bm.to_mesh(obj.data); bm.free()

def uv_lateral(obj):
    """Proyeccion lateral (X = largo, Z = alto) normalizada a 0..1."""
    bm = bmesh.new(); bm.from_mesh(obj.data)
    uv = bm.loops.layers.uv.verify()
    xs = [v.co.x for v in bm.verts]; zs = [v.co.z for v in bm.verts]
    x0, x1, z0, z1 = min(xs), max(xs), min(zs), max(zs)
    for f in bm.faces:
        for l in f.loops:
            l[uv].uv = ((l.vert.co.x - x0) / (x1 - x0 + 1e-6), (l.vert.co.z - z0) / (z1 - z0 + 1e-6))
    bm.to_mesh(obj.data); bm.free()

def textura_pintada(colores, nombre, carpeta):
    lomo, vientre, cola = colores
    v = np.linspace(1, 0, 256)[:, None, None]                       # fila 0 = arriba (lomo)
    t = np.clip((v - 0.35) / 0.4, 0, 1)
    img = np.array(vientre) * (1 - t) + np.array(lomo) * t
    img = np.repeat(img, 512, axis=1)
    ruido = np.random.default_rng(3).normal(0, 6, img.shape)
    img = img + ruido
    if cola: img[:, :80] = img[:, :80] * 0.3 + np.array(cola) * 0.7   # cabeza en +X => la cola queda en u bajo
    guardar_jpg(np.clip(img, 0, 255), nombre, carpeta)

def normalizar(obj, rot_z, tam, centrar=True):
    obj.data.transform(Matrix.Rotation(math.radians(rot_z), 4, 'Z'))
    xs = [v.co for v in obj.data.vertices]
    mn = Vector((min(c.x for c in xs), min(c.y for c in xs), min(c.z for c in xs)))
    mx = Vector((max(c.x for c in xs), max(c.y for c in xs), max(c.z for c in xs)))
    ctr = (mn + mx) / 2; s = tam / max(mx - mn)
    obj.data.transform(Matrix.Translation(-ctr) if centrar else Matrix.Translation(Vector((-ctr.x, -ctr.y, -mn.z))))
    obj.data.transform(Matrix.Scale(s, 4))
    obj.matrix_world = Matrix.Identity(4)
    return (mx - mn) * s

def exportar(obj, carpeta, nombre):
    os.makedirs(carpeta, exist_ok=True)
    for o in bpy.context.scene.objects: o.select_set(False)
    obj.select_set(True); bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.fbx(filepath=os.path.join(carpeta, nombre + '.fbx'), use_selection=True,
                             object_types={'MESH'}, apply_scale_options='FBX_SCALE_ALL', axis_forward='-Z',
                             axis_up='Y', bake_space_transform=True, path_mode='STRIP', embed_textures=False,
                             mesh_smooth_type='FACE', use_mesh_modifiers=True, add_leaf_bones=False, bake_anim=False)

def nuevo_objeto(nombre, bm):
    me = bpy.data.meshes.new(nombre); bm.to_mesh(me); bm.free()
    ob = bpy.data.objects.new(nombre, me); bpy.context.scene.collection.objects.link(ob)
    return ob

def suavizar(obj):
    for p in obj.data.polygons: p.use_smooth = True

TABLA = []   # (zona, nombre, largo X, alto Y, ancho Z)

# ---------------------------------------------------------------- 1. modelos del equipo
def exportar_modelos():
    for nombre, (zona, rot, tam) in MODELOS.items():
        if SOLO and nombre not in SOLO: continue
        ruta = os.path.join(ENTRADA, nombre + '.blend')
        if not os.path.exists(ruta): print('FALTA', ruta); continue
        bpy.ops.wm.open_mainfile(filepath=ruta, load_ui=False)
        sc = bpy.context.scene
        mallas = [o for o in sc.objects if o.type == 'MESH' and not any(s in o.name.lower() for s in OMITIR) and not o.hide_render]
        dg = bpy.context.evaluated_depsgraph_get()
        bm = bmesh.new()
        imagen = None
        for o in mallas:
            ev = o.evaluated_get(dg); me = ev.to_mesh()
            me.transform(o.matrix_world)
            bm.from_mesh(me); ev.to_mesh_clear()
            for s in o.material_slots:
                if s.material and s.material.use_nodes and imagen is None:
                    nt = s.material.node_tree
                    bsdf = next((nd for nd in nt.nodes if nd.type == 'BSDF_PRINCIPLED'), None)
                    if bsdf and bsdf.inputs['Base Color'].is_linked:          # la imagen conectada al color base
                        src = bsdf.inputs['Base Color'].links[0].from_node
                        if src.type == 'TEX_IMAGE' and src.image: imagen = src.image
                    if imagen is None:
                        for nd in nt.nodes:
                            if nd.type == 'TEX_IMAGE' and nd.image and 'metallic' not in nd.image.name and 'normal' not in nd.image.name.lower():
                                imagen = nd.image
        ob = nuevo_objeto(nombre, bm)
        carpeta = os.path.join(SALIDA, zona)
        if len(ob.data.polygons) > 12000:          # mallas muy pesadas (tiburon, tortuga, barco) -> ~8k caras
            d = ob.modifiers.new('dec', 'DECIMATE'); d.ratio = max(0.08, 8000.0 / len(ob.data.polygons))
            ev = ob.evaluated_get(bpy.context.evaluated_depsgraph_get()); me2 = bpy.data.meshes.new_from_object(ev)
            ob.modifiers.clear(); ob.data = me2
        dims = normalizar(ob, rot, tam)
        archivo = nombre + '.jpg'
        if nombre in PINTADOS:
            uv_lateral(ob); textura_pintada(PINTADOS[nombre], archivo, carpeta)
        elif nombre == 'PezPayaso':
            Image.open(os.path.join(RAIZ, 'src', 'bin', 'models', 'PezPayaso.webp')).convert('RGB').save(os.path.join(carpeta, archivo), quality=92)
        elif imagen is not None and imagen.size[0] > 0:
            imagen_a_jpg(imagen, archivo, carpeta)
        elif nombre in TEXTURA_EXTERNA and os.path.exists(TEXTURA_EXTERNA[nombre]):
            Image.open(TEXTURA_EXTERNA[nombre]).convert('RGB').save(os.path.join(carpeta, archivo), quality=90)
        else:
            print('SIN TEXTURA', nombre); uv_lateral(ob); textura_pintada(((120, 120, 120), (200, 200, 200), None), archivo, carpeta)
        material_con_textura(ob, archivo)
        suavizar(ob)
        exportar(ob, carpeta, nombre)
        TABLA.append((zona, nombre, dims.x, dims.z, dims.y))
        print('EXPORTADO', zona, nombre, len(ob.data.polygons), 'caras', [round(d, 2) for d in dims])

# ---------------------------------------------------------------- 2. texturas procedurales
def ruido(h, w, escala, seed, oct=5):
    g = np.random.default_rng(seed); acc = np.zeros((h, w)); amp = 1.0; tot = 0
    for o in range(oct):
        n = max(2, int(escala * 2 ** o))
        base = g.random((n + 1, n + 1)); base[-1, :] = base[0, :]; base[:, -1] = base[:, 0]   # repetible
        im = Image.fromarray((base * 255).astype(np.uint8)).resize((w, h), Image.BICUBIC)
        acc += amp * np.asarray(im) / 255.0; tot += amp; amp *= 0.5
    return acc / tot

def tex_arena(c1, c2, seed, ondas=True):
    n = ruido(512, 512, 4, seed)
    y, x = np.mgrid[0:512, 0:512]
    rip = 0.5 + 0.5 * np.sin((x * 0.08 + n * 9.0)) if ondas else 0.5
    t = np.clip(0.55 * n + 0.45 * rip, 0, 1)[..., None]
    img = np.array(c1) * (1 - t) + np.array(c2) * t
    img += np.random.default_rng(seed).normal(0, 7, img.shape)
    return np.clip(img, 0, 255)

def tex_ruido(c1, c2, seed, escala=6, motas=10):
    t = ruido(512, 512, escala, seed)[..., None]
    img = np.array(c1) * (1 - t) + np.array(c2) * t
    img += np.random.default_rng(seed).normal(0, motas, img.shape)
    return np.clip(img, 0, 255)

def tex_oxido(seed):
    img = tex_ruido((55, 60, 58), (85, 88, 82), seed, 5, 6)
    o = ruido(512, 512, 8, seed + 5)
    mask = np.clip((o - 0.55) * 5, 0, 1)[..., None]
    img = img * (1 - mask) + np.array((120, 68, 35)) * mask
    for k in range(0, 512, 64): img[:, k:k + 2] *= 0.6                 # juntas de placas
    for k in range(0, 512, 128): img[k:k + 2, :] *= 0.6
    return np.clip(img, 0, 255)

def tex_hielo(seed):
    img = tex_ruido((200, 228, 240), (245, 252, 255), seed, 5, 4)
    gr = Image.new('L', (512, 512), 0); d = ImageDraw.Draw(gr); g = random.Random(seed)
    for _ in range(40):
        x, y = g.uniform(0, 512), g.uniform(0, 512); pts = [(x, y)]
        for _ in range(6): x += g.uniform(-40, 40); y += g.uniform(-40, 40); pts.append((x, y))
        d.line(pts, fill=255, width=2)
    gr = np.asarray(gr.filter(ImageFilter.GaussianBlur(1)))[..., None] / 255.0
    img = img * (1 - gr * 0.5) + np.array((120, 180, 215)) * gr * 0.5
    return np.clip(img, 0, 255)

# ---------------------------------------------------------------- 3. geometria procedural
def terreno(nombre, tam, n, altura, seed, tile, crater=None):
    g = np.random.default_rng(seed)
    bm = bmesh.new()
    bmesh.ops.create_grid(bm, x_segments=n, y_segments=n, size=tam / 2)
    hmap = ruido(n + 1, n + 1, 3, seed, 4)
    for v in bm.verts:
        i = int((v.co.x / tam + 0.5) * n); j = int((v.co.y / tam + 0.5) * n)
        h = (hmap[min(j, n), min(i, n)] - 0.5) * 2 * altura
        r = math.hypot(v.co.x, v.co.y)
        h += max(0, r - tam * 0.32) * 0.35                                  # bordes que suben (cuenco)
        if crater: h -= crater * math.exp(-(r / 9.0) ** 2)
        v.co.z = h
    ob = nuevo_objeto(nombre, bm); uv_caja(ob, 1.0 / tile); suavizar(ob)
    return ob

def roca(nombre, seed, radio=1.0, aplanar=0.6):
    bm = bmesh.new(); bmesh.ops.create_icosphere(bm, subdivisions=3, radius=radio)
    g = random.Random(seed); fases = [g.uniform(0, 6.28) for _ in range(6)]
    for v in bm.verts:
        d = v.co.normalized()
        k = 1 + 0.18 * math.sin(d.x * 3 + fases[0]) * math.sin(d.y * 4 + fases[1]) + 0.12 * math.sin(d.z * 7 + fases[2]) + g.uniform(-0.04, 0.04)
        v.co = d * radio * k; v.co.z *= aplanar
    ob = nuevo_objeto(nombre, bm); uv_caja(ob, 0.5); suavizar(ob)
    return ob

def coral_ramificado(nombre, seed, niveles=5, largo=0.5):
    g = random.Random(seed)
    verts = [(0, 0, 0)]; edges = []; radios = [0.09]
    def rama(i, d, l, r, n):
        if n == 0: return
        p = Vector(verts[i]) + d * l
        verts.append(tuple(p)); j = len(verts) - 1; edges.append((i, j)); radios.append(r * 0.8)
        for s in range(g.choice([2, 2, 3])):
            nd = (d + Vector((g.uniform(-0.7, 0.7), g.uniform(-0.7, 0.7), g.uniform(0.1, 0.6)))).normalized()
            rama(j, nd, l * g.uniform(0.7, 0.85), r * 0.78, n - 1)
    rama(0, Vector((0, 0, 1)), largo, 0.09, niveles)
    me = bpy.data.meshes.new(nombre); me.from_pydata(verts, edges, [])
    ob = bpy.data.objects.new(nombre, me); bpy.context.scene.collection.objects.link(ob)
    sk = ob.modifiers.new('skin', 'SKIN')
    for i, r in enumerate(radios):
        ob.data.skin_vertices[0].data[i].radius = (r, r)
    ob.data.skin_vertices[0].data[0].use_root = True
    sub = ob.modifiers.new('sub', 'SUBSURF'); sub.levels = 1; sub.render_levels = 1
    ev = ob.evaluated_get(bpy.context.evaluated_depsgraph_get()); me2 = bpy.data.meshes.new_from_object(ev)
    ob.modifiers.clear(); ob.data = me2; uv_caja(ob, 2.0); suavizar(ob)
    return ob

def coral_cerebro(nombre, seed, radio=0.8):
    bm = bmesh.new(); bmesh.ops.create_uvsphere(bm, u_segments=48, v_segments=24, radius=radio)
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if v.co.z < -0.05 * radio], context='VERTS')
    g = random.Random(seed); a, b = g.uniform(0, 6), g.uniform(0, 6)
    for v in bm.verts:
        d = v.co.normalized()
        surco = abs(math.sin(d.x * 9 + math.sin(d.y * 7 + a) * 1.5 + b))
        v.co = d * radio * (1 - 0.06 * (1 - surco)); v.co.z *= 0.75
    ob = nuevo_objeto(nombre, bm); uv_caja(ob, 1.5); suavizar(ob)
    return ob

def abanico(nombre, seed):
    g = random.Random(seed); verts = [(0, 0, 0)]; edges = []; radios = [0.05]
    def rama(i, ang, l, r, n):
        if n == 0: return
        p = Vector(verts[i]) + Vector((math.sin(ang), 0, math.cos(ang))) * l
        verts.append(tuple(p)); j = len(verts) - 1; edges.append((i, j)); radios.append(r)
        for s in (-1, 1): rama(j, ang + s * g.uniform(0.15, 0.4), l * 0.8, r * 0.8, n - 1)
    rama(0, 0, 0.35, 0.04, 7)
    me = bpy.data.meshes.new(nombre); me.from_pydata(verts, edges, [])
    ob = bpy.data.objects.new(nombre, me); bpy.context.scene.collection.objects.link(ob)
    ob.modifiers.new('skin', 'SKIN')
    for i, r in enumerate(radios): ob.data.skin_vertices[0].data[i].radius = (r, r * 0.4)
    ob.data.skin_vertices[0].data[0].use_root = True
    ev = ob.evaluated_get(bpy.context.evaluated_depsgraph_get()); me2 = bpy.data.meshes.new_from_object(ev)
    ob.modifiers.clear(); ob.data = me2; uv_caja(ob, 2.0); suavizar(ob)
    return ob

def alga(nombre, alto=4.0):
    bm = bmesh.new(); n = 24; w = 0.18
    filas = []
    for k in range(n + 1):
        t = k / n; z = t * alto; ang = t * 2.5; ancho = w * (1 - 0.6 * t)
        x = 0.15 * math.sin(t * 6)
        a = bm.verts.new((x - math.cos(ang) * ancho, -math.sin(ang) * ancho, z))
        b = bm.verts.new((x + math.cos(ang) * ancho, math.sin(ang) * ancho, z)); filas.append((a, b))
    for k in range(n): bm.faces.new((filas[k][0], filas[k][1], filas[k + 1][1], filas[k + 1][0]))
    ob = nuevo_objeto(nombre, bm); uv_lateral(ob); suavizar(ob)
    return ob

def plataforma_hielo(nombre, seed, radio=6.0, grosor=1.6):
    g = random.Random(seed); n = 14
    bm = bmesh.new()
    pts = []
    for k in range(n):
        a = k / n * 2 * math.pi; r = radio * g.uniform(0.65, 1.0)
        pts.append((math.cos(a) * r, math.sin(a) * r))
    abajo = [bm.verts.new((x, y, -grosor * g.uniform(0.7, 1.3))) for x, y in pts]
    arriba = [bm.verts.new((x, y, 0)) for x, y in pts]
    bm.faces.new(arriba); bm.faces.new(list(reversed(abajo)))
    for k in range(n):
        bm.faces.new((abajo[k], abajo[(k + 1) % n], arriba[(k + 1) % n], arriba[k]))
    bmesh.ops.subdivide_edges(bm, edges=bm.edges[:], cuts=2, use_grid_fill=True)
    bmesh.ops.triangulate(bm, faces=bm.faces[:])
    for v in bm.verts:
        if v.co.z < -0.1: v.co.z += g.uniform(-0.25, 0.25)
    ob = nuevo_objeto(nombre, bm); uv_caja(ob, 0.25)
    return ob

def iceberg(nombre, seed):
    ob = roca(nombre, seed, 4.0, 1.0)
    for v in ob.data.vertices: v.co.z *= 1.8 if v.co.z < 0 else 0.6; v.co.x *= 1.3
    uv_caja(ob, 0.25)
    return ob

def submarino(nombre):
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=40, v_segments=20, radius=1.0)
    for v in bm.verts: v.co.x *= 7.0; v.co.y *= 1.3; v.co.z *= 1.3       # casco alargado (frente en +X)
    t = bmesh.new(); bmesh.ops.create_cube(t, size=1.0)
    for v in t.verts: v.co.x = v.co.x * 2.6 + 1.2; v.co.y *= 0.7; v.co.z = v.co.z * 1.8 + 1.9
    tm = bpy.data.meshes.new('t'); t.to_mesh(tm); bm.from_mesh(tm)
    for (sx, sy, sz, px, py, pz) in [(1.2, 3.6, 0.12, -5.6, 0, 0), (1.2, 0.12, 2.6, -5.6, 0, 0), (0.9, 0.12, 0.25, 1.4, 0, 2.4)]:
        f = bmesh.new(); bmesh.ops.create_cube(f, size=1.0)
        for v in f.verts: v.co.x = v.co.x * sx + px; v.co.y = v.co.y * sy + py; v.co.z = v.co.z * sz + pz
        fm = bpy.data.meshes.new('f'); f.to_mesh(fm); bm.from_mesh(fm)
    ob = nuevo_objeto(nombre, bm); uv_caja(ob, 0.35)
    return ob

def columna(nombre, seed, alto=5.0, rota=False):
    g = random.Random(seed); bm = bmesh.new(); seg = 32; anillos = 10; r = 0.45
    filas = []
    for k in range(anillos + 1):
        z = alto * k / anillos; fila = []
        for s in range(seg):
            a = s / seg * 2 * math.pi; rr = r * (1 - 0.06 * (0.5 + 0.5 * math.cos(a * 16)))
            zz = z
            if rota and k == anillos: zz = z - g.uniform(0, 1.2)
            fila.append(bm.verts.new((math.cos(a) * rr, math.sin(a) * rr, zz)))
        filas.append(fila)
    for k in range(anillos):
        for s in range(seg):
            bm.faces.new((filas[k][s], filas[k][(s + 1) % seg], filas[k + 1][(s + 1) % seg], filas[k + 1][s]))
    bm.faces.new(list(reversed(filas[0]))); bm.faces.new(filas[-1])
    for (h, z0) in [(0.35, -0.35)] + ([] if rota else [(0.35, alto)]):
        c = bmesh.new(); bmesh.ops.create_cube(c, size=1.0)
        for v in c.verts: v.co.x *= 1.25; v.co.y *= 1.25; v.co.z = v.co.z * h + z0 + h / 2
        cm = bpy.data.meshes.new('c'); c.to_mesh(cm); bm.from_mesh(cm)
    ob = nuevo_objeto(nombre, bm); uv_caja(ob, 0.6)
    return ob

def bloque(nombre, seed):
    bm = bmesh.new(); bmesh.ops.create_cube(bm, size=1.0)
    bmesh.ops.subdivide_edges(bm, edges=bm.edges[:], cuts=3, use_grid_fill=True)
    g = random.Random(seed)
    for v in bm.verts: v.co.x *= 1.6; v.co.z *= 0.8; v.co += Vector((g.uniform(-0.04, 0.04),) * 3)
    ob = nuevo_objeto(nombre, bm); uv_caja(ob, 0.6)
    return ob

def burbuja(nombre):
    bm = bmesh.new(); bmesh.ops.create_icosphere(bm, subdivisions=2, radius=0.5)
    ob = nuevo_objeto(nombre, bm); uv_caja(ob, 1.0); suavizar(ob)
    return ob

def escenografia():
    piezas = []   # (zona, objeto, textura_np, nombre_textura, origen_en_base)
    # --- arrecifes
    limpiar_escena()
    piezas.append(('arrecifes', terreno('terreno_arrecifes', 110, 110, 1.4, 1, 4.0), tex_arena((205, 188, 145), (232, 220, 180), 1), 'arena.jpg', True))
    piezas.append(('arrecifes', roca('roca_arrecife', 3, 1.6, 0.55), tex_ruido((90, 95, 85), (140, 135, 115), 3), 'roca_arrecife.jpg', True))
    for i in range(2):
        piezas.append(('arrecifes', coral_ramificado(f'coral_rama{i + 1}', 10 + i), tex_ruido((215, 210, 205), (255, 252, 248), 11, 12, 14), 'coral.jpg', True))
    piezas.append(('arrecifes', coral_cerebro('coral_cerebro', 20), tex_ruido((200, 195, 190), (255, 250, 245), 21, 20, 10), 'coral_cerebro.jpg', True))
    piezas.append(('arrecifes', abanico('coral_abanico', 30), tex_ruido((215, 210, 205), (255, 252, 248), 31, 12, 8), 'coral.jpg', True))
    piezas.append(('arrecifes', alga('alga', 4.0), tex_ruido((60, 110, 40), (120, 150, 60), 41, 4, 8), 'alga.jpg', True))
    # --- aguas frias
    piezas.append(('aguas_frias', terreno('terreno_aguas_frias', 110, 110, 2.0, 2, 5.0), tex_ruido((70, 78, 88), (150, 160, 170), 2, 8, 16), 'grava.jpg', True))
    piezas.append(('aguas_frias', roca('roca_fria', 4, 1.8, 0.7), tex_ruido((60, 66, 75), (120, 130, 140), 4), 'roca_fria.jpg', True))
    for i in range(2):
        piezas.append(('aguas_frias', plataforma_hielo(f'hielo{i + 1}', 50 + i, 6.0 + i * 2), tex_hielo(50 + i), 'hielo.jpg', False))
    piezas.append(('aguas_frias', iceberg('iceberg', 60), tex_hielo(60), 'hielo.jpg', False))
    # --- naufragios
    piezas.append(('naufragios', terreno('terreno_naufragios', 110, 110, 1.6, 3, 4.0, crater=1.5), tex_arena((48, 55, 45), (78, 82, 66), 3, False), 'sedimento.jpg', True))
    piezas.append(('naufragios', submarino('submarino'), tex_oxido(7), 'oxido.jpg', False))
    piezas.append(('naufragios', columna('columna', 8), tex_ruido((150, 145, 125), (195, 190, 170), 8, 10, 10), 'piedra.jpg', True))
    piezas.append(('naufragios', columna('columna_rota', 9, 3.0, True), tex_ruido((150, 145, 125), (195, 190, 170), 8, 10, 10), 'piedra.jpg', True))
    piezas.append(('naufragios', bloque('bloque', 10), tex_ruido((150, 145, 125), (195, 190, 170), 8, 10, 10), 'piedra.jpg', False))
    piezas.append(('naufragios', roca('roca_naufragio', 5, 1.5, 0.6), tex_ruido((40, 48, 44), (85, 90, 80), 5), 'roca_naufragio.jpg', True))
    # --- comun
    piezas.append(('comun', burbuja('burbuja'), tex_ruido((215, 240, 255), (255, 255, 255), 9, 4, 2), 'burbuja.jpg', False))

    for zona, ob, tex, ntex, base in piezas:
        carpeta = os.path.join(SALIDA, zona)
        guardar_jpg(tex, ntex, carpeta)
        material_con_textura(ob, ntex)
        xs = [v.co for v in ob.data.vertices]
        dims = Vector((max(c.x for c in xs) - min(c.x for c in xs), max(c.y for c in xs) - min(c.y for c in xs), max(c.z for c in xs) - min(c.z for c in xs)))
        exportar(ob, carpeta, ob.name)
        TABLA.append((zona, ob.name, dims.x, dims.z, dims.y))
        print('GENERADO', zona, ob.name, len(ob.data.polygons), 'caras')

if __name__ == '__main__':
    exportar_modelos()
    if SOLO: raise SystemExit
    escenografia()
    with open(os.path.join(SALIDA, 'medidas.txt'), 'w') as f:
        f.write('# zona nombre largoX altoY anchoZ (metros, ejes de OpenGL)\n')
        for z, n, x, y, w in TABLA: f.write(f'{z} {n} {x:.3f} {y:.3f} {w:.3f}\n')
    print('LISTO')
