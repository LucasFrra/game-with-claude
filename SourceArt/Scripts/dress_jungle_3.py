# Project Eldoria - jungle visual pass 3: foliage master material (two-sided lighting, gradient, wind).
import os
import sys
import importlib

sys.path.append(os.path.dirname(__file__))
import unreal
import eldoria_lib as L
importlib.reload(L)

REPORT = os.path.join(unreal.Paths.project_saved_dir(), "Dress3Report.txt")
log = []
FOLDER = "/Game/Project/Materials/Foliage"


def tex(model, name):
    return L.find_asset("/Game/Project/Art/Nature/" + model, name, unreal.Texture2D)


def run():
    parent = L.build_foliage_material()
    log.append("master: %s" % parent.get_path_name())

    mis = {
        "Grass": L.make_instance("MI_Foliage_Grass", parent, FOLDER,
                                 {"BaseTex": tex("Grass_Common_Short", "Grass")},
                                 {"RootDarkness": 0.5, "UpNormal": 1.0, "WindStrength": 0.35, "WindSpeed": 0.4, "Roughness": 0.9},
                                 {"Tint": (0.17, 0.33, 0.05), "TipTint": (1.55, 1.45, 0.85)}),
        "Leaves_NormalTree": L.make_instance("MI_Foliage_TreeLeaves", parent, FOLDER,
                                             {"BaseTex": tex("CommonTree_1", "Leaves_NormalTree_C")},
                                             {"RootDarkness": 1.0, "UpNormal": 0.45, "WindStrength": 0.06, "WindSpeed": 0.25},
                                             {"Tint": (0.9, 0.95, 0.85), "TipTint": (1, 1, 1)}),
        "Leaves": L.make_instance("MI_Foliage_PlantLeaves", parent, FOLDER,
                                  {"BaseTex": tex("Plant_1_Big", "Leaves")},
                                  {"RootDarkness": 0.55, "UpNormal": 0.6, "WindStrength": 0.15, "WindSpeed": 0.3},
                                  {"Tint": (0.85, 0.95, 0.8), "TipTint": (1.1, 1.1, 1.0)}),
        "Flowers": L.make_instance("MI_Foliage_Flowers", parent, FOLDER,
                                   {"BaseTex": tex("Flower_3_Group", "Flowers")},
                                   {"RootDarkness": 1.0, "UpNormal": 0.6, "WindStrength": 0.2, "WindSpeed": 0.35},
                                   {"Tint": (1, 1, 1), "TipTint": (1, 1, 1)}),
    }
    log.append("instances: %s" % {k: bool(v) for k, v in mis.items()})

    meshes = ["CommonTree_%d" % i for i in range(1, 6)] + ["Plant_1_Big", "Plant_7_Big", "Fern_1", "Bush_Common_Flowers",
              "Grass_Common_Short", "Grass_Common_Tall", "Grass_Wispy_Short", "Grass_Wispy_Tall",
              "Flower_3_Group", "Flower_4_Group", "Clover_1", "Clover_2"]
    assets = [L.nature_mesh(m) for m in meshes] + [unreal.load_asset("/Game/Project/Environment/Trees/SM_YoungTree_Physics")]
    changed = 0
    for mesh in assets:
        if not mesh:
            continue
        for index, slot in enumerate(mesh.get_editor_property("static_materials")):
            slot_name = str(slot.get_editor_property("material_slot_name"))
            mat = slot.get_editor_property("material_interface")
            mat_name = mat.get_name() if mat else ""
            key = next((k for k in mis if slot_name == k or mat_name == k or mat_name.startswith(k + "_") or mat_name.endswith(k)), None)
            # Leaves_NormalTree must win over Leaves
            if slot_name.startswith("Leaves_NormalTree") or mat_name.startswith("Leaves_NormalTree"):
                key = "Leaves_NormalTree"
            if key:
                mesh.set_material(index, mis[key])
                changed += 1
            else:
                log.append("  kept %s slot %s (%s)" % (mesh.get_name(), slot_name, mat_name))
        L.EAL.save_loaded_asset(mesh)
    log.append("slots changed: %d" % changed)


try:
    run()
except Exception as exc:
    log.append("[FATAL] %r" % exc)
with open(REPORT, "w") as f:
    f.write("\n".join(log))
