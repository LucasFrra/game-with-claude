# For every collapsible structure in PIE: how far its pieces went and how tilted the main body is.
import os
import unreal
out = []
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.EldoriaCollapsibleStructure):
    origin = a.get_actor_location()
    far = 0.0
    tilt = 0.0
    for c in a.get_components_by_class(unreal.StaticMeshComponent):
        d = (c.get_world_location() - origin).length() / 100
        far = max(far, d)
        up = c.get_up_vector()
        tilt = max(tilt, unreal.MathLibrary.radians_to_degrees(unreal.MathLibrary.acos(max(-1.0, min(1.0, up.z)))))
    out.append("%-28s collapsed=%s farthest piece %.1f m, max tilt %.0f deg" % (a.get_actor_label() if hasattr(a, "get_actor_label") else a.get_name(), a.is_collapsed(), far, tilt))
with open(os.path.join(unreal.Paths.project_saved_dir(), "Topple.txt"), "a") as f:
    f.write("\n".join(out) + "\n---\n")
