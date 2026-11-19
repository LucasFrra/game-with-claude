# Project Eldoria - J1 art direction: painterly cel-shading post-process, sky/fog/light and grading.
# See docs/ART_DIRECTION.md. Safe to run again (rebuilds the material, re-applies settings).
import os
import sys
import importlib

sys.path.append(os.path.dirname(__file__))
import unreal
import eldoria_lib as L
importlib.reload(L)

MEL = unreal.MaterialEditingLibrary
PP_DIR = "/Game/Project/Materials/PostProcess"
REPORT = os.path.join(unreal.Paths.project_saved_dir(), "ArtDirection.txt")
log = []

# Shading model (HLSL). Lighting is recovered as SceneColor / BaseColor, then remapped onto a soft
# 3-tone ramp (tinted shadow / lit / highlight). Distant pixels fade back to the scene so the height
# fog keeps the aerial perspective. Silhouettes get a thin outline darkened from the scene colour.
TOON_HLSL = r"""
float2 UV = GetDefaultSceneTextureUV(Parameters, 14);
float3 scene = Scene.rgb;
float3 result = scene;
float3 base = Base.rgb;
float depth = Depth.r;
float3 shadowTint = ShadowTint.rgb;
float3 lightTint = LightTint.rgb;
float3 outlineColor = OutlineColor.rgb;
bool sky = depth > 1.0e6;
float baseSum = base.r + base.g + base.b;
if (!sky && baseSum > 0.004)
{
    float3 lit = scene / max(base, 0.012);
    float l = dot(lit, float3(0.2126, 0.7152, 0.0722));
    if (l < EmissiveCut)
    {
        float s = smoothstep(Threshold - Softness, Threshold + Softness, l);
        float h = smoothstep(HiThreshold - Softness, HiThreshold + Softness, l);
        float3 shade = lerp(base * shadowTint * ShadowLevel, base * lightTint * LitLevel, s);
        shade = lerp(shade, base * lightTint * HiLevel, h * HiAmount);
        // keep a little of the real light colour/bounce so it stays alive
        float3 lightColor = lit / max(l, 0.001);
        shade *= lerp(float3(1, 1, 1), saturate(lightColor), ColorKeep);
        float fade = 1.0 - smoothstep(FadeStart, FadeEnd, depth);
        result = lerp(scene, shade, Strength * fade);
    }
}

// Silhouette outline: a neighbour much farther away means we sit on the edge of a closer object
float2 texel = View.BufferSizeAndInvSize.zw * OutlineWidth;
float dR = SceneTextureLookup(UV + float2(texel.x, 0), 1, false).r;
float dL = SceneTextureLookup(UV - float2(texel.x, 0), 1, false).r;
float dU = SceneTextureLookup(UV + float2(0, texel.y), 1, false).r;
float dD = SceneTextureLookup(UV - float2(0, texel.y), 1, false).r;
float far = max(max(dR, dL), max(dU, dD));
float edge = saturate(((far - depth) / max(depth, 1.0) - OutlineDepth) * 4.0);
edge *= 1.0 - smoothstep(OutlineFadeStart, OutlineFadeEnd, depth);
if (sky) { edge = 0; }
result = lerp(result, result * outlineColor, edge * OutlineStrength);
return result;
"""

PARAMS = {  # name: default
    "Strength": 0.8, "Threshold": 0.3, "HiThreshold": 1.25, "Softness": 0.14, "HiAmount": 0.35,
    "ShadowLevel": 0.58, "LitLevel": 1.0, "HiLevel": 1.18, "ColorKeep": 0.5, "EmissiveCut": 6.0,
    "FadeStart": 3500.0, "FadeEnd": 14000.0,
    "OutlineWidth": 1.5, "OutlineDepth": 0.18, "OutlineStrength": 0.6, "OutlineFadeStart": 2500.0, "OutlineFadeEnd": 9000.0,
}
VECTORS = {"ShadowTint": (0.70, 0.74, 0.92), "LightTint": (1.04, 1.0, 0.93), "OutlineColor": (0.28, 0.27, 0.34)}


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


def set_enum(obj, prop, names):
    for n in names:
        v = getattr(unreal.BlendableLocation, n, None) if prop == "blendable_location" else None
        if v is not None:
            obj.set_editor_property(prop, v)
            return n
    return None


