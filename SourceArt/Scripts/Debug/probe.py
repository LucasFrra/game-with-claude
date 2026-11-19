# Debug probe: what does the camera see, how big are Tharum's parts.
import os
import unreal

out = []
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
for a in eas.get_all_level_actors():
    if isinstance(a, unreal.EldoriaTharum) or isinstance(a, unreal.EldoriaControlDevice):
        o, e = a.get_actor_bounds(False)
        out.append("%s origin %s extent %s" % (a.get_actor_label(), o, e))
        for c in a.get_components_by_class(unreal.StaticMeshComponent):
            m = c.static_mesh
            if m:
                b = m.get_bounds()
                out.append("  %-14s %-24s scale %s mesh extent %s world %s" % (
                    c.get_name(), m.get_name(), c.get_editor_property("relative_scale3d"), b.box_extent, c.get_world_location()))
for start, rot in [((-2400, 800, 2000), unreal.Rotator(roll=0, pitch=-35, yaw=-50)), ((-2854, 1683, 750), unreal.Rotator(roll=0, pitch=-2, yaw=-52))]:
    s = unreal.Vector(*start)
    e = s + rot.get_forward_vector() * 20000
    hit = unreal.SystemLibrary.line_trace_single(world, s, e, unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, True, [], unreal.DrawDebugTrace.NONE, True)
    if hit:
        t = hit.to_tuple()
        out.append("trace from %s hit %s comp %s at %s" % (start, t[9].get_actor_label() if t[9] else None, t[10].get_name() if t[10] else None, t[4]))
    else:
        out.append("trace from %s: nothing" % (start,))
with open(os.path.join(unreal.Paths.project_saved_dir(), "Probe.txt"), "w") as f:
    f.write("\n".join(out))
