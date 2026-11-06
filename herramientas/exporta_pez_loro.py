"""Exporta PezLoro.blend a FBX y textura JPG para el juego."""
import os
import bpy
import bmesh
import numpy as np
from mathutils import Matrix, Vector

RAIZ = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
ORIGEN = os.path.join(RAIZ, 'ModelosProyecto', 'PezLoro.blend')
SALIDA = os.path.join(RAIZ, 'src', 'bin', 'models', 'zonas', 'arrecifes')
NOMBRE = 'PezLoro'
os.makedirs(SALIDA, exist_ok=True)

bpy.ops.wm.open_mainfile(filepath=ORIGEN, load_ui=False)
depsgraph = bpy.context.evaluated_depsgraph_get()
bm = bmesh.new()
for obj in [o for o in bpy.context.scene.objects if o.type == 'MESH' and not o.hide_render]:
    mesh = obj.evaluated_get(depsgraph).to_mesh()
    mesh.transform(obj.matrix_world)
    bm.from_mesh(mesh)
    obj.evaluated_get(depsgraph).to_mesh_clear()

mesh = bpy.data.meshes.new(NOMBRE)
bm.to_mesh(mesh)
bm.free()
obj = bpy.data.objects.new(NOMBRE, mesh)
bpy.context.scene.collection.objects.link(obj)

# Alinea cabeza a +X y normaliza a 0.7 m, como el resto de peces del proyecto.
mesh.transform(Matrix.Rotation(np.pi, 4, 'Z'))
coords = [v.co.copy() for v in mesh.vertices]
mn = Vector(tuple(min(v[i] for v in coords) for i in range(3)))
mx = Vector(tuple(max(v[i] for v in coords) for i in range(3)))
center = (mn + mx) * 0.5
scale = 0.7 / max(mx - mn)
dimensiones = (mx - mn) * scale
mesh.transform(Matrix.Translation(-center))
mesh.transform(Matrix.Scale(scale, 4))

# UV lateral para textura continua en piezas combinadas.
uv = mesh.uv_layers.active or mesh.uv_layers.new(name='UVMap')
xs = [v.co.x for v in mesh.vertices]
zs = [v.co.z for v in mesh.vertices]
x0, x1, z0, z1 = min(xs), max(xs), min(zs), max(zs)
for poly in mesh.polygons:
    for loop_idx in poly.loop_indices:
        vi = mesh.loops[loop_idx].vertex_index
        v = mesh.vertices[vi].co
        uv.data[loop_idx].uv = ((v.x - x0) / (x1 - x0), (v.z - z0) / (z1 - z0))

# Textura de escamas turquesa y vientre dorado; Blender la escribe como JPG.
w, h = 512, 256
image = bpy.data.images.new('PezLoro.jpg', width=w, height=h, alpha=False)
y, x = np.mgrid[0:h, 0:w].astype(np.float32)
u, v = x / (w - 1), y / (h - 1)
top = np.array([25.0, 105.0, 115.0])
belly = np.array([244.0, 198.0, 78.0])
t = np.clip((v - 0.35) / 0.4, 0.0, 1.0)[..., None]
rgb = belly * (1.0 - t) + top * t
stripe = np.clip((np.cos((u * 7.0 + v * 0.18) * np.pi * 2.0) - 0.35) * 0.65, 0.0, 1.0)[..., None]
teal = np.array([32.0, 165.0, 145.0])
rgb = rgb * (1.0 - stripe * 0.7) + teal * (stripe * 0.7)
tail = np.clip((0.16 - u) / 0.16, 0.0, 1.0)[..., None]
rgb = rgb * (1.0 - tail * 0.75) + np.array([32.0, 165.0, 145.0]) * (tail * 0.75)
pixels = np.empty((h, w, 4), dtype=np.float32)
pixels[:, :, :3] = np.clip(rgb / 255.0, 0.0, 1.0)
pixels[:, :, 3] = 1.0
image.pixels.foreach_set(pixels.ravel())
image.filepath_raw = os.path.join(SALIDA, 'PezLoro.jpg')
image.file_format = 'JPEG'
image.save()

mat = bpy.data.materials.new('M_PezLoro')
mat.use_nodes = True
nodes = mat.node_tree.nodes
tex = nodes.new('ShaderNodeTexImage')
tex.image = image
mat.node_tree.links.new(tex.outputs['Color'], nodes.get('Principled BSDF').inputs['Base Color'])
mesh.materials.clear()
mesh.materials.append(mat)
for poly in mesh.polygons:
    poly.use_smooth = True

os.makedirs(SALIDA, exist_ok=True)
for item in bpy.context.scene.objects:
    item.select_set(False)
obj.select_set(True)
bpy.context.view_layer.objects.active = obj
bpy.ops.export_scene.fbx(filepath=os.path.join(SALIDA, NOMBRE + '.fbx'), use_selection=True,
    object_types={'MESH'}, apply_scale_options='FBX_SCALE_ALL', axis_forward='-Z', axis_up='Y',
    bake_space_transform=True, path_mode='STRIP', embed_textures=False,
    mesh_smooth_type='FACE', use_mesh_modifiers=True, add_leaf_bones=False, bake_anim=False)
print('EXPORTADO', NOMBRE, len(mesh.polygons), 'caras; dimensiones X/Y/Z:', [round(d, 3) for d in dimensiones])