@step("toon post-process material")
def build_toon():
    L.ensure_dir(PP_DIR)
    path = PP_DIR + "/M_PP_EldoriaToon"
    if L.EAL.does_asset_exist(path):
        mat = unreal.load_asset(path)          # reuse: deleting a referenced material pops a dialog
        MEL.delete_all_material_expressions(mat)
    else:
        mat = L.TOOLS.create_asset("M_PP_EldoriaToon", PP_DIR, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
    loc = set_enum(mat, "blendable_location", ["BL_SCENE_COLOR_BEFORE_BLOOM", "BL_SCENE_COLOR_AFTER_DOF", "BL_BEFORE_TONEMAPPING"])

    custom = L.expr(mat, unreal.MaterialExpressionCustom, -300, 0)
    custom.set_editor_property("code", TOON_HLSL)
    custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    names = ["Scene", "Base", "Depth"] + list(PARAMS) + list(VECTORS)
    def custom_input(n):
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", n)
        return ci
    custom.set_editor_property("inputs", [custom_input(n) for n in names])

    def scene_tex(scene_id, y):
        e = L.expr(mat, unreal.MaterialExpressionSceneTexture, -900, y)
        e.set_editor_property("scene_texture_id", scene_id)
        return e
    ST = unreal.SceneTextureId
    MEL.connect_material_expressions(scene_tex(ST.PPI_POST_PROCESS_INPUT0, -300), "Color", custom, "Scene")
    MEL.connect_material_expressions(scene_tex(ST.PPI_BASE_COLOR, -200), "Color", custom, "Base")
    MEL.connect_material_expressions(scene_tex(ST.PPI_SCENE_DEPTH, -100), "Color", custom, "Depth")
    y = 0
    for n, v in PARAMS.items():
        MEL.connect_material_expressions(L.param_scalar(mat, n, v, -900, y), "", custom, n)
        y += 60
    for n, v in VECTORS.items():
        MEL.connect_material_expressions(L.param_vector(mat, n, v, -900, y), "", custom, n)
        y += 80
    MEL.connect_material_property(custom, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    L.finish(mat)
    mi = L.make_instance("MI_PP_EldoriaToon", mat, PP_DIR)
    return "%s (%s)" % (mi.get_path_name(), loc)


@step("world: light, sky, fog, grading")
def world(mi):
    for a in L.EAS.get_all_level_actors():
        lbl = a.get_actor_label()
        if lbl == "DirectionalLight":
            c = a.light_component
            c.set_editor_property("intensity", 7.0)
            c.set_editor_property("temperature", 5700.0)
            c.set_editor_property("use_temperature", True)
            c.set_editor_property("light_source_angle", 2.0)
        elif lbl == "SkyLight":
            a.light_component.set_editor_property("intensity", 1.6)
            a.light_component.set_editor_property("lower_hemisphere_color", unreal.LinearColor(0.16, 0.2, 0.12, 1))
        elif lbl == "ExponentialHeightFog":
            c = a.component
            # BOTW: distances melt into a pale sky blue
            c.set_editor_property("fog_density", 0.06)
            c.set_editor_property("fog_height_falloff", 0.03)
            c.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(0.55, 0.72, 0.95, 1.0))
            c.set_editor_property("directional_inscattering_luminance", unreal.LinearColor(0.9, 0.75, 0.5, 1.0))
            c.set_editor_property("directional_inscattering_exponent", 6.0)
            c.set_editor_property("start_distance", 1500.0)
            c.set_editor_property("fog_max_opacity", 0.85)
        elif lbl == "PostProcessVolume":
            a.set_editor_property("unbound", True)
            s = a.get_editor_property("settings")
            values = [
                ("bloom_intensity", 0.55), ("bloom_threshold", 1.2), ("vignette_intensity", 0.22),
                ("auto_exposure_bias", 0.25), ("ambient_occlusion_intensity", 0.35),
                ("color_saturation", unreal.Vector4(1.04, 1.04, 1.04, 1.0)),
                ("color_contrast", unreal.Vector4(0.94, 0.94, 0.94, 1.0)),
                ("color_gain_shadows", unreal.Vector4(0.94, 0.98, 1.1, 1.0)),
                ("color_gain_highlights", unreal.Vector4(1.04, 1.01, 0.95, 1.0)),
                ("white_temp", 6200.0), ("film_toe", 0.6), ("motion_blur_amount", 0.15),
            ]
            for prop, value in values:
                try:
                    s.set_editor_property("override_" + prop, True)
                    s.set_editor_property(prop, value)
                except Exception as exc:
                    log.append("  pp %s: %r" % (prop, exc))
            wb = unreal.WeightedBlendable()
            wb.set_editor_property("weight", 1.0)
            wb.set_editor_property("object", mi)
            wbs = unreal.WeightedBlendables()
            wbs.set_editor_property("array", [wb])
            s.set_editor_property("weighted_blendables", wbs)
            a.set_editor_property("settings", s)
        elif isinstance(a, unreal.EldoriaEncounterManager):
            # Under the device's grip the jungle is greyer and hazier; freed, it gets the full palette
            for prop, value in [("controlled_saturation", 0.9), ("freed_saturation", 1.12),
                                ("controlled_fog_density", 0.08), ("freed_fog_density", 0.06),
                                ("controlled_sun_intensity", 5.5), ("freed_sun_intensity", 7.0)]:
                a.set_editor_property(prop, value)


def run():
    r = build_toon()
    mi = unreal.load_asset(PP_DIR + "/MI_PP_EldoriaToon")
    if mi:
        world(mi)
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    L.EAL.save_directory("/Game/Project", only_if_is_dirty=True, recursive=True)


try:
    run()
except Exception as exc:
    log.append("[FATAL] %r" % exc)
with open(REPORT, "w") as f:
    f.write("\n".join(log))
