import os
import unreal
out = []
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ignore = [a for a in eas.get_all_level_actors() if a.get_actor_label().startswith("ForceTarget_")]
for x in range(4400, 6800, 200):
    row = []
    for z in (60, 250):
        ys = []
        for side in (1, -1):
            h = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(x, 0, z), unreal.Vector(x, side * 1500, z),
                                                       unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, ignore, unreal.DrawDebugTrace.NONE, True)
            ys.append(round(h.to_tuple()[4].y) if h else None)
        row.append("z%d: %s" % (z, ys))
    g = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(x, 0, 400), unreal.Vector(x, 0, -500),
                                               unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, ignore, unreal.DrawDebugTrace.NONE, True)
    out.append("x=%d floor=%s %s" % (x, round(g.to_tuple()[4].z) if g else None, "  ".join(row)))
with open(os.path.join(unreal.Paths.project_saved_dir(), "Tunnel.txt"), "w") as f:
    f.write("\n".join(out))
