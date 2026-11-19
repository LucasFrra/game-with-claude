# Traces through the trunk and under the crown of one forest tree instance and one hero tree.
import os
import unreal
out = []
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
def trace(a, b):
    h = unreal.SystemLibrary.line_trace_single(world, a, b, unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [], unreal.DrawDebugTrace.NONE, True)
    if not h:
        return "nothing"
    t = h.to_tuple()
    return "%s at %.0f cm" % (t[9].get_actor_label() if t[9] else "?", (t[4] - a).length())
for a in eas.get_all_level_actors():
    if isinstance(a, unreal.InstancedFoliageActor):
        for c in a.get_components_by_class(unreal.InstancedStaticMeshComponent):
            m = c.static_mesh
            if m and m.get_name().startswith("SM_Tree_") and c.get_instance_count() > 0:
                tr = c.get_instance_transform(0, True)
                p = tr.translation
                s = tr.scale3d.x
                out.append("%s inst0 at %s scale %.2f profile=%s" % (m.get_name(), p, s, c.get_collision_profile_name()))
                out.append("  chest: " + trace(p + unreal.Vector(-600, 0, 120), p + unreal.Vector(600, 0, 120)))
                out.append("  under crown (r=2.5m): " + trace(p + unreal.Vector(-600, 250 * s, 120), p + unreal.Vector(600, 250 * s, 120)))
                break
    elif a.get_actor_label() == "HeroTree_00":
        p = a.get_actor_location()
        out.append("hero %s" % p)
        out.append("  chest: " + trace(p + unreal.Vector(-800, 0, 120), p + unreal.Vector(800, 0, 120)))
with open(os.path.join(unreal.Paths.project_saved_dir(), "TreeCollision.txt"), "w") as f:
    f.write("\n".join(str(x) for x in out))
