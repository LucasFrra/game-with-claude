import os
import unreal
out = []
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if a.get_actor_label() == "Pool_Water":
        c = a.static_mesh_component
        o, e = a.get_actor_bounds(False)
        out.append("water mesh=%s mat=%s vis=%s origin=%s ext=%s" % (c.static_mesh.get_name(), c.get_material(0).get_name(), c.is_visible(), o, e))
for x, y in [(3300, 200), (3000, 200), (3600, 200), (3300, 600)]:
    h = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(x, y, 1000), unreal.Vector(x, y, -1000), unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [], unreal.DrawDebugTrace.NONE, True)
    t = h.to_tuple() if h else None
    out.append("ground at (%d,%d): %s %s" % (x, y, round(t[4].z) if t else None, t[9].get_actor_label() if t else ""))
with open(os.path.join(unreal.Paths.project_saved_dir(), "Pool.txt"), "w") as f:
    f.write("\n".join(str(x) for x in out))
