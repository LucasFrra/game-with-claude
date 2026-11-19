# Project Eldoria - J1 palette pass (BOTW-like): terrain, grass matched to the ground, warmer leaves,
# slate-stone skin for Tharum. Re-runnable. See docs/ART_DIRECTION.md.
import os
import sys
import importlib

sys.path.append(os.path.dirname(__file__))
import unreal
import eldoria_lib as L
importlib.reload(L)

MEL = unreal.MaterialEditingLibrary
log = []

PALETTE = {
    "grass": {"Tint": (0.55, 0.76, 0.32), "TipTint": (1.3, 1.25, 0.85), "RootDarkness": 0.72},
    "tree_leaves": {"Tint": (1.0, 1.05, 0.72), "TipTint": (1.08, 1.06, 0.9)},
    "plant_leaves": {"Tint": (0.95, 1.0, 0.7), "TipTint": (1.12, 1.1, 0.9)},
    "tharum": {"SkinA": (0.16, 0.165, 0.18), "SkinB": (0.30, 0.30, 0.31), "Moss": (0.13, 0.27, 0.05)},
}


def set_vectors(mi, values):
    for k, v in values.items():
        if isinstance(v, tuple):
            MEL.set_material_instance_vector_parameter_value(mi, k, unreal.LinearColor(v[0], v[1], v[2], 1.0))
        else:
            MEL.set_material_instance_scalar_parameter_value(mi, k, v)
    MEL.update_material_instance(mi)
    L.EAL.save_loaded_asset(mi)


def run():
    L.build_terrain_material()
    log.append("terrain material rebuilt")
    for name, key in [("MI_Foliage_Grass", "grass"), ("MI_Foliage_TreeLeaves", "tree_leaves"), ("MI_Foliage_PlantLeaves", "plant_leaves")]:
        mi = unreal.load_asset("/Game/Project/Materials/Foliage/" + name)
        if mi:
            set_vectors(mi, PALETTE[key])
            log.append("%s updated" % name)
    skin_parent = unreal.load_asset("/Game/Project/Materials/Master/M_Eldoria_CreatureSkin")
    skin = L.make_instance("MI_Tharum_SkinStone", skin_parent, "/Game/Project/Materials/Creatures", None, None, PALETTE["tharum"])
    bp = unreal.load_asset("/Game/Project/Bosses/Tharum/BP_Tharum")
    unreal.get_default_object(bp.generated_class()).set_editor_property("skin_material", skin)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    L.EAL.save_loaded_asset(bp)
    for a in L.EAS.get_all_level_actors():
        if isinstance(a, unreal.EldoriaTharum):
            a.set_editor_property("skin_material", skin)
            a.rerun_construction_scripts() if hasattr(a, "rerun_construction_scripts") else None
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    log.append("tharum skin %s" % skin.get_path_name())


try:
    run()
except Exception as exc:
    log.append("[FATAL] %r" % exc)
with open(os.path.join(unreal.Paths.project_saved_dir(), "Palette.txt"), "w") as f:
    f.write("\n".join(log))
