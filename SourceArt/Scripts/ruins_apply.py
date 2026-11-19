# Project Eldoria - J3: imports the ruins kit (blender_ruins.py), builds stone/wood/water materials and swaps every
# greybox piece listed in greybox.json for its stylized twin (same size, so gameplay geometry is unchanged).
import json
import os
import sys
import importlib

sys.path.append(os.path.dirname(__file__))
import unreal
import eldoria_lib as L
importlib.reload(L)

MEL = unreal.MaterialEditingLibrary
SMES = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
SRC = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "Generated", "Ruins"))
DST = "/Game/Project/Environment/Ruins"
MAT_DIR = "/Game/Project/Materials/Stylized"
log = []

STONE_HLSL = r"""
float3 n = abs(N.xyz);
float2 uv; float2 bs;
if (n.z > 0.7) { uv = WP.xy; bs = float2(BlockTop, BlockTop); }
else if (n.x > n.y) { uv = float2(WP.y, WP.z); bs = float2(BlockWidth, BlockHeight); }
else { uv = float2(WP.x, WP.z); bs = float2(BlockWidth, BlockHeight); }
float row = floor(uv.y / bs.y);
float u = uv.x / bs.x + fmod(abs(row), 2.0) * 0.5;
float2 f = float2(frac(u), frac(uv.y / bs.y));
float2 cell = float2(floor(u), row);
float gap = min(min(f.x, 1.0 - f.x) * bs.x, min(f.y, 1.0 - f.y) * bs.y);
float joint = 1.0 - smoothstep(JointWidth, JointWidth * 2.2, gap);
float h = frac(sin(dot(cell + floor(WP.z / 997.0), float2(12.9898, 78.233))) * 43758.5453);
float tone = saturate(VC.r * 0.3 + h * 0.7);
tone = tone * tone;
float3 col = lerp(StoneA.rgb, StoneB.rgb, tone);
col *= lerp(0.92, 1.06, MossNoise);
col = lerp(col, JointColor.rgb, joint * 0.8);
float moss = saturate(VC.g * 1.4) * smoothstep(0.35, 0.6, MossNoise);
col = lerp(col, MossColor.rgb, moss);
// weathering: damp, darker and greener near the ground
float grime = 1.0 - smoothstep(20.0, 260.0, WP.z);
col = lerp(col, col * float3(0.62, 0.7, 0.5), grime * 0.7);
return col;
"""

WOOD_HLSL = r"""
float3 n = abs(N.xyz);
float along = (n.x > n.y) ? WP.y : WP.x;
float across = (n.z > 0.7) ? ((n.x > n.y) ? WP.x : WP.y) : WP.z;
float plank = frac(across / PlankWidth);
float seam = 1.0 - smoothstep(0.02, 0.06, min(plank, 1.0 - plank));
float id = floor(across / PlankWidth);
float h = frac(sin(id * 91.17) * 43758.5453);
float grain = 0.5 + 0.5 * sin(along * 0.08 + h * 20.0 + sin(along * 0.013 + id) * 3.0);
float3 col = lerp(WoodA.rgb, WoodB.rgb, h * 0.6 + grain * 0.4);
col = lerp(col, WoodA.rgb * 0.45, seam);
return col;
"""


