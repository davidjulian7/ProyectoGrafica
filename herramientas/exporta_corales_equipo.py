# Exporta el coral ramificado de Santiago con la textura clara de coral (se tine y se blanquea en el shader).
#   blender --background --python herramientas/exporta_corales_equipo.py
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bpy, bmesh
from exporta_zonas import ENTRADA, SALIDA, material_con_textura, uv_caja, normalizar, exportar, nuevo_objeto, suavizar
bpy.ops.wm.open_mainfile(filepath=os.path.join(ENTRADA, 'CoralesRamificados.blend'), load_ui=False)
dg = bpy.context.evaluated_depsgraph_get(); bm = bmesh.new()
for o in [o for o in bpy.context.scene.objects if o.type == 'MESH' and not o.hide_render]:
    me = o.evaluated_get(dg).to_mesh(); me.transform(o.matrix_world); bm.from_mesh(me)
ob = nuevo_objeto('coral_rama_santiago', bm)
normalizar(ob, 0, 1.6, centrar=False)          # base del coral en el origen, 1.6 m de alto
uv_caja(ob, 2.0); suavizar(ob)
material_con_textura(ob, 'coral.jpg')           # misma textura clara que los demas corales
exportar(ob, os.path.join(SALIDA, 'arrecifes'), ob.name)
print('EXPORTADO coral_rama_santiago', len(ob.data.polygons), 'caras')
