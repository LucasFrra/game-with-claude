# J4 HUD assets: heart icons (make_ui_textures.py) and the radial stamina ring material.
import os
import sys
import importlib

sys.path.append(os.path.dirname(__file__))
import unreal
import eldoria_lib as L
importlib.reload(L)

MEL = unreal.MaterialEditingLibrary
SRC = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "Generated", "UI"))
log = []

RING_HLSL = r"""
float2 v = UV - 0.5;
float r = length(v);
float ang = atan2(v.x, -v.y) / 6.2831853 + 0.5;          // 0 at the top, clockwise
float ring = smoothstep(Inner - 0.012, Inner, r) * (1.0 - smoothstep(Outer, Outer + 0.012, r));
float filled = step(1.0 - Percent, 1.0 - ang) ;
float3 col = lerp(Back.rgb, Fill.rgb, filled);
return float4(col, ring * lerp(0.35, 1.0, filled));
"""


def run():
    tasks = []
    for f in sorted(os.listdir(SRC)):
        if f.endswith(".png"):
            t = unreal.AssetImportTask()
            t.filename = os.path.join(SRC, f)
            t.destination_path = "/Game/Project/UI/Textures"
            t.automated = True
            t.replace_existing = True
            t.save = True
            tasks.append(t)
    L.TOOLS.import_asset_tasks(tasks)
    for p in L.EAL.list_assets("/Game/Project/UI/Textures"):
        tex = unreal.load_asset(p)
        if isinstance(tex, unreal.Texture2D):
            tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
            L.EAL.save_loaded_asset(tex)
    log.append("heart textures imported")

    L.ensure_dir("/Game/Project/UI/Materials")
    path = "/Game/Project/UI/Materials/M_UI_StaminaRing"
    if L.EAL.does_asset_exist(path):
        mat = unreal.load_asset(path)
        MEL.delete_all_material_expressions(mat)
    else:
        mat = L.TOOLS.create_asset("M_UI_StaminaRing", "/Game/Project/UI/Materials", unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    custom = L.expr(mat, unreal.MaterialExpressionCustom, -300, 0)
    custom.set_editor_property("code", RING_HLSL)
    custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT4)
    ins = []
    for n in ("UV", "Percent", "Inner", "Outer", "Fill", "Back"):
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", n)
        ins.append(ci)
    custom.set_editor_property("inputs", ins)
    MEL.connect_material_expressions(L.expr(mat, unreal.MaterialExpressionTextureCoordinate, -800, -200), "", custom, "UV")
    MEL.connect_material_expressions(L.param_scalar(mat, "Percent", 1.0, -800, -100), "", custom, "Percent")
    MEL.connect_material_expressions(L.param_scalar(mat, "Inner", 0.32, -800, 0), "", custom, "Inner")
    MEL.connect_material_expressions(L.param_scalar(mat, "Outer", 0.47, -800, 60), "", custom, "Outer")
    MEL.connect_material_expressions(L.param_vector(mat, "Fill", (0.42, 0.86, 0.36), -800, 140), "", custom, "Fill")
    MEL.connect_material_expressions(L.param_vector(mat, "Back", (0.05, 0.08, 0.05), -800, 240), "", custom, "Back")
    rgb = L.expr(mat, unreal.MaterialExpressionComponentMask, -100, 0)
    for c in ("r", "g", "b", "a"):
        rgb.set_editor_property(c, c != "a")
    MEL.connect_material_expressions(custom, "", rgb, "")
    alpha = L.mask(mat, custom, "a", -100, 100)
    MEL.connect_material_property(rgb, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.connect_material_property(alpha, "", unreal.MaterialProperty.MP_OPACITY)
    MEL.recompile_material(mat)
    L.EAL.save_loaded_asset(mat)
    log.append("stamina ring material")


try:
    run()
except Exception as exc:
    log.append("[FATAL] %r" % exc)
with open(os.path.join(unreal.Paths.project_saved_dir(), "UIAssets.txt"), "w") as f:
    f.write("\n".join(log))
