# Integra modelos hechos por el equipo (Meshy y Blender) al formato del proyecto:
# une piezas, hornea transformaciones, reduce triangulos, orienta, escala y exporta FBX + textura .jpg
#   blender --background --python herramientas/exporta_modelos_equipo.py
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bpy, bmesh
from exporta_zonas import RAIZ, ENTRADA, SALIDA, material_con_textura, uv_caja, uv_lateral, normalizar, exportar, nuevo_objeto, suavizar, imagen_a_jpg
from exporta_zona3_extras import uv_cilindro

DESCARGAS = os.path.join(RAIZ, 'ModelosProyecto')
# nombre final: (archivo origen, carpeta, rotacion Z, tamano en m, origen en la base?, max caras, uv, textura existente)
MODELOS = {
    'pulpo':        ('pulpo_meshy.glb',      'arrecifes',  0, 1.1,  True,  4000,  None,       None),
    'submarino':    ('submarino_meshy.glb',  'naufragios', 0, 14.0, False, 12000, None,       None),
    'medusa':       ('medusa_meshy.glb',     'naufragios', 0, 1.2,  False, 6000,  None,       None),   # Meshy (antes Medusa.blend)
    'estrella_mar': ('EstrellaDeMar.blend',  'naufragios', 0, 0.3,  True,  4000,  'caja',     'estrella.jpg'),
    'botella':      ('BotellaPlastico.blend','naufragios', 0, 0.32, True,  3000,  'cilindro', 'botella.jpg'),
    # ruinas (Meshy): base en el origen para apoyarse en el fondo y para que la columna caiga sobre su base
    'columna':       ('ruina_columna.glb',  'naufragios', 0, 5.0,  True,  3000,  None,       None),
    'ruinas_arcada': ('ruina_arcada.glb',   'naufragios', 0, 13.0, True,  3000,  None,       None),
    'ruinas_balcon': ('ruina_balcon.glb',   'naufragios', 0, 7.5,  True,  3000,  None,       None),
}
solo = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else list(MODELOS)
for nombre in solo:
    src, carpeta, rot, tam, base, maxc, uv, tex = MODELOS[nombre]
    ruta = os.path.join(DESCARGAS, src)
    if src.endswith('.glb'):
        bpy.ops.wm.read_factory_settings(use_empty=True); bpy.ops.import_scene.gltf(filepath=ruta)
    else:
        bpy.ops.wm.open_mainfile(filepath=ruta, load_ui=False)
    dg = bpy.context.evaluated_depsgraph_get(); bm = bmesh.new(); imagen = None
    for o in [o for o in bpy.context.scene.objects if o.type == 'MESH' and not o.hide_render]:
        me = o.evaluated_get(dg).to_mesh(); me.transform(o.matrix_world); bm.from_mesh(me)
        for s in o.material_slots:
            if imagen is None and s.material and s.material.use_nodes:
                b = next((n for n in s.material.node_tree.nodes if n.type == 'BSDF_PRINCIPLED'), None)
                if b and b.inputs['Base Color'].is_linked and b.inputs['Base Color'].links[0].from_node.type == 'TEX_IMAGE':
                    imagen = b.inputs['Base Color'].links[0].from_node.image
    ob = nuevo_objeto(nombre, bm); antes = len(ob.data.polygons)
    if antes > maxc:
        d = ob.modifiers.new('dec', 'DECIMATE'); d.ratio = maxc / antes
        ev = ob.evaluated_get(bpy.context.evaluated_depsgraph_get()); ob.data = bpy.data.meshes.new_from_object(ev); ob.modifiers.clear()
    normalizar(ob, rot, tam, centrar=not base)
    if uv == 'caja': uv_caja(ob, 6.0)
    elif uv == 'lateral': uv_lateral(ob)
    elif uv == 'cilindro': uv_cilindro(ob)
    suavizar(ob)
    dest = os.path.join(SALIDA, carpeta); archivo = tex or (nombre + '.jpg')
    if tex is None and imagen is not None and imagen.size[0] > 0: imagen_a_jpg(imagen, archivo, dest)
    material_con_textura(ob, archivo); exportar(ob, dest, nombre)
    print('EXPORTADO', nombre, antes, '->', len(ob.data.polygons), 'caras ->', carpeta)
