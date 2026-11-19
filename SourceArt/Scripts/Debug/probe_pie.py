# Lists Tharum's visual components in the running PIE world (meshes, scales, materials, visibility).
import os
import unreal

out = []
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
out.append("world %s" % world)
if world:
    for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.EldoriaTharum):
        out.append("%s at %s" % (a.get_name(), a.get_actor_location()))
        for c in a.get_components_by_class(unreal.StaticMeshComponent):
            m = c.static_mesh
            mats = [x.get_name() if x else None for x in c.get_materials()]
            out.append("  %-14s %-24s vis=%s scale=%s mats=%s" % (c.get_name(), m.get_name() if m else None, c.is_visible(),
                                                                   c.get_editor_property("relative_scale3d"), mats))
with open(os.path.join(unreal.Paths.project_saved_dir(), "ProbePIE.txt"), "w") as f:
    f.write("\n".join(out))
