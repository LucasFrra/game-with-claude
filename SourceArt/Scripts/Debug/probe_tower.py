import os
import unreal
out = []
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.EldoriaCollapsibleStructure):
    if a.get_name().endswith("1215057704"):
        out.append("%s label=%s" % (a.get_name(), a.get_actor_label()))
        for c in a.get_components_by_class(unreal.PrimitiveComponent):
            o, e, r = unreal.SystemLibrary.get_component_bounds(c)
            out.append("  %-24s origin (%.0f, %.0f, %.0f) ext (%.0f, %.0f, %.0f) coll=%s" % (c.get_name(), o.x, o.y, o.z, e.x, e.y, e.z, c.get_collision_enabled()))
for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.EldoriaTharum):
    out.append("tharum %s yaw %.0f" % (a.get_actor_location(), a.get_actor_rotation().yaw))
with open(os.path.join(unreal.Paths.project_saved_dir(), "ProbeTower.txt"), "w") as f:
    f.write("\n".join(out))
