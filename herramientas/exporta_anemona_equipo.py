# Exporta la anemona modelada por Julian: une sus piezas, reduce triangulos, base en el origen y textura clara.
#   blender --background --python herramientas/exporta_anemona_equipo.py -- [ruta.blend]
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bpy, bmesh
from exporta_zonas import ENTRADA, SALIDA, material_con_textura, uv_caja, normalizar, exportar, nuevo_objeto, suavizar
ruta = sys.argv[sys.argv.index('--') + 1] if '--' in sys.argv else os.path.join(ENTRADA, 'anemona.blend')
bpy.ops.wm.open_mainfile(filepath=ruta, load_ui=False)
dg = bpy.context.evaluated_depsgraph_get(); bm = bmesh.new()
for o in [o for o in bpy.context.scene.objects if o.type == 'MESH' and not o.hide_render]:
    me = o.evaluated_get(dg).to_mesh(); me.transform(o.matrix_world); bm.from_mesh(me)
ob = nuevo_objeto('anemona', bm)
antes = len(ob.data.polygons)
d = ob.modifiers.new('dec', 'DECIMATE'); d.ratio = min(1.0, 5000.0 / max(1, antes))
ev = ob.evaluated_get(bpy.context.evaluated_depsgraph_get()); ob.data = bpy.data.meshes.new_from_object(ev); ob.modifiers.clear()
normalizar(ob, 0, 0.75, centrar=False)          # base en el origen, 75 cm
uv_caja(ob, 2.0); suavizar(ob)
material_con_textura(ob, 'anemona.jpg')
exportar(ob, os.path.join(SALIDA, 'arrecifes'), 'anemona')
print('EXPORTADO anemona de Julian:', antes, '->', len(ob.data.polygons), 'caras')
