# J5 assets: material parameter collection (player position), dust material, foliage bending, lock-on input.
import os
import sys
import importlib

sys.path.append(os.path.dirname(__file__))
import unreal
import eldoria_lib as L
importlib.reload(L)

MEL = unreal.MaterialEditingLibrary
log = []


def step(name, fn):
    try:
        r = fn()
        log.append("[ok] %s %s" % (name, "" if r is None else r))
    except Exception as exc:
        log.append("[FAIL] %s: %r" % (name, exc))


def mpc():
    path = "/Game/Project/Materials/MPC_Eldoria"
    col = unreal.load_asset(path) if L.EAL.does_asset_exist(path) else L.TOOLS.create_asset(
        "MPC_Eldoria", "/Game/Project/Materials", unreal.MaterialParameterCollection, unreal.MaterialParameterCollectionFactoryNew())
    p = unreal.CollectionVectorParameter()
    p.set_editor_property("parameter_name", "PlayerPosition")
    p.set_editor_property("default_value", unreal.LinearColor(0, 0, -100000, 0))
    col.set_editor_property("vector_parameters", [p])
    L.EAL.save_loaded_asset(col)
    return path


def dust():
    L.ensure_dir("/Game/Project/Materials/FX")
    path = "/Game/Project/Materials/FX/M_Eldoria_Dust"
    if L.EAL.does_asset_exist(path):
        mat = unreal.load_asset(path)
        MEL.delete_all_material_expressions(mat)
    else:
        mat = L.TOOLS.create_asset("M_Eldoria_Dust", "/Game/Project/Materials/FX", unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    color = L.param_vector(mat, "DustColor", (0.78, 0.71, 0.58), -700, -100)
    MEL.connect_material_property(color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    fres = L.expr(mat, unreal.MaterialExpressionFresnel, -900, 100)
    fres.set_editor_property("exponent", 2.2)
    soft = L.expr(mat, unreal.MaterialExpressionOneMinus, -700, 100)
    MEL.connect_material_expressions(fres, "", soft, "")
    fade = L.expr(mat, unreal.MaterialExpressionDepthFade, -700, 220)
    MEL.connect_material_expressions(L.scalar(mat, 60.0, -900, 260), "", fade, "FadeDistance")
    m1 = L.expr(mat, unreal.MaterialExpressionMultiply, -500, 140)
    MEL.connect_material_expressions(soft, "", m1, "A")
    MEL.connect_material_expressions(fade, "", m1, "B")
    m2 = L.expr(mat, unreal.MaterialExpressionMultiply, -300, 160)
    MEL.connect_material_expressions(m1, "", m2, "A")
    MEL.connect_material_expressions(L.param_scalar(mat, "Opacity", 0.5, -500, 300), "", m2, "B")
    MEL.connect_material_property(m2, "", unreal.MaterialProperty.MP_OPACITY)
    return L.finish(mat).get_path_name()


def foliage():
    return L.build_foliage_material().get_path_name()


def lock_on():
    path = "/Game/Project/Input/IA_LockOn"
    ia = unreal.load_asset(path) if L.EAL.does_asset_exist(path) else L.TOOLS.create_asset("IA_LockOn", "/Game/Project/Input", unreal.InputAction, unreal.InputAction_Factory() if hasattr(unreal, "InputAction_Factory") else None)
    imc = unreal.load_asset("/Game/Project/Input/IMC_Eldoria")
    existing = [str(m.get_editor_property("key").get_editor_property("key_name")) for m in imc.get_editor_property("default_key_mappings").get_editor_property("mappings")]
    for key in ("RightMouseButton", "Gamepad_LeftTrigger"):
        if key not in existing:
            k = unreal.Key()
            k.set_editor_property("key_name", key)
            imc.map_key(ia, k)
    L.EAL.save_loaded_asset(imc)
    return existing


step("mpc", mpc)
step("dust material", dust)
step("foliage master (player push)", foliage)
step("lock-on input", lock_on)
with open(os.path.join(unreal.Paths.project_saved_dir(), "JuiceAssets.txt"), "w") as f:
    f.write("\n".join(log))
