# Align BP_Tharum (CDO) and the placed instance on the C++ layout for the sculpted meshes:
# full-size parts at scale 1, correct ear sides, body/head offsets. Stale overrides otherwise win.
import os
import unreal

FULL = {"Body": "SM_Tharum_Body", "Head": "SM_Tharum_Head", "Ear0": "SM_Tharum_EarB", "Ear1": "SM_Tharum_EarA",
        "Leg0": "SM_Tharum_Leg", "Leg1": "SM_Tharum_Leg", "Leg2": "SM_Tharum_Leg", "Leg3": "SM_Tharum_Leg",
        "Foot0": "SM_Tharum_Foot", "Foot1": "SM_Tharum_Foot", "Foot2": "SM_Tharum_Foot", "Foot3": "SM_Tharum_Foot"}
LOCS = {"Body": unreal.Vector(-80, 0, 955), "Head": unreal.Vector(200, 0, 0), "HeadCollider": unreal.Vector(220, 0, 0)}
out = []


def mesh(name):
    return unreal.load_asset("/Game/Project/Bosses/Tharum/Meshes/%s/StaticMeshes/%s.%s" % (name, name, name))


def fix(actor_like, components):
    for c in components:
        n = c.get_name()
        if n in FULL:
            c.set_editor_property("static_mesh", mesh(FULL[n]))
            c.set_editor_property("relative_scale3d", unreal.Vector(1, 1, 1))
        if n in LOCS:
            c.set_editor_property("relative_location", LOCS[n])
        if n == "Belly":
            c.set_editor_property("visible", False)
            c.set_editor_property("hidden_in_game", True)
        if n.startswith("Foot"):
            c.set_editor_property("override_materials", [])
    out.append("fixed %s" % actor_like)


bp = unreal.load_asset("/Game/Project/Bosses/Tharum/BP_Tharum")
cdo = unreal.get_default_object(bp.generated_class())
fix("CDO", [cdo.get_editor_property(p) for p in ("body_mesh", "belly_mesh", "head_mesh")]
    + list(cdo.get_editor_property("ear_meshes")) + list(cdo.get_editor_property("leg_meshes"))
    + list(cdo.get_editor_property("foot_meshes")))
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
unreal.EditorAssetLibrary.save_loaded_asset(bp)

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in eas.get_all_level_actors():
    if isinstance(a, unreal.EldoriaTharum):
        fix(a.get_actor_label(), a.get_components_by_class(unreal.SceneComponent))
        a.modify()
        for c in a.get_components_by_class(unreal.StaticMeshComponent):
            if c.static_mesh and c.get_name() in FULL:
                out.append("  %s %s %s" % (c.get_name(), c.static_mesh.get_name(), c.get_editor_property("relative_scale3d")))
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
with open(os.path.join(unreal.Paths.project_saved_dir(), "FixTharum.txt"), "w") as f:
    f.write("\n".join(out))
