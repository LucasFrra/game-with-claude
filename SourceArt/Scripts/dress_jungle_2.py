# Project Eldoria - jungle visual pass 2: terrain refresh, lighting, vegetation, rocks, real props.
import json
import os
import random
import sys
import importlib

sys.path.append(os.path.dirname(__file__))
import unreal
import eldoria_lib as L
importlib.reload(L)

HERE = os.path.dirname(__file__)
TERRAIN_DIR = os.path.normpath(os.path.join(HERE, "..", "Generated", "Terrain"))
REPORT = os.path.join(unreal.Paths.project_saved_dir(), "Dress2Report.txt")
log = []
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
SMES = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)


def step(name):
    def deco(fn):
        def wrapper(*a, **k):
            try:
                r = fn(*a, **k)
                log.append("[ok] %s %s" % (name, "" if r is None else r))
            except Exception as exc:
                log.append("[FAIL] %s: %r" % (name, exc))
        return wrapper
    return deco


@step("terrain mesh + material")
def terrain():
    task = unreal.AssetImportTask()
    task.filename = os.path.join(TERRAIN_DIR, "SM_Terrain_TharumJungle.gltf")
    task.destination_path = "/Game/Project/Environment/Terrain"
    task.automated = True
    task.replace_existing = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    mesh = unreal.load_asset("/Game/Project/Environment/Terrain/SM_Terrain_TharumJungle/StaticMeshes/SM_Terrain_TharumJungle")
    body = mesh.get_editor_property("body_setup")
    body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    mat = L.build_terrain_material()
    mesh.set_material(0, mat)
    L.EAL.save_loaded_asset(mesh)
    for a in L.find_actors(["Terrain_TharumJungle"]):
        a.static_mesh_component.set_material(0, mat)
    return mesh.get_path_name()


@step("lighting + post")
def lighting():
    for a in L.EAS.get_all_level_actors():
        label = a.get_actor_label()
        if label == "DirectionalLight":
            a.set_actor_rotation(unreal.Rotator(pitch=-62.0, yaw=12.0, roll=0.0), False)   # along the arrival corridor
            c = a.light_component
            c.set_editor_property("intensity", 6.0)
            c.set_editor_property("temperature", 5400.0)
            c.set_editor_property("use_temperature", True)
            c.set_editor_property("light_source_angle", 1.5)
        elif label == "ExponentialHeightFog":
            c = a.component
            c.set_editor_property("fog_density", 0.018)
            c.set_editor_property("fog_height_falloff", 0.06)
            c.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(0.30, 0.42, 0.55, 1.0))
            c.set_editor_property("directional_inscattering_luminance", unreal.LinearColor(0.55, 0.45, 0.30, 1.0))
            c.set_editor_property("start_distance", 1500.0)
            loc = a.get_actor_location()
            a.set_actor_location(unreal.Vector(loc.x, loc.y, -500.0), False, False)
        elif label == "SkyLight":
            a.light_component.set_editor_property("intensity", 2.0)
        elif label == "PostProcessVolume":
            s = a.get_editor_property("settings")
            for prop, value in [("bloom_intensity", 0.35), ("vignette_intensity", 0.3), ("auto_exposure_bias", 0.35),
                                ("color_saturation", unreal.Vector4(1.06, 1.06, 1.06, 1.0)),
                                ("color_contrast", unreal.Vector4(1.04, 1.04, 1.04, 1.0)),
                                ("ambient_occlusion_intensity", 0.6), ("white_temp", 6300.0)]:
                s.set_editor_property("override_" + prop, True)
                s.set_editor_property(prop, value)
            a.set_editor_property("settings", s)


VEGETATION_NO_COLLISION = ["CommonTree_1", "CommonTree_2", "CommonTree_3", "CommonTree_4", "CommonTree_5",
                           "Plant_1_Big", "Plant_7_Big", "Fern_1", "Bush_Common_Flowers", "Grass_Common_Short",
                           "Grass_Common_Tall", "Grass_Wispy_Short", "Grass_Wispy_Tall", "Flower_3_Group",
                           "Flower_4_Group", "Clover_1", "Clover_2", "Mushroom_Common"] + ["Pebble_Round_%d" % i for i in range(1, 6)]


@step("young tree physics mesh")
def young_tree_mesh():
    # Keep a copy WITH collision for the young trees that topple physically
    path = "/Game/Project/Environment/Trees/SM_YoungTree_Physics"
    if not L.EAL.does_asset_exist(path):
        L.ensure_dir("/Game/Project/Environment/Trees")
        L.EAL.duplicate_asset("/Game/Project/Art/Nature/CommonTree_2/StaticMeshes/CommonTree_2", path)
    return path


