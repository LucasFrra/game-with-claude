import os
import unreal
out = []
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.StaticMeshActor):
    if a.get_name().endswith(("1875441430", "1879441434")):
        o, e = a.get_actor_bounds(False)
        out.append("%s label=%s mesh=%s origin=%s extent=%s" % (a.get_name(), a.get_actor_label(), a.static_mesh_component.static_mesh.get_name(), o, e))
with open(os.path.join(unreal.Paths.project_saved_dir(), "ProbeName.txt"), "w") as f:
    f.write("\n".join(out))
