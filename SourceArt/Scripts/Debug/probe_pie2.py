import os
import unreal
out = []
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.EldoriaPhysicsObject):
    m = a.get_editor_property("mesh") if hasattr(a, "mesh") else None
    o, e = a.get_actor_bounds(False)
    out.append("%-34s loc=%s ext=%s mass=%s" % (a.get_actor_label() if hasattr(a, "get_actor_label") else a.get_name(), a.get_actor_location(), e, a.get_mass_kg()))
for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.EldoriaCharacter):
    out.append("player %s" % a.get_actor_location())
with open(os.path.join(unreal.Paths.project_saved_dir(), "ProbePIE2.txt"), "w") as f:
    f.write("\n".join(out))
