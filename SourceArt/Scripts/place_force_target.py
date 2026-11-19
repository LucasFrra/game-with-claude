# Post-boss test: the boulder plugging the north tunnel. Only FORCE (strength 3) can move it.
# Named ForceTarget_* so the dressing scripts (which clear "Boulder_*") never delete it again.
import os
import sys
sys.path.append(os.path.dirname(__file__))
import unreal
import eldoria_lib as L

out = []
L.destroy(["ForceTarget_", "Boulder_ForceTarget", "Tunnel_Boulder"])
rock = L.nature_mesh("Rock_Medium_2")
b = rock.get_bounds()
size = max(b.box_extent.x, b.box_extent.y, b.box_extent.z) * 2.0
scale = 560.0 / max(size, 1.0)
body = rock.get_editor_property("body_setup")
geom = body.get_editor_property("agg_geom") if body else None
n_convex = len(geom.get_editor_property("convex_elems")) if geom else 0
n_simple = (len(geom.get_editor_property("sphere_elems")) + len(geom.get_editor_property("box_elems"))) if geom else 0
out.append("rock %s size %.0f cm -> scale %.2f, convex %d simple %d" % (rock.get_name(), size, scale, n_convex, n_simple))

a = L.EAS.spawn_actor_from_class(unreal.EldoriaPhysicsObject, unreal.Vector(4900, 0, 300))
a.set_actor_label("ForceTarget_TunnelBoulder")
a.set_folder_path("Jungle/Gameplay")
m = a.get_editor_property("mesh")
m.set_static_mesh(rock if (n_convex or n_simple) else unreal.load_asset("/Engine/BasicShapes/Sphere"))
a.set_actor_scale3d(unreal.Vector(scale, scale, scale) if (n_convex or n_simple) else unreal.Vector(5.6, 5.6, 5.6))
stone = unreal.load_asset("/Game/Project/Materials/Master/M_Eldoria_RuinStone")
if not (n_convex or n_simple) and stone:
    m.set_material(0, stone)
for prop, value in [("mass_kg", 20000.0), ("simulate_when_idle", False), ("grabbable", False), ("pushable", True),
                    ("required_strength", 3.0), ("push_speed", 90.0)]:
    a.set_editor_property(prop, value)
o, e = a.get_actor_bounds(False)
# rest it on the tunnel floor
a.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=0.0), False)   # narrow side across: room to slide, still no way past on foot
o, e = a.get_actor_bounds(False)
a.set_actor_location(unreal.Vector(4900, 0, 0), False, True)
o, e = a.get_actor_bounds(False)
ground = 0.0
hit = unreal.SystemLibrary.line_trace_single(unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world(),
                                             unreal.Vector(4900, 0, 450), unreal.Vector(4900, 0, -1000),
                                             unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [a], unreal.DrawDebugTrace.NONE, True)
if hit:
    ground = hit.to_tuple()[4].z
# the mesh pivot is not at its centre: lift so that the bounds' bottom sits 5 cm into the ground
a.set_actor_location(unreal.Vector(4900, 0, ground - (o.z - e.z) - 5), False, True)
out.append("ground %.0f" % ground)
out.append("placed at %s extent %s" % (a.get_actor_location(), e))
out.append("stele: %s" % [x.get_actor_label() for x in L.find_actors(["Inspect_Shrine"])])
# free width on each side of the rock at knee height (player capsule is ~84 cm wide)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
for side in (1, -1):
    for xs in (4800, 4900, 5000):
        h1 = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(xs, 0, 60), unreal.Vector(xs, side * 900, 60),
                                                    unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [a], unreal.DrawDebugTrace.NONE, True)
        h2 = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(xs, side * 900, 60), unreal.Vector(xs, 0, 60),
                                                    unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [], unreal.DrawDebugTrace.NONE, True)
        wall = h1.to_tuple()[4].y if h1 else None
        rock = h2.to_tuple()[4].y if h2 else None
        out.append("x=%d side %+d: wall y=%s, rock edge y=%s" % (xs, side, wall, rock))
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
with open(os.path.join(unreal.Paths.project_saved_dir(), "ForceTarget.txt"), "w") as f:
    f.write("\n".join(str(x) for x in out))
