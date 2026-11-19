# J2 audit: every static mesh used in the level (actors + foliage) with its size and collision.
import os
import unreal

out = []
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
usage = {}


def note(mesh, where, comp=None):
    if not mesh:
        return
    key = mesh.get_path_name()
    u = usage.setdefault(key, {"mesh": mesh, "count": 0, "where": set(), "comp_collision": set()})
    u["count"] += 1
    u["where"].add(where)
    if comp is not None:
        u["comp_collision"].add(str(comp.get_collision_enabled()).split(".")[-1])


for a in eas.get_all_level_actors():
    for c in a.get_components_by_class(unreal.StaticMeshComponent):
        if isinstance(c, unreal.InstancedStaticMeshComponent):
            note(c.static_mesh, "foliage:" + a.get_actor_label(), c)
        else:
            note(c.static_mesh, a.get_class().get_name(), c)

for key, u in sorted(usage.items(), key=lambda kv: -kv[1]["count"]):
    mesh = u["mesh"]
    b = mesh.get_bounds()
    size = (b.box_extent.x * 2 / 100, b.box_extent.y * 2 / 100, b.box_extent.z * 2 / 100)
    body = mesh.get_editor_property("body_setup")
    simple = 0
    complex_flag = ""
    if body:
        g = body.get_editor_property("agg_geom")
        simple = sum(len(g.get_editor_property(k)) for k in ("box_elems", "sphere_elems", "sphyl_elems", "convex_elems"))
        complex_flag = str(body.get_editor_property("collision_trace_flag")).split(".")[-1]
    out.append("%-28s n=%-5d size=(%.1f,%.1f,%.1f)m simple=%d trace=%s comp=%s in=%s" % (
        mesh.get_name(), u["count"], size[0], size[1], size[2], simple, complex_flag,
        ",".join(sorted(u["comp_collision"])), ",".join(sorted(u["where"]))[:60]))
with open(os.path.join(unreal.Paths.project_saved_dir(), "AuditCollisions.txt"), "w") as f:
    f.write("\n".join(out))