def custom_material(name, code, inputs, params, vectors, noise_scale=None):
    mat = L.new_material(name)
    custom = L.expr(mat, unreal.MaterialExpressionCustom, -300, 0)
    custom.set_editor_property("code", code)
    custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    names = list(inputs) + list(params) + list(vectors) + (["MossNoise"] if noise_scale else [])
    cins = []
    for n in names:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", n)
        cins.append(ci)
    custom.set_editor_property("inputs", cins)
    sources = {"WP": unreal.MaterialExpressionWorldPosition, "N": unreal.MaterialExpressionVertexNormalWS, "VC": unreal.MaterialExpressionVertexColor}
    y = -400
    for n in inputs:
        MEL.connect_material_expressions(L.expr(mat, sources[n], -900, y), "", custom, n)
        y += 80
    for n, v in params.items():
        MEL.connect_material_expressions(L.param_scalar(mat, n, v, -900, y), "", custom, n)
        y += 60
    for n, v in vectors.items():
        MEL.connect_material_expressions(L.param_vector(mat, n, v, -900, y), "", custom, n)
        y += 80
    if noise_scale:
        MEL.connect_material_expressions(L.noise(mat, noise_scale, -900, y, 3), "", custom, "MossNoise")
    MEL.connect_material_property(custom, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(L.scalar(mat, 0.9, -300, 200), "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(L.scalar(mat, 0.25, -300, 260), "", unreal.MaterialProperty.MP_SPECULAR)
    return L.finish(mat)


def water_material():
    mat = L.new_material("M_Eldoria_Water")
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    try:
        mat.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE)
    except Exception:
        pass
    fade = L.expr(mat, unreal.MaterialExpressionDepthFade, -900, 0)
    MEL.connect_material_expressions(L.scalar(mat, 220.0, -1100, 40), "", fade, "FadeDistance")
    foam = L.expr(mat, unreal.MaterialExpressionDepthFade, -900, 160)
    MEL.connect_material_expressions(L.scalar(mat, 18.0, -1100, 200), "", foam, "FadeDistance")
    shallow = L.param_vector(mat, "Shallow", (0.16, 0.62, 0.58), -900, -260)
    deep = L.param_vector(mat, "Deep", (0.02, 0.22, 0.32), -900, -180)
    col = L.lerp(mat, shallow, deep, fade, -600, -200)
    foam_inv = L.expr(mat, unreal.MaterialExpressionOneMinus, -700, 160)
    MEL.connect_material_expressions(foam, "", foam_inv, "")
    col = L.lerp(mat, col, L.const3(mat, (0.95, 0.98, 1.0), -700, 260), foam_inv, -400, 0)
    MEL.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
    opacity = L.lerp(mat, L.scalar(mat, 0.55, -600, 360), L.scalar(mat, 0.9, -600, 420), fade, -400, 380)
    MEL.connect_material_property(opacity, "", unreal.MaterialProperty.MP_OPACITY)
    MEL.connect_material_property(L.scalar(mat, 0.05, -400, 480), "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(L.scalar(mat, 0.6, -400, 540), "", unreal.MaterialProperty.MP_SPECULAR)
    return L.finish(mat)


def run():
    data = json.load(open(os.path.join(SRC, "greybox.json")))
    stone = custom_material("M_Eldoria_Stone", STONE_HLSL, ["WP", "N", "VC"],
                            {"BlockWidth": 230.0, "BlockHeight": 95.0, "BlockTop": 170.0, "JointWidth": 2.2},
                            {"StoneA": (0.30, 0.27, 0.22), "StoneB": (0.53, 0.48, 0.39), "JointColor": (0.14, 0.12, 0.10),
                             "MossColor": (0.20, 0.36, 0.07)}, noise_scale=0.006)
    wood = custom_material("M_Eldoria_Wood", WOOD_HLSL, ["WP", "N"], {"PlankWidth": 24.0},
                           {"WoodA": (0.26, 0.15, 0.07), "WoodB": (0.42, 0.27, 0.14)})
    water = water_material()
    log.append("materials ok")

    tasks = []
    for f in sorted(os.listdir(SRC)):
        if f.endswith(".glb"):
            t = unreal.AssetImportTask()
            t.filename = os.path.join(SRC, f)
            t.destination_path = DST
            t.automated = True
            t.replace_existing = True
            t.save = True
            tasks.append(t)
    L.TOOLS.import_asset_tasks(tasks)
    meshes = {}
    for p in L.EAL.list_assets(DST, recursive=True):
        a = unreal.load_asset(p)
        if isinstance(a, unreal.StaticMesh):
            meshes[a.get_name()] = a
    for name, mesh in meshes.items():
        wood_piece = name.endswith("_W")
        for i, _ in enumerate(mesh.get_editor_property("static_materials")):
            mesh.set_material(i, wood if wood_piece else stone)
        # predictable collision: exact box for blocks, 26-DOP hull for columns
        SMES.remove_collisions(mesh)
        SMES.add_simple_collisions(mesh, unreal.ScriptCollisionShapeType.NDOP26 if "_Col_" in name else unreal.ScriptCollisionShapeType.BOX)
        L.EAL.save_loaded_asset(mesh)
    log.append("imported %d ruin meshes" % len(meshes))

    actors = {a.get_actor_label(): a for a in L.EAS.get_all_level_actors()}
    swapped = 0
    for item in data["items"]:
        a = actors.get(item["actor"])
        mesh = meshes.get("SM_Ruin_" + item["key"])
        if not a or not mesh:
            log.append("  missing %s / %s" % (item["actor"], item["key"]))
            continue
        comp = next((c for c in a.get_components_by_class(unreal.StaticMeshComponent) if c.get_name() == item["component"]), None)
        if not comp:
            continue
        a.modify()
        comp.set_static_mesh(mesh)
        comp.set_editor_property("override_materials", [])
        comp.set_world_scale3d(unreal.Vector(1, 1, 1))
        swapped += 1
    for a in L.find_actors(["Pool_Water"]):
        a.static_mesh_component.set_material(0, water)
        a.static_mesh_component.set_collision_profile_name("NoCollision")
    log.append("swapped %d pieces" % swapped)
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()


try:
    run()
except Exception as exc:
    log.append("[FATAL] %r" % exc)
with open(os.path.join(unreal.Paths.project_saved_dir(), "Ruins.txt"), "w") as f:
    f.write("\n".join(log))
