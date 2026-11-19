import os
import unreal
out = []
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.EldoriaTharum):
    out.append("tharum %s yaw %.0f" % (a.get_actor_location(), a.get_actor_rotation().yaw))
    for c in a.get_components_by_class(unreal.BoxComponent):
        o, e, r = unreal.SystemLibrary.get_component_bounds(c)
        out.append("  %s origin %s extent %s" % (c.get_name(), o, e))
for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.StaticMeshActor):
    if a.get_actor_label() == "Aqueduct_Deck":
        o, e = a.get_actor_bounds(False)
        out.append("deck origin %s extent %s" % (o, e))
with open(os.path.join(unreal.Paths.project_saved_dir(), "ProbePIE3.txt"), "w") as f:
    f.write("\n".join(str(x) for x in out))
