import os
import unreal
out = []
for p in ["/Game/Project/Environment/Stylized"]:
    for path in unreal.EditorAssetLibrary.list_assets(p, recursive=True):
        m = unreal.load_asset(path)
        if isinstance(m, unreal.StaticMesh) and m.get_name().startswith("SM_Tree"):
            b = m.get_editor_property("body_setup")
            g = b.get_editor_property("agg_geom")
            caps = g.get_editor_property("sphyl_elems")
            out.append("%s sphyl=%d convex=%d box=%d flag=%s" % (m.get_name(), len(caps), len(g.get_editor_property("convex_elems")), len(g.get_editor_property("box_elems")), b.get_editor_property("collision_trace_flag")))
            for c in caps:
                out.append("   center=%s r=%s len=%s" % (c.get_editor_property("center"), c.get_editor_property("radius"), c.get_editor_property("length")))
            out.append("   simple count via subsystem: %s" % unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem).get_simple_collision_count(m))
with open(os.path.join(unreal.Paths.project_saved_dir(), "Body.txt"), "w") as f:
    f.write("\n".join(str(x) for x in out))
