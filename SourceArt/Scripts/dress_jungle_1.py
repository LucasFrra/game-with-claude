# Project Eldoria - jungle visual pass 1: remove box terrain, create master materials, apply them.
import os
import unreal

MEL = unreal.MaterialEditingLibrary
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
tools = unreal.AssetToolsHelpers.get_asset_tools()
log = []
MAT_DIR = "/Game/Project/Materials/Master"


def expr(mat, cls, x, y):
    return MEL.create_material_expression(mat, cls, x, y)


def const3(mat, rgb, x, y):
    e = expr(mat, unreal.MaterialExpressionConstant3Vector, x, y)
    e.set_editor_property("constant", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    return e


def scalar(mat, v, x, y):
    e = expr(mat, unreal.MaterialExpressionConstant, x, y)
    e.set_editor_property("r", v)
    return e


def lerp(mat, a, b, alpha, x, y, a_out="", b_out="", alpha_out=""):
    e = expr(mat, unreal.MaterialExpressionLinearInterpolate, x, y)
    MEL.connect_material_expressions(a, a_out, e, "A")
    MEL.connect_material_expressions(b, b_out, e, "B")
    MEL.connect_material_expressions(alpha, alpha_out, e, "Alpha")
    return e


def new_material(name):
    path = MAT_DIR + "/" + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.delete_asset(path)
    return tools.create_asset(name, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())


def noise(mat, scale, x, y, levels=3):
    wp = expr(mat, unreal.MaterialExpressionWorldPosition, x - 300, y)
    n = expr(mat, unreal.MaterialExpressionNoise, x, y)
    n.set_editor_property("scale", scale)
    n.set_editor_property("levels", levels)
    n.set_editor_property("output_min", 0.0)
    n.set_editor_property("output_max", 1.0)
    n.set_editor_property("quality", 1)
    MEL.connect_material_expressions(wp, "", n, "Position")
    return n


# ------------------------------------------------------------------ terrain material
def build_terrain_material():
    mat = new_material("M_Eldoria_Terrain")
    vc = expr(mat, unreal.MaterialExpressionVertexColor, -1400, 0)

    # Grass: two greens broken up by large noise, darkened by moss
    grass_n = noise(mat, 0.0015, -1100, -400, 4)
    grass = lerp(mat, const3(mat, (0.11, 0.27, 0.05), -900, -520), const3(mat, (0.24, 0.42, 0.08), -900, -460), grass_n, -700, -450)
    moss_amount = expr(mat, unreal.MaterialExpressionMultiply, -700, -330)
    MEL.connect_material_expressions(vc, "B", moss_amount, "A")
    moss_amount.set_editor_property("const_b", 0.45)
    grass = lerp(mat, grass, const3(mat, (0.06, 0.17, 0.04), -700, -280), moss_amount, -500, -380)

    # Dirt
    dirt_n = noise(mat, 0.004, -1100, -150, 2)
    dirt = lerp(mat, const3(mat, (0.26, 0.17, 0.09), -900, -200), const3(mat, (0.36, 0.26, 0.14), -900, -140), dirt_n, -700, -150)

    # Rock with horizontal strata (stylized layered cliffs)
    wp = expr(mat, unreal.MaterialExpressionWorldPosition, -1300, 150)
    mask_z = expr(mat, unreal.MaterialExpressionComponentMask, -1150, 150)
    for c, v in (("r", False), ("g", False), ("b", True), ("a", False)):
        mask_z.set_editor_property(c, v)
    MEL.connect_material_expressions(wp, "", mask_z, "")
    strata_noise = noise(mat, 0.003, -1150, 260, 2)
    strata_in = expr(mat, unreal.MaterialExpressionAdd, -1000, 180)
    scale_z = expr(mat, unreal.MaterialExpressionMultiply, -1050, 120)
    MEL.connect_material_expressions(mask_z, "", scale_z, "A")
    scale_z.set_editor_property("const_b", 1.0 / 220.0)
    MEL.connect_material_expressions(scale_z, "", strata_in, "A")
    MEL.connect_material_expressions(strata_noise, "", strata_in, "B")
    frac = expr(mat, unreal.MaterialExpressionFrac, -850, 180)
    MEL.connect_material_expressions(strata_in, "", frac, "")
    rock = lerp(mat, const3(mat, (0.30, 0.25, 0.21), -700, 120), const3(mat, (0.45, 0.38, 0.30), -700, 180), frac, -550, 150)

    # Combine: grass -> dirt (G) -> rock (R)
    ground = lerp(mat, grass, dirt, vc, -300, -250, alpha_out="G")
    final = lerp(mat, ground, rock, vc, -100, 0, alpha_out="R")
    MEL.connect_material_property(final, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(scalar(mat, 0.95, -300, 300), "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(scalar(mat, 0.25, -300, 360), "", unreal.MaterialProperty.MP_SPECULAR)
    MEL.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    return mat


# ------------------------------------------------------------------ world aligned stone for ruins
def find_texture(name):
    for p in unreal.EditorAssetLibrary.list_assets("/Game/Project/Art/Village", recursive=True):
        if p.split(".")[-1] == name or p.endswith("/" + name):
            a = unreal.load_asset(p)
            if isinstance(a, unreal.Texture2D):
                return a
    return None


def build_stone_material():
    mat = new_material("M_Eldoria_RuinStone")
    tex = find_texture("T_UnevenBrick_BaseColor")
    fn = unreal.load_asset("/Engine/Functions/Engine_MaterialFunctions01/Texturing/WorldAlignedTexture")
    if not tex or not fn:
        log.append("stone material: missing texture or function (%s, %s)" % (tex, fn))
        return None
    tobj = expr(mat, unreal.MaterialExpressionTextureObject, -900, 0)
    tobj.set_editor_property("texture", tex)
    call = expr(mat, unreal.MaterialExpressionMaterialFunctionCall, -600, 0)
    call.set_editor_property("material_function", fn)
    MEL.connect_material_expressions(tobj, "", call, "TextureObject")
    size = const3(mat, (300.0, 300.0, 300.0), -900, 120)
    MEL.connect_material_expressions(size, "", call, "TextureSize")
    tint = expr(mat, unreal.MaterialExpressionMultiply, -300, 0)
    MEL.connect_material_expressions(call, "XYZ Texture", tint, "A")
    MEL.connect_material_expressions(const3(mat, (0.95, 0.88, 0.75), -500, 150), "", tint, "B")
    # Moss on top faces
    wn = expr(mat, unreal.MaterialExpressionVertexNormalWS, -600, 300)
    up = expr(mat, unreal.MaterialExpressionComponentMask, -450, 300)
    for c, v in (("r", False), ("g", False), ("b", True), ("a", False)):
        up.set_editor_property(c, v)
    MEL.connect_material_expressions(wn, "", up, "")
    moss_mask = expr(mat, unreal.MaterialExpressionPower, -300, 300)
    MEL.connect_material_expressions(up, "", moss_mask, "Base")
    moss_mask.set_editor_property("const_exponent", 6.0)
    final = lerp(mat, tint, const3(mat, (0.14, 0.27, 0.07), -300, 420), moss_mask, -100, 100)
    MEL.connect_material_property(final, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(scalar(mat, 0.9, -300, 500), "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    return mat


# ------------------------------------------------------------------ level edits
REMOVE_PREFIXES = ("Ground_Main", "Cliff_Ring_", "Corridor_Wall_", "Corridor_BackWall", "Cliff_Shoulder_",
                   "Plateau_Arrival", "Ramp_Arrival", "Mound_", "Outcrop_", "Tunnel_Wall_", "Shrine_Wall_")
STONE_PREFIXES = ("Stair_Step", "Aqueduct_", "Colonnade_", "Tower_Wall_Stub", "Tunnel_Arch", "Shrine_Dais")


def run():
    if not unreal.EditorAssetLibrary.does_directory_exist(MAT_DIR):
        unreal.EditorAssetLibrary.make_directory(MAT_DIR)
    terrain_mat = build_terrain_material()
    stone_mat = build_stone_material()
    log.append("materials: %s %s" % (terrain_mat, stone_mat))

    removed = 0
    stoned = 0
    for a in eas.get_all_level_actors():
        label = a.get_actor_label()
        if label.startswith(REMOVE_PREFIXES):
            eas.destroy_actor(a)
            removed += 1
            continue
        if label == "Terrain_TharumJungle":
            a.static_mesh_component.set_material(0, terrain_mat)
        elif stone_mat and label.startswith(STONE_PREFIXES) and isinstance(a, unreal.StaticMeshActor):
            a.static_mesh_component.set_material(0, stone_mat)
            stoned += 1
        elif label == "Pool_Water":
            loc = a.get_actor_location()
            a.set_actor_location(unreal.Vector(loc.x, loc.y, -25.0), False, False)
    log.append("removed %d box terrain actors, stone material on %d ruins" % (removed, stoned))
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()


try:
    run()
except Exception as exc:
    log.append("ERROR: %r" % exc)
with open(os.path.join(unreal.Paths.project_saved_dir(), "DressReport.txt"), "w") as f:
    f.write("\n".join(str(l) for l in log))
