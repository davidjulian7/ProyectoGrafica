# Cofre de Meshy: viene abierto en una sola malla. Se separa la tapa, se cierra girandola sobre su
# bisagra (borde trasero de la caja) y se exportan caja y tapa por separado para animar la apertura.
#   blender --background --python herramientas/exporta_cofre.py
import os, sys, math
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bpy, bmesh
import numpy as np
from mathutils import Vector, Matrix
from exporta_zonas import RAIZ, SALIDA, material_con_textura, exportar, nuevo_objeto, suavizar, imagen_a_jpg
ANCHO = 1.3                                            # metros (eje X)
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=os.path.join(RAIZ, 'ModelosProyecto', 'cofre_meshy.glb'))
o = [o for o in bpy.context.scene.objects if o.type == 'MESH'][0]
o.data.transform(o.matrix_world); o.matrix_world.identity()
img = next(n.image for n in o.active_material.node_tree.nodes if n.type == 'TEX_IMAGE' and 'metallic' not in n.image.name)
vs = np.array([v.co[:] for v in o.data.vertices])
mn, mx = vs.min(0), vs.max(0)
s = ANCHO / (mx[0] - mn[0])
o.data.transform(Matrix.Scale(s, 4) @ Matrix.Translation(Vector((-(mn[0] + mx[0]) / 2, -(mn[1] + mx[1]) / 2, -mn[2]))))
vs = np.array([v.co[:] for v in o.data.vertices])
# borde superior de la caja: la pared delantera (y muy negativa) no tiene partes de la tapa abierta
# (en este modelo la tapa entreabierta empieza arriba del 60% de la altura; la pared delantera esta en y < -30%)
pared = vs[(vs[:, 1] < -0.30 * (vs[:, 1].max() - vs[:, 1].min()) / 2 * 2 * 0.5) & (vs[:, 2] < 0.60 * vs[:, 2].max())]
borde = float(pared[:, 2].max())
bm = bmesh.new(); bm.from_mesh(o.data)
tapa_caras = [f for f in bm.faces if f.calc_center_median().z > borde + 0.01]
# bisagra: borde trasero (+Y) a la altura del borde de la caja
cerca = vs[np.abs(vs[:, 2] - borde) < 0.04]
bisagra = Vector((0.0, float(cerca[:, 1].max()), borde))
corte = borde
bm_tapa = bm.copy(); bmesh.ops.delete(bm_tapa, geom=[f for f in bm_tapa.faces if f.calc_center_median().z <= corte], context='FACES')
bmesh.ops.delete(bm, geom=tapa_caras, context='FACES')
caja = nuevo_objeto('cofre', bm); tapa = nuevo_objeto('cofre_tapa', bm_tapa)
# cerrar la tapa: girarla sobre la bisagra hasta que su borde delantero toque el borde de la caja
tv = np.array([v.co[:] for v in tapa.data.vertices]) - np.array(bisagra[:])
cerrado = 0.0
for g in range(0, 170):
    a = math.radians(g)
    z = tv[:, 1] * math.sin(a) + tv[:, 2] * math.cos(a)
    y = tv[:, 1] * math.cos(a) - tv[:, 2] * math.sin(a)
    lejos = np.hypot(tv[:, 1], tv[:, 2]) > 0.25 * ANCHO      # sin contar la zona de la bisagra
    if z[lejos].min() < 0.004: break
    cerrado = a
tapa.data.transform(Matrix.Translation(bisagra) @ Matrix.Rotation(cerrado, 4, 'X') @ Matrix.Translation(-bisagra))
tapa.data.transform(Matrix.Translation(-bisagra))            # origen en la bisagra para girar al abrir
print(f'CORTE z={corte:.3f}  bisagra=({bisagra.y:.3f}, {bisagra.z:.3f})  tapa cerrada girando {math.degrees(cerrado):.0f} grados')
dest = os.path.join(SALIDA, 'naufragios')
imagen_a_jpg(img, 'cofre_meshy.jpg', dest)
for ob in (caja, tapa):
    suavizar(ob); material_con_textura(ob, 'cofre_meshy.jpg'); exportar(ob, dest, ob.name)
    print('EXPORTADO', ob.name, len(ob.data.polygons), 'caras')
with open(os.path.join(dest, 'cofre_bisagra.txt'), 'w') as f:
    f.write(f'{bisagra.y:.4f} {bisagra.z:.4f} {math.degrees(cerrado):.1f}\n')
