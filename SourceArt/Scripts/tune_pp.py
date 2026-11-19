# Applies Saved/pp_params.json to MI_PP_EldoriaToon (fast look-dev iterations; final values go in art_direction.py).
import json
import os
import unreal
MEL = unreal.MaterialEditingLibrary
mi = unreal.load_asset("/Game/Project/Materials/PostProcess/MI_PP_EldoriaToon")
p = json.load(open(os.path.join(unreal.Paths.project_saved_dir(), "pp_params.json")))
for k, v in p.items():
    if isinstance(v, list):
        MEL.set_material_instance_vector_parameter_value(mi, k, unreal.LinearColor(v[0], v[1], v[2], 1.0))
    elif k.startswith("fog_"):
        for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
            if a.get_actor_label() == "ExponentialHeightFog":
                a.component.set_editor_property(k[4:], v)
    else:
        MEL.set_material_instance_scalar_parameter_value(mi, k, v)
MEL.update_material_instance(mi)