@step("vegetation collision removed")
def vegetation_collision():
    n = 0
    for name in VEGETATION_NO_COLLISION:
        mesh = L.nature_mesh(name)
        if mesh:
            SMES.remove_collisions(mesh)
            L.EAL.save_loaded_asset(mesh)
            n += 1
    return n


# BOTW-like replacements (blender_botw_assets.py + botw_vegetation.py): Quaternius name -> (asset, scale factor, copies)
STYLIZED = {
    "CommonTree_1": ("SM_Tree_Broadleaf_A", 1.0 / 3.7, 1), "CommonTree_2": ("SM_Tree_Broadleaf_B", 1.5 / 3.7, 1),
    "CommonTree_3": ("SM_Tree_Broadleaf_A", 0.9 / 3.7, 1), "CommonTree_4": ("SM_Tree_Broadleaf_B", 1.4 / 3.7, 1),
    "CommonTree_5": ("SM_Tree_Broadleaf_A", 1.1 / 3.7, 1),
    "Plant_7_Big": ("SM_Bush_Round_A", 0.55, 1),
    "Grass_Common_Short": ("SM_Grass_Clump_A", 2.6, 3), "Grass_Wispy_Short": ("SM_Grass_Clump_A", 2.4, 3),
    "Grass_Common_Tall": ("SM_Grass_Clump_B", 2.0, 2), "Grass_Wispy_Tall": ("SM_Grass_Clump_B", 1.9, 2),
}


def veg_mesh(name):
    if name in STYLIZED:
        mesh = L.find_asset("/Game/Project/Environment/Stylized", STYLIZED[name][0], unreal.StaticMesh)
        if mesh:
            return mesh, STYLIZED[name][1], STYLIZED[name][2]
    return L.nature_mesh(name), 1.0, 1


def foliage_type(name):
    L.ensure_dir("/Game/Project/Environment/Foliage")
    path = "/Game/Project/Environment/Foliage/FT_" + name
    ft = unreal.load_asset(path) if L.EAL.does_asset_exist(path) else None
    if ft is None:
        ft = L.TOOLS.create_asset("FT_" + name, "/Game/Project/Environment/Foliage", unreal.FoliageType_InstancedStaticMesh, unreal.FoliageType_InstancedStaticMeshFactory())
    ft.set_editor_property("mesh", veg_mesh(name)[0])
    small = not name.startswith("CommonTree")
    grass = name.startswith("Grass")
    ft.set_editor_property("cull_distance", unreal.Int32Interval(min=0, max=4500 if grass else 9000 if small else 0))
    ft.set_editor_property("cast_shadow", not name.startswith(("Grass", "Clover", "Pebble")))
    L.EAL.save_loaded_asset(ft)
    return ft


@step("foliage instances")
def foliage(data):
    total = 0
    for name, items in data["foliage"].items():
        ft = foliage_type(name)
        unreal.InstancedFoliageActor.remove_all_instances(world, ft)
        _, factor, copies = veg_mesh(name)
        rng = random.Random(name)
        transforms = []
        for i in items:
            for c in range(copies):
                # extra copies are jittered around the original: a denser, continuous carpet
                dx, dy = (rng.uniform(-0.7, 0.7), rng.uniform(-0.7, 0.7)) if c else (0.0, 0.0)
                sc = i["scale"] * factor * (rng.uniform(0.8, 1.15) if c else 1.0)
                transforms.append(unreal.Transform(unreal.Vector((i["x"] + dx) * 100, (i["y"] + dy) * 100, i["z"] * 100 - 3),
                                                   unreal.Rotator(pitch=0.0, yaw=rng.uniform(0, 360) if c else i["yaw"], roll=0.0),
                                                   unreal.Vector(sc, sc, sc)))
        unreal.InstancedFoliageActor.add_instances(world, ft, transforms)
        total += len(transforms)
    return total


@step("hero trees + trunk colliders")
def hero_trees(data):
    L.destroy(["GiantTree_", "HeroTree_", "TrunkCollider_"])
    cyl = unreal.load_asset("/Engine/BasicShapes/Cylinder")
    for k, t in enumerate(data["trees_hero"]):
        mesh, factor, _ = veg_mesh(t["mesh"])
        L.spawn_mesh(mesh, "HeroTree_%02d" % k, (t["x"] * 100, t["y"] * 100, t["z"] * 100), t["yaw"], t["scale"] * factor * 1.15, "Jungle/Trees/Hero", collision=False)
        r, h = t["collider"]["radius"], t["collider"]["height"]
        trunk = {"SM_Tree_Broadleaf_A": 0.95, "SM_Tree_Broadleaf_B": 0.55}.get(mesh.get_name())
        if trunk:   # match the stylized trunk (radius at chest height, mesh units are metres)
            r, h = trunk * t["scale"] * factor * 1.15 * 1.05, 16.0 * t["scale"] * factor * 1.15 * 0.5
        col = L.spawn_mesh(cyl, "TrunkCollider_%02d" % k, (t["x"] * 100, t["y"] * 100, (t["z"] + h / 2) * 100), 0.0, (r * 2, r * 2, h), "Jungle/Trees/Hero")
        col.set_actor_hidden_in_game(True)
        col.static_mesh_component.set_visibility(False)
    return len(data["trees_hero"])


