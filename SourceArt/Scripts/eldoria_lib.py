# Project Eldoria - shared editor helpers (materials, assets, actors). Imported by the dressing scripts.
import unreal

MEL = unreal.MaterialEditingLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
MAT_DIR = "/Game/Project/Materials/Master"


def ensure_dir(path):
    if not EAL.does_directory_exist(path):
        EAL.make_directory(path)


def nature_mesh(name):
    return unreal.load_asset("/Game/Project/Art/Nature/%s/StaticMeshes/%s" % (name, name))


def find_asset(root, name, cls):
    for p in EAL.list_assets(root, recursive=True):
        if p.split("/")[-1].split(".")[0] == name:
            a = unreal.load_asset(p)
            if isinstance(a, cls):
                return a
    return None


# ------------------------------------------------------------------ material graph helpers
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


def mul_const(mat, src, value, x, y, src_out=""):
    e = expr(mat, unreal.MaterialExpressionMultiply, x, y)
    MEL.connect_material_expressions(src, src_out, e, "A")
    e.set_editor_property("const_b", value)
    return e


def mask(mat, src, channel, x, y):
    e = expr(mat, unreal.MaterialExpressionComponentMask, x, y)
    for c in ("r", "g", "b", "a"):
        e.set_editor_property(c, c == channel)
    MEL.connect_material_expressions(src, "", e, "")
    return e


def noise(mat, scale, x, y, levels=3, lo=0.0, hi=1.0):
    wp = expr(mat, unreal.MaterialExpressionWorldPosition, x - 300, y)
    n = expr(mat, unreal.MaterialExpressionNoise, x, y)
    n.set_editor_property("scale", scale)
    n.set_editor_property("levels", levels)
    n.set_editor_property("output_min", lo)
    n.set_editor_property("output_max", hi)
    n.set_editor_property("quality", 1)
    MEL.connect_material_expressions(wp, "", n, "Position")
    return n


def new_material(name):
    ensure_dir(MAT_DIR)
    path = MAT_DIR + "/" + name
    if EAL.does_asset_exist(path):
        mat = unreal.load_asset(path)
        MEL.delete_all_material_expressions(mat)
        return mat
    return TOOLS.create_asset(name, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())


def finish(mat):
    MEL.recompile_material(mat)
    EAL.save_loaded_asset(mat)
    return mat


