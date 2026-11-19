# Clear the north tunnel of scattered dressing rocks and make the FORCE boulder a tight, sliding plug.
import os
import unreal
out = []
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
for a in eas.get_all_level_actors():
    if a.get_actor_label() in ("Rock_Dress_024", "Rock_Dress_014"):
        out.append("removed %s" % a.get_actor_label())
        eas.destroy_actor(a)
for a in eas.get_all_level_actors():
    if a.get_actor_label() == "ForceTarget_TunnelBoulder":
        m = a.get_editor_property("mesh")
        # wider than tall-ish: ~5.7 m across at knee height in a ~6.3 m tunnel, so nobody slips past
        a.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=0.0), False)
        a.set_actor_scale3d(unreal.Vector(1.84, 2.3, 1.84))
        bi = m.get_editor_property("body_instance")
        for prop, value in [("dof_mode", unreal.DOFMode.SIX_DOF), ("lock_y_translation", True),
                            ("lock_x_rotation", True), ("lock_y_rotation", True), ("lock_z_rotation", True)]:
            try:
                bi.set_editor_property(prop, value)
            except Exception as exc:
                out.append("body %s: %r" % (prop, exc))
        m.set_editor_property("body_instance", bi)
        a.set_actor_location(unreal.Vector(5100, 0, 0), False, True)
        o, e = a.get_actor_bounds(False)
        g = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(5100, 0, 450), unreal.Vector(5100, 0, -1000),
                                                   unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [a], unreal.DrawDebugTrace.NONE, True)
        ground = g.to_tuple()[4].z if g else 0.0
        a.set_actor_location(unreal.Vector(5100, 0, ground - (o.z - e.z) - 5), False, True)
        o, e = a.get_actor_bounds(False)
        for side in (1, -1):
            h = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(5100, side * 900, 60), unreal.Vector(5100, 0, 60),
                                                       unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [], unreal.DrawDebugTrace.NONE, True)
            out.append("rock edge side %+d at knee: y=%s" % (side, round(h.to_tuple()[4].y) if h else None))
        out.append("boulder at %s extent %s" % (a.get_actor_location(), e))
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
with open(os.path.join(unreal.Paths.project_saved_dir(), "FixTunnel.txt"), "w") as f:
    f.write("\n".join(str(x) for x in out))