@step("rocks")
def rocks(data):
    L.destroy(["Boulder_", "RockSlab_", "Rock_Dress_"])
    for k, r in enumerate(data["rocks"]):
        L.spawn_mesh(L.nature_mesh(r["mesh"]), "Rock_Dress_%03d" % k, (r["x"] * 100, r["y"] * 100, r["z"] * 100), r["yaw"], r["scale"], "Jungle/Rocks")
    return len(data["rocks"])


@step("pool rim stones")
def pool():
    rock = L.nature_mesh("Rock_Medium_2")
    n = 0
    for a in L.find_actors(["Pool_RimStone_"]):
        c = a.static_mesh_component
        k = int(a.get_actor_label().split("_")[-1])
        s = (1.2 + 0.5 * ((k * 5) % 3)) / 3.0          # absolute size: safe to run again
        c.set_static_mesh(rock)
        c.set_editor_property("override_materials", [])
        a.set_actor_scale3d(unreal.Vector(s, s, s * 0.85))
        n += 1
    return n


@step("young trees visual")
def young_trees():
    mesh = unreal.load_asset("/Game/Project/Environment/Trees/SM_YoungTree_Physics")
    n = 0
    for a in L.find_actors(["Collapsible_YoungTree_"]):
        for c in a.get_components_by_class(unreal.StaticMeshComponent):
            name = c.get_name()
            if name.endswith("_Canopy"):
                c.set_static_mesh(mesh)
                c.set_editor_property("override_materials", [])
                loc = c.get_editor_property("relative_location")
                c.set_editor_property("relative_location", unreal.Vector(loc.x, loc.y, -20.0))
                c.set_editor_property("relative_scale3d", unreal.Vector(1.7, 1.7, 1.7))
            elif name.endswith("_Trunk"):
                c.set_visibility(False)
                c.set_hidden_in_game(True)
        n += 1
    return n


@step("props reskin")
def props():
    crate = unreal.load_asset("/Game/Project/Art/Props/Crate_Wooden/StaticMeshes/Crate_Wooden")
    bark = L.find_asset("/Game/Project/Art/Nature/CommonTree_1", "Bark_NormalTree", unreal.MaterialInterface)
    wood = L.find_asset("/Game/Project/Art/Village", "MI_WoodTrim", unreal.MaterialInterface)
    done = []
    for a in L.find_actors(["Crate_"]):
        m = a.get_editor_property("mesh") if hasattr(a, "mesh") else None
        comp = a.get_components_by_class(unreal.StaticMeshComponent)[0]
        comp.set_static_mesh(crate)
        comp.set_editor_property("override_materials", [])
        a.set_actor_scale3d(unreal.Vector(1.1, 1.1, 1.1))
        done.append(a.get_actor_label())
    for a in L.find_actors(["Log_"]):
        if bark:
            a.get_components_by_class(unreal.StaticMeshComponent)[0].set_material(0, bark)
            done.append(a.get_actor_label())
    if wood:
        for a in L.find_actors(["Collapsible_Scaffold", "Collapsible_Hut", "Lift_Post_", "Lift_Beam", "Lift_Aqueduct"]):
            for c in a.get_components_by_class(unreal.StaticMeshComponent):
                if c.get_name() != "Counterweight":
                    c.set_material(0, wood)
            done.append(a.get_actor_label())
    stone = unreal.load_asset(L.MAT_DIR + "/M_Eldoria_RuinStone")
    for a in L.find_actors(["Collapsible_RuinTower", "Inspect_"]):
        for c in a.get_components_by_class(unreal.StaticMeshComponent):
            c.set_material(0, stone)
        done.append(a.get_actor_label())
    return "%d actors (bark=%s wood=%s)" % (len(done), bool(bark), bool(wood))


def run():
    with open(os.path.join(TERRAIN_DIR, "scatter.json")) as f:
        data = json.load(f)
    terrain()
    # lighting, fog and grading now live in art_direction.py (run it after this script)
    young_tree_mesh()
    vegetation_collision()
    foliage(data)
    hero_trees(data)
    rocks(data)
    pool()
    young_trees()
    props()
    # stylized young trees + rocks (J1b): runs its own pass and saves
    import stylize_props
    importlib.reload(stylize_props)
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    L.EAL.save_directory("/Game/Project", only_if_is_dirty=True, recursive=True)
    log.append("saved")


try:
    run()
except Exception as exc:
    log.append("[FATAL] %r" % exc)
with open(REPORT, "w") as f:
    f.write("\n".join(log))
