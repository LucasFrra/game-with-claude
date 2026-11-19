# Project Eldoria - J1b: imports the BOTW-like vegetation/backdrop (blender_botw_assets.py), builds their
# materials and places the distant peaks. Run BEFORE dress_jungle_2.py (which scatters them).
import os
import sys
import importlib

sys.path.append(os.path.dirname(__file__))
import unreal
import eldoria_lib as L
importlib.reload(L)

MEL = unreal.MaterialEditingLibrary
GEN = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "Generated"))
VEG_DIR = "/Game/Project/Environment/Stylized"
BACK_DIR = "/Game/Project/Environment/Backdrop"
MAT_DIR = "/Game/Project/Materials/Stylized"
log = []


def import_files(folder, dest, exts):
    tasks = []
    for f in sorted(os.listdir(folder)):
        if f.lower().endswith(exts):
            t = unreal.AssetImportTask()
            t.filename = os.path.join(folder, f)
            t.destination_path = dest
            t.automated = True
            t.replace_existing = True
            t.save = True
            tasks.append(t)
    L.TOOLS.import_asset_tasks(tasks)
    found = {}
    for p in L.EAL.list_assets(dest, recursive=True):
        a = unreal.load_asset(p)
        found[a.get_name()] = a
    return found


def distant_material():
    mat = L.new_material("M_Eldoria_Distant")
    vc = L.expr(mat, unreal.MaterialExpressionVertexColor, -900, 0)
    rock = L.param_vector(mat, "Rock", (0.20, 0.25, 0.27), -900, -200)
    snow = L.param_vector(mat, "Snow", (0.88, 0.92, 0.97), -900, -100)
    col = L.lerp(mat, rock, snow, vc, -600, -100, alpha_out="R")
    MEL.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(L.scalar(mat, 1.0, -600, 100), "", unreal.MaterialProperty.MP_ROUGHNESS)
    return L.finish(mat)


def run():
    tex = import_files(os.path.join(GEN, "Foliage"), MAT_DIR + "/Textures", (".png",))
    leaf_tex = tex.get("T_LeafCluster")
    log.append("leaf texture %s" % bool(leaf_tex))
    veg = import_files(os.path.join(GEN, "Foliage"), VEG_DIR, (".glb",))
    back = import_files(os.path.join(GEN, "Backdrop"), BACK_DIR, (".glb",))
    log.append("veg %s" % sorted(k for k, v in veg.items() if isinstance(v, unreal.StaticMesh)))
    log.append("backdrop %s" % sorted(k for k, v in back.items() if isinstance(v, unreal.StaticMesh)))

    foliage = unreal.load_asset("/Game/Project/Materials/Master/M_Eldoria_Foliage")
    skin = unreal.load_asset("/Game/Project/Materials/Master/M_Eldoria_CreatureSkin")
    white = unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture")
    leaves = L.make_instance("MI_Leaves_Cards", foliage, MAT_DIR, {"BaseTex": leaf_tex},
                             {"RootDarkness": 0.5, "UpNormal": 0.0, "WindStrength": 0.06, "WindSpeed": 0.25, "Roughness": 0.9},
                             {"Tint": (0.33, 0.56, 0.10), "TipTint": (1.12, 1.08, 0.85)})
    core = L.make_instance("MI_Leaves_Core", foliage, MAT_DIR, {"BaseTex": white},
                           {"RootDarkness": 0.5, "UpNormal": 0.0, "WindStrength": 0.0, "Roughness": 0.9},
                           {"Tint": (0.27, 0.48, 0.09), "TipTint": (1.05, 1.02, 0.85)})
    bark = L.make_instance("MI_Bark_Stylized", skin, MAT_DIR, None, None,
                           {"SkinA": (0.16, 0.10, 0.06), "SkinB": (0.30, 0.20, 0.12), "Moss": (0.14, 0.28, 0.05)})
    grass = L.make_instance("MI_Grass_Blades", foliage, MAT_DIR, {"BaseTex": white},
                            {"RootDarkness": 0.72, "UpNormal": 1.0, "WindStrength": 0.4, "WindSpeed": 0.45, "Roughness": 0.95},
                            {"Tint": (0.24, 0.40, 0.07), "TipTint": (1.5, 1.42, 0.95)})
    distant = distant_material()
    slots = {"Bark": bark, "Leaves": leaves, "LeavesCore": core, "Grass": grass, "Mountain": distant}

    for mesh in list(veg.values()) + list(back.values()):
        if not isinstance(mesh, unreal.StaticMesh):
            continue
        for i, slot in enumerate(mesh.get_editor_property("static_materials")):
            name = str(slot.get_editor_property("material_slot_name"))
            key = next((k for k in slots if name == k or name.startswith(k)), None)
            if key == "Leaves" and name.startswith("LeavesCore"):
                key = "LeavesCore"
            if key:
                mesh.set_material(i, slots[key])
        L.EAL.save_loaded_asset(mesh)
    log.append("materials assigned")

    # Trunk-only collision: the crown stays soft/passable, the trunk is solid (mesh units: cm)
    trunks = {"SM_Tree_Broadleaf_A": (100.0, 620.0, 330.0), "SM_Tree_Broadleaf_B": (62.0, 380.0, 210.0)}
    for name, (radius, length, z) in trunks.items():
        mesh = veg.get(name)
        if not mesh:
            continue
        body = mesh.get_editor_property("body_setup")
        geom = body.get_editor_property("agg_geom")
        cap = unreal.KSphylElem()
        cap.set_editor_property("center", unreal.Vector(0, 0, z))
        cap.set_editor_property("radius", radius)
        cap.set_editor_property("length", length)
        for k in ("box_elems", "sphere_elems", "convex_elems"):
            geom.set_editor_property(k, [])
        geom.set_editor_property("sphyl_elems", [cap])
        body.set_editor_property("agg_geom", geom)
        body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AS_COMPLEX)
        mesh.set_editor_property("body_setup", body)
        L.EAL.save_loaded_asset(mesh)
    log.append("trunk capsules")

    # Distant peaks: far beyond the cliffs, tall enough to show above them; fog turns them blue
    L.destroy(["Backdrop_Peak_"])
    spots = [("SM_Peak_1", 2600, 1500, 0), ("SM_Peak_0", 2000, -2300, 40), ("SM_Peak_3", 3200, -500, 120),
             ("SM_Peak_2", -1600, 2800, 200), ("SM_Peak_1", -2400, -2600, 300), ("SM_Peak_3", 800, 3400, 80)]
    for k, (name, x, y, yaw) in enumerate(spots):
        mesh = back.get(name)
        if mesh:
            a = L.spawn_mesh(mesh, "Backdrop_Peak_%d" % k, (x * 100, y * 100, -15000), yaw, 1.0, "Backdrop", collision=False)
            a.static_mesh_component.set_editor_property("cast_shadow", False)
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    log.append("peaks placed")


try:
    run()
except Exception as exc:
    log.append("[FATAL] %r" % exc)
with open(os.path.join(unreal.Paths.project_saved_dir(), "BotwVegetation.txt"), "w") as f:
    f.write("\n".join(log))