# ------------------------------------------------------------------ master materials
def build_terrain_material():
    """Grass (two tones + moss) / dirt (vertex G) / layered rock (vertex R)."""
    mat = new_material("M_Eldoria_Terrain")
    vc = expr(mat, unreal.MaterialExpressionVertexColor, -1500, 0)

    grass_n = noise(mat, 0.0008, -1200, -450, 4)
    grass = lerp(mat, const3(mat, (0.15, 0.28, 0.045), -1000, -560), const3(mat, (0.27, 0.42, 0.07), -1000, -500), grass_n, -800, -480)
    detail_n = noise(mat, 0.02, -1200, -330, 2)
    grass = lerp(mat, grass, const3(mat, (0.21, 0.36, 0.06), -800, -380), mul_const(mat, detail_n, 0.3, -950, -330), -650, -430)
    moss = mul_const(mat, vc, 0.5, -800, -270, src_out="B")
    grass = lerp(mat, grass, const3(mat, (0.12, 0.27, 0.04), -650, -300), moss, -480, -380)

    dirt_n = noise(mat, 0.004, -1200, -150, 2)
    dirt = lerp(mat, const3(mat, (0.40, 0.25, 0.10), -1000, -200), const3(mat, (0.55, 0.36, 0.16), -1000, -140), dirt_n, -800, -160)

    wp = expr(mat, unreal.MaterialExpressionWorldPosition, -1450, 180)
    z = mask(mat, wp, "b", -1300, 180)
    bands = mul_const(mat, z, 1.0 / 420.0, -1150, 180)
    wobble = mul_const(mat, noise(mat, 0.001, -1150, 300, 2), 0.35, -1000, 300)
    strata_in = expr(mat, unreal.MaterialExpressionAdd, -950, 200)
    MEL.connect_material_expressions(bands, "", strata_in, "A")
    MEL.connect_material_expressions(wobble, "", strata_in, "B")
    frac = expr(mat, unreal.MaterialExpressionFrac, -800, 200)
    MEL.connect_material_expressions(strata_in, "", frac, "")
    rock = lerp(mat, const3(mat, (0.27, 0.23, 0.18), -650, 140), const3(mat, (0.40, 0.35, 0.27), -650, 200), frac, -500, 170)
    rock_grain = noise(mat, 0.01, -650, 300, 2)
    rock = lerp(mat, rock, const3(mat, (0.21, 0.18, 0.14), -500, 300), mul_const(mat, rock_grain, 0.2, -500, 360), -350, 200)

    ground = lerp(mat, grass, dirt, vc, -300, -250, alpha_out="G")
    rock_alpha = expr(mat, unreal.MaterialExpressionPower, -300, 60)
    MEL.connect_material_expressions(vc, "R", rock_alpha, "Base")
    rock_alpha.set_editor_property("const_exponent", 0.7)
    final = lerp(mat, ground, rock, rock_alpha, -100, 0)
    MEL.connect_material_property(final, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(scalar(mat, 0.95, -300, 420), "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(scalar(mat, 0.2, -300, 480), "", unreal.MaterialProperty.MP_SPECULAR)
    return finish(mat)


# ------------------------------------------------------------------ actors
def find_actors(prefixes):
    return [a for a in EAS.get_all_level_actors() if a.get_actor_label().startswith(tuple(prefixes))]


def destroy(prefixes):
    n = 0
    for a in find_actors(prefixes):
        EAS.destroy_actor(a)
        n += 1
    return n


def spawn_mesh(mesh, label, loc, yaw=0.0, scale=1.0, folder=None, collision=True):
    actor = EAS.spawn_actor_from_object(mesh, unreal.Vector(*loc), unreal.Rotator(pitch=0.0, yaw=yaw, roll=0.0))
    actor.set_actor_label(label)
    s = scale if isinstance(scale, (tuple, list)) else (scale, scale, scale)
    actor.set_actor_scale3d(unreal.Vector(*s))
    if folder:
        actor.set_folder_path(folder)
    if not collision:
        actor.static_mesh_component.set_collision_profile_name("NoCollision")
    return actor


# ------------------------------------------------------------------ foliage master material + instances
def param_scalar(mat, name, default, x, y):
    e = expr(mat, unreal.MaterialExpressionScalarParameter, x, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", default)
    return e


def param_vector(mat, name, rgb, x, y):
    e = expr(mat, unreal.MaterialExpressionVectorParameter, x, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    return e


def build_foliage_material():
    """Masked two-sided foliage: texture * tint, root->tip gradient from vertex color, wind on vertex color,
    world-space normals bendable towards up (grass lit like the ground)."""
    mat = new_material("M_Eldoria_Foliage")
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    mat.set_editor_property("two_sided", True)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
    mat.set_editor_property("tangent_space_normal", False)
    for usage in ("used_with_instanced_static_meshes", "used_with_static_lighting"):
        try:
            mat.set_editor_property(usage, True)
        except Exception:
            pass

    tex = expr(mat, unreal.MaterialExpressionTextureSampleParameter2D, -1400, -200)
    tex.set_editor_property("parameter_name", "BaseTex")
    default_tex = unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture")
    if default_tex:
        tex.set_editor_property("texture", default_tex)
    vc = expr(mat, unreal.MaterialExpressionVertexColor, -1400, 200)

    tint = param_vector(mat, "Tint", (1, 1, 1), -1200, -320)
    tip = param_vector(mat, "TipTint", (1, 1, 1), -1200, -60)
    root = param_scalar(mat, "RootDarkness", 0.55, -1200, 40)

    base = expr(mat, unreal.MaterialExpressionMultiply, -1000, -250)
    MEL.connect_material_expressions(tex, "RGB", base, "A")
    MEL.connect_material_expressions(tint, "", base, "B")
    root_col = expr(mat, unreal.MaterialExpressionMultiply, -800, -200)
    MEL.connect_material_expressions(base, "", root_col, "A")
    MEL.connect_material_expressions(root, "", root_col, "B")
    tip_col = expr(mat, unreal.MaterialExpressionMultiply, -800, -100)
    MEL.connect_material_expressions(base, "", tip_col, "A")
    MEL.connect_material_expressions(tip, "", tip_col, "B")
    color = lerp(mat, root_col, tip_col, vc, -600, -150, alpha_out="R")
    MEL.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)

    sss = mul_const(mat, color, 0.6, -400, -60)
    MEL.connect_material_property(sss, "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
    MEL.connect_material_property(tex, "A", unreal.MaterialProperty.MP_OPACITY_MASK)
    MEL.connect_material_property(param_scalar(mat, "Roughness", 0.85, -400, 40), "", unreal.MaterialProperty.MP_ROUGHNESS)

    # World-space normal: vertex normal bent towards up
    vn = expr(mat, unreal.MaterialExpressionVertexNormalWS, -800, 300)
    up = const3(mat, (0, 0, 1), -800, 380)
    up_amount = param_scalar(mat, "UpNormal", 0.0, -800, 460)
    normal = lerp(mat, vn, up, up_amount, -600, 360)
    norm = expr(mat, unreal.MaterialExpressionNormalize, -450, 360)
    MEL.connect_material_expressions(normal, "", norm, "")
    MEL.connect_material_property(norm, "", unreal.MaterialProperty.MP_NORMAL)

    # Wind: tips move, roots stay
    fn = unreal.load_asset("/Engine/Functions/Engine_MaterialFunctions01/WorldPositionOffset/SimpleGrassWind")
    if fn:
        wind = expr(mat, unreal.MaterialExpressionMaterialFunctionCall, -400, 600)
        wind.set_editor_property("material_function", fn)
        MEL.connect_material_expressions(param_scalar(mat, "WindStrength", 0.2, -700, 560), "", wind, "WindIntensity")
        weight = expr(mat, unreal.MaterialExpressionMultiply, -600, 640)
        MEL.connect_material_expressions(vc, "R", weight, "A")
        MEL.connect_material_expressions(param_scalar(mat, "WindWeight", 1.0, -750, 700), "", weight, "B")
        MEL.connect_material_expressions(weight, "", wind, "WindWeight")
        MEL.connect_material_expressions(param_scalar(mat, "WindSpeed", 0.35, -700, 760), "", wind, "WindSpeed")
        MEL.connect_material_expressions(const3(mat, (0, 0, 0), -700, 820), "", wind, "AdditionalWPO")
        MEL.connect_material_property(wind, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    return finish(mat)


def make_instance(name, parent, folder, textures=None, scalars=None, vectors=None):
    ensure_dir(folder)
    path = folder + "/" + name
    mi = unreal.load_asset(path) if EAL.does_asset_exist(path) else TOOLS.create_asset(
        name, folder, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, parent)
    for k, v in (textures or {}).items():
        MEL.set_material_instance_texture_parameter_value(mi, k, v)
    for k, v in (scalars or {}).items():
        MEL.set_material_instance_scalar_parameter_value(mi, k, v)
    for k, v in (vectors or {}).items():
        MEL.set_material_instance_vector_parameter_value(mi, k, unreal.LinearColor(v[0], v[1], v[2], 1.0))
    MEL.update_material_instance(mi)
    EAL.save_loaded_asset(mi)
    return mi
