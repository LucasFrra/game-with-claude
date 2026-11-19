import os
import unreal
out = []
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
for p in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.EldoriaCharacter):
    ic = p.get_component_by_class(unreal.EldoriaInteractionComponent)
    t = ic.get_focused_target() if ic else None
    out.append("player %s focus=%s" % (p.get_actor_location(), t.get_name() if t else None))
for d in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.EldoriaControlDevice):
    core = [c for c in d.get_components_by_class(unreal.StaticMeshComponent) if c.get_name() == "Core"]
    if core:
        out.append("core at %s" % core[0].get_world_location())
with open(os.path.join(unreal.Paths.project_saved_dir(), "Focus.txt"), "a") as f:
    f.write("\n".join(out) + "\n")
