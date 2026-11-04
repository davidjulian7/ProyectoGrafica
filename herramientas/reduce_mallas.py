# Reduce los triangulos de los modelos mas pesados (Decimate) sin cambiar su aspecto.
#   blender --background --python herramientas/reduce_mallas.py
import bpy, os
RAIZ = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
Z = os.path.join(RAIZ, 'src', 'bin', 'models', 'zonas')
OBJETIVOS = {   # archivo: fraccion de triangulos que se conserva
    'arrecifes/coral_rama1': 0.30, 'arrecifes/coral_rama2': 0.40, 'arrecifes/coral_abanico': 0.35,
    'arrecifes/anemona': 0.40, 'arrecifes/pulpo': 0.45, 'arrecifes/langosta': 0.6,
    'naufragios/red_fantasma': 0.30, 'naufragios/medusa': 0.30, 'naufragios/timon': 0.40,
    'naufragios/ancla': 0.6,
}
for rel, ratio in OBJETIVOS.items():
    ruta = os.path.join(Z, rel + '.fbx')
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=ruta)
    antes = despues = 0
    for o in [o for o in bpy.context.scene.objects if o.type == 'MESH']:
        o.data.transform(o.matrix_world); o.matrix_world.identity()   # la importacion agrega una rotacion: se hornea
        antes += sum(len(p.vertices) - 2 for p in o.data.polygons)
        m = o.modifiers.new('dec', 'DECIMATE'); m.ratio = ratio
        bpy.context.view_layer.objects.active = o
        bpy.ops.object.modifier_apply(modifier='dec')
        despues += sum(len(p.vertices) - 2 for p in o.data.polygons)
        o.select_set(True)
    bpy.ops.export_scene.fbx(filepath=ruta, use_selection=True, object_types={'MESH'}, apply_scale_options='FBX_SCALE_ALL',
                             axis_forward='-Z', axis_up='Y', bake_space_transform=True, path_mode='STRIP', embed_textures=False,
                             mesh_smooth_type='FACE', add_leaf_bones=False, bake_anim=False)
    print(f'REDUCIDO {rel}: {antes} -> {despues} triangulos')
