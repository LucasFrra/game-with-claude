# Project Eldoria - Tharum materials, blueprints and placement in Lvl_TharumJungle.
import os
import sys
import importlib

sys.path.append(os.path.dirname(__file__))
import unreal
import eldoria_lib as L
importlib.reload(L)

MEL = L.MEL
REPORT = os.path.join(unreal.Paths.project_saved_dir(), "TharumReport.txt")
log = []
MAT = "/Game/Project/Materials/Creatures"
BP_DIR = "/Game/Project/Bosses/Tharum"


def build_skin():
    mat = L.new_material("M_Eldoria_CreatureSkin")
    lp = L.expr(mat, unreal.MaterialExpressionLocalPosition, -1300, 0)
    n = L.expr(mat, unreal.MaterialExpressionNoise, -1100, 0)
    n.set_editor_property("scale", 0.004)
    n.set_editor_property("levels", 3)
    n.set_editor_property("output_min", 0.0)
    n.set_editor_property("output_max", 1.0)
    MEL.connect_material_expressions(lp, "", n, "Position")
    tint = L.param_vector(mat, "SkinA", (0.22, 0.20, 0.19), -1100, -200)
    tint2 = L.param_vector(mat, "SkinB", (0.36, 0.33, 0.30), -1100, -120)
    skin = L.lerp(mat, tint, tint2, n, -850, -100)
    # Moss and small plants growing on the upper surfaces (world up = top of the back)
    up = L.mask(mat, L.expr(mat, unreal.MaterialExpressionVertexNormalWS, -1100, 250), "b", -950, 250)
    moss = L.expr(mat, unreal.MaterialExpressionPower, -800, 250)
    MEL.connect_material_expressions(up, "", moss, "Base")
    moss.set_editor_property("const_exponent", 5.0)
    moss_n = L.noise(mat, 0.01, -800, 380, 2)
    moss_mask = L.expr(mat, unreal.MaterialExpressionMultiply, -650, 300)
    MEL.connect_material_expressions(moss, "", moss_mask, "A")
    MEL.connect_material_expressions(moss_n, "", moss_mask, "B")
    final = L.lerp(mat, skin, L.param_vector(mat, "Moss", (0.10, 0.22, 0.05), -650, 420), moss_mask, -450, 100)
    MEL.connect_material_property(final, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(L.scalar(mat, 0.88, -450, 300), "", unreal.MaterialProperty.MP_ROUGHNESS)
    return L.finish(mat)


def build_core():
    mat = L.new_material("M_Eldoria_DeviceCore")
    color = L.param_vector(mat, "Glow", (0.12, 0.75, 0.62), -600, 0)
    MEL.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    emissive = L.mul_const(mat, color, 2.2, -400, 100)
    MEL.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.connect_material_property(L.scalar(mat, 0.3, -400, 200), "", unreal.MaterialProperty.MP_ROUGHNESS)
    return L.finish(mat)


def flat(name, rgb, rough, metal=0.0):
    parent = unreal.load_asset("/Game/LevelPrototyping/Materials/M_FlatCol")
    return L.make_instance(name, parent, MAT, None, {"Roughness": rough, "Metallic": metal}, {"Base Color": rgb})


def make_blueprint(name, parent_class):
    path = BP_DIR + "/" + name
    if L.EAL.does_asset_exist(path):
        return unreal.load_asset(path)
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    return L.TOOLS.create_asset(name, BP_DIR, unreal.Blueprint, factory)


def run():
    L.ensure_dir(MAT)
    L.ensure_dir(BP_DIR)
    skin = build_skin()
    core = build_core()
    tusk = flat("MI_Tharum_Tusk", (0.78, 0.72, 0.58), 0.45)
    metal = flat("MI_Device_Metal", (0.11, 0.17, 0.16), 0.35, 0.8)
    armor = L.find_asset("/Game/Project/Art/Nature/Rock_Medium_1", "Rocks", unreal.MaterialInterface) or flat("MI_Tharum_Armor", (0.3, 0.28, 0.24), 0.9)
    log.append("materials: skin=%s core=%s armor=%s" % (skin.get_name(), core.get_name(), armor.get_name()))

    device_bp = make_blueprint("BP_ControlDevice", unreal.EldoriaControlDevice)
    device_cdo = unreal.get_default_object(device_bp.generated_class())
    device_cdo.set_editor_property("metal_material", metal)
    device_cdo.set_editor_property("core_material", core)
    unreal.BlueprintEditorLibrary.compile_blueprint(device_bp)
    L.EAL.save_loaded_asset(device_bp)

    tharum_bp = make_blueprint("BP_Tharum", unreal.EldoriaTharum)
    cdo = unreal.get_default_object(tharum_bp.generated_class())
    cdo.set_editor_property("skin_material", skin)
    cdo.set_editor_property("tusk_material", tusk)
    cdo.set_editor_property("armor_material", armor)
    cdo.set_editor_property("control_device_class", device_bp.generated_class())
    cdo.set_editor_property("draw_debug", True)
    unreal.BlueprintEditorLibrary.compile_blueprint(tharum_bp)
    L.EAL.save_loaded_asset(tharum_bp)
    log.append("blueprints ok")

    L.destroy(["Tharum"])
    actor = L.EAS.spawn_actor_from_class(tharum_bp.generated_class(), unreal.Vector(-800, -1200, 0), unreal.Rotator(pitch=0.0, yaw=60.0, roll=0.0))
    actor.set_actor_label("Tharum")
    actor.set_folder_path("Jungle/Tharum")

    def wp(x, y, wait, head=False):
        w = unreal.EldoriaDeityWaypoint()
        w.set_editor_property("location", unreal.Vector(x * 100, y * 100, 0))
        w.set_editor_property("wait_time", wait)
        w.set_editor_property("lower_head", head)
        return w

    actor.set_editor_property("waypoints", [wp(17, -3, 12.0, True), wp(-8, -12, 3.0), wp(-20, 12, 4.0), wp(5, 18, 3.0)])
    log.append("tharum placed with %d waypoints" % len(actor.get_editor_property("waypoints")))
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()


try:
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    les.load_level("/Game/Project/Levels/Lvl_TharumJungle")
    run()
except Exception as exc:
    log.append("[FATAL] %r" % exc)
with open(REPORT, "w") as f:
    f.write("\n".join(log))
