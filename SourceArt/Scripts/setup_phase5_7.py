# Project Eldoria - phases 5 to 7: Blender assets on Tharum and the device, encounter routes,
# encounter manager, FORCE input, lighting. Run in Lvl_TharumJungle AFTER dress_jungle_2.py.
import os
import sys
import importlib

sys.path.append(os.path.dirname(__file__))
import unreal
import eldoria_lib as L
importlib.reload(L)

HERE = os.path.dirname(__file__)
MESH_SRC = os.path.normpath(os.path.join(HERE, "..", "Generated", "Tharum"))
MESH_DST = "/Game/Project/Bosses/Tharum/Meshes"
REPORT = os.path.join(unreal.Paths.project_saved_dir(), "Phase57Report.txt")
log = []


def step(name):
    def deco(fn):
        def wrapper(*a, **k):
            try:
                r = fn(*a, **k)
                log.append("[ok] %s %s" % (name, "" if r is None else r))
                return r
            except Exception as exc:
                log.append("[FAIL] %s: %r" % (name, exc))
        return wrapper
    return deco


@step("import blender meshes")
def import_meshes():
    tasks = []
    for f in sorted(os.listdir(MESH_SRC)):
        if f.endswith(".glb"):
            t = unreal.AssetImportTask()
            t.filename = os.path.join(MESH_SRC, f)
            t.destination_path = MESH_DST
            t.automated = True
            t.replace_existing = True
            t.save = True
            tasks.append(t)
    L.TOOLS.import_asset_tasks(tasks)
    meshes = {}
    for p in L.EAL.list_assets(MESH_DST, recursive=True):
        a = unreal.load_asset(p)
        if isinstance(a, unreal.StaticMesh):
            meshes[a.get_name()] = a
    return meshes


def comp(cdo, prop):
    return cdo.get_editor_property(prop)


@step("tharum blueprint visuals")
def tharum_visuals(meshes):
    bp = unreal.load_asset("/Game/Project/Bosses/Tharum/BP_Tharum")
    cdo = unreal.get_default_object(bp.generated_class())
    tusk_mi = unreal.load_asset("/Game/Project/Materials/Creatures/MI_Tharum_Tusk")

    def set_mesh(c, name, scale=None):
        c.set_editor_property("static_mesh", meshes[name])
        if scale is not None:
            c.set_editor_property("relative_scale3d", unreal.Vector(*scale))

    set_mesh(comp(cdo, "body_mesh"), "SM_Tharum_Body", (1, 1, 1))
    belly = comp(cdo, "belly_mesh")
    belly.set_editor_property("visible", False)
    belly.set_editor_property("hidden_in_game", True)
    set_mesh(comp(cdo, "head_mesh"), "SM_Tharum_Head", (1, 1, 1))
    ears = comp(cdo, "ear_meshes")
    set_mesh(ears[0], "SM_Tharum_EarA", (1, 1, 1))
    set_mesh(ears[1], "SM_Tharum_EarB", (1, 1, 1))
    for leg in comp(cdo, "leg_meshes"):
        set_mesh(leg, "SM_Tharum_Leg", (1, 1, 1))
    for foot in comp(cdo, "foot_meshes"):
        set_mesh(foot, "SM_Tharum_Foot", (1, 1, 1))
        foot.set_editor_property("override_materials", [None, tusk_mi])
    for seg in comp(cdo, "trunk_meshes"):
        set_mesh(seg, "SM_Tharum_TrunkSegment")
    for t in comp(cdo, "tusk_meshes"):
        set_mesh(t, "SM_Tharum_TuskTip" if "Tip" in t.get_name() else "SM_Tharum_Tusk")
    for a in comp(cdo, "armor_meshes"):
        set_mesh(a, "SM_Tharum_Armor")
    cdo.set_editor_property("draw_debug", False)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    L.EAL.save_loaded_asset(bp)


@step("device blueprint visuals")
def device_visuals(meshes):
    bp = unreal.load_asset("/Game/Project/Bosses/Tharum/BP_ControlDevice")
    cdo = unreal.get_default_object(bp.generated_class())
    metal = unreal.load_asset("/Game/Project/Materials/Creatures/MI_Device_Metal")
    core = unreal.load_asset("/Game/Project/Materials/Master/M_Eldoria_DeviceCore")
    comp(cdo, "mesh").set_editor_property("static_mesh", meshes["SM_Device_Plate"])
    for c in comp(cdo, "clamps"):
        c.set_editor_property("static_mesh", meshes["SM_Device_Pillar" if c.get_name() == "Pillar" else "SM_Device_Clamp"])
    for r in comp(cdo, "rings"):
        r.set_editor_property("static_mesh", meshes["SM_Device_Ring"])
    comp(cdo, "core").set_editor_property("static_mesh", meshes["SM_Device_Core"])
    for lock in comp(cdo, "locks"):
        lock.set_editor_property("static_mesh", meshes["SM_Device_Lock"])
        lock.set_editor_property("override_materials", [metal, core])
    cdo.set_editor_property("lock_material", metal)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    L.EAL.save_loaded_asset(bp)


@step("route B: lift removed, block moved, pool moved")
def route_b():
    removed = L.destroy(["Lift_Aqueduct", "Lift_Post_", "Lift_Beam"])
    for a in L.find_actors(["Block_RouteB"]):
        a.set_actor_location(unreal.Vector(1300, 3600, 62), False, True)
    for a in L.find_actors(["Pool_Water"]):
        a.set_actor_location(unreal.Vector(3300, 200, -25), False, True)
        a.set_actor_scale3d(unreal.Vector(12.5, 12.5, 0.06))
    import math
    for a in L.find_actors(["Pool_RimStone_"]):
        k = int(a.get_actor_label().split("_")[-1])
        ang = math.radians(k * 40 + 10)
        a.set_actor_location(unreal.Vector(3300 + 650 * math.cos(ang), 200 + 650 * math.sin(ang), 20), False, True)
    return "removed %d lift actors" % removed


@step("route A: the tower falls onto its attacker")
def route_a():
    for a in L.find_actors(["Collapsible_RuinTower"]):
        a.set_editor_property("use_fixed_fall_direction", True)
        a.set_editor_property("fixed_fall_direction", unreal.Vector(1, 0, 0))
        a.set_editor_property("freeze_when_settled", False)
        a.set_editor_property("top_kick_speed", 900.0)
        a.set_editor_property("structure_mass_kg", 40000.0)


@step("tharum route")
def tharum_route():
    def wp(x, y, wait, head=False, facing=None):
        w = unreal.EldoriaDeityWaypoint()
        w.set_editor_property("location", unreal.Vector(x * 100, y * 100, 0))
        w.set_editor_property("wait_time", wait)
        w.set_editor_property("lower_head", head)
        if facing is not None:
            w.set_editor_property("use_facing", True)
            w.set_editor_property("facing_yaw", facing)
        return w
    route = [wp(-8, -12, 3.0), wp(-20, 12, 3.0), wp(5, 22, 2.0), wp(33, -6, 1.0),
             wp(33.5, 17, 16.0, True, -90.0), wp(33, -6, 1.0), wp(4, 0, 3.0)]
    for t in L.find_actors(["Tharum"]):
        if isinstance(t, unreal.EldoriaTharum):
            t.set_editor_property("waypoints", route)
            t.set_actor_location(unreal.Vector(-800, -1200, 0), False, True)
    return len(route)


@step("input: FORCE ability")
def ability_input():
    path = "/Game/Project/Input/IA_Ability"
    ia = unreal.load_asset(path) if L.EAL.does_asset_exist(path) else L.TOOLS.create_asset("IA_Ability", "/Game/Project/Input", unreal.InputAction, None)
    imc = unreal.load_asset("/Game/Project/Input/IMC_Eldoria")
    existing = [str(m.key.key_name) for m in imc.get_editor_property("default_key_mappings").get_editor_property("mappings")]
    for key in ("F", "Gamepad_FaceButton_Top"):
        if key not in existing:
            imc.map_key(ia, unreal.Key(key_name=key))
    L.EAL.save_loaded_asset(imc)
    bp = unreal.load_asset("/Game/Project/Characters/BP_EldoriaCharacter")
    unreal.get_default_object(bp.generated_class()).set_editor_property("ability_action", ia)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    L.EAL.save_loaded_asset(bp)
    return existing


@step("encounter manager")
def encounter():
    L.destroy(["Encounter_Tharum"])
    m = L.EAS.spawn_actor_from_class(unreal.EldoriaEncounterManager, unreal.Vector(0, 0, 200))
    m.set_actor_label("Encounter_Tharum")
    m.set_folder_path("Jungle/Tharum")
    for a in L.EAS.get_all_level_actors():
        lbl = a.get_actor_label()
        if isinstance(a, unreal.EldoriaTharum):
            m.set_editor_property("deity", a)
        elif lbl == "PostProcessVolume":
            m.set_editor_property("post_process", a)
        elif lbl == "ExponentialHeightFog":
            m.set_editor_property("fog", a)
        elif lbl == "DirectionalLight":
            m.set_editor_property("sun", a)
    return m.get_editor_property("deity")


@step("lighting at the start")
def lighting():
    for a in L.EAS.get_all_level_actors():
        lbl = a.get_actor_label()
        if lbl == "DirectionalLight":
            # higher sun so the arrival corridor is not left in the cliffs' shadow
            a.set_actor_rotation(unreal.Rotator(pitch=-56.0, yaw=38.0, roll=0.0), False)
        elif lbl == "SkyLight":
            a.light_component.set_editor_property("intensity", 1.7)
        elif lbl == "PostProcessVolume":
            s = a.get_editor_property("settings")
            s.set_editor_property("override_auto_exposure_bias", True)
            s.set_editor_property("auto_exposure_bias", 0.35)
            a.set_editor_property("settings", s)


def run():
    L.ensure_dir(MESH_DST)
    meshes = import_meshes() or {}
    log.append("meshes: %s" % sorted(meshes.keys()))
    if meshes:
        tharum_visuals(meshes)
        device_visuals(meshes)
    route_b()
    route_a()
    tharum_route()
    encounter()
    lighting()
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    L.EAL.save_directory("/Game/Project", only_if_is_dirty=True, recursive=True)
    log.append("saved")


try:
    run()
except Exception as exc:
    log.append("[FATAL] %r" % exc)
with open(REPORT, "w") as f:
    f.write("\n".join(log))
