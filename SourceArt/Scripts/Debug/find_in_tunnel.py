import os
import unreal
out = []
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
box = unreal.Box(unreal.Vector(5400, -500, -100), unreal.Vector(6500, 500, 800)) if hasattr(unreal, "Box") else None
for a in eas.get_all_level_actors():
    o, e = a.get_actor_bounds(False)
    if e.x > 20000:   # terrain, sky...
        continue
    if o.x + e.x > 5400 and o.x - e.x < 6500 and o.y + e.y > -450 and o.y - e.y < 450 and o.z - e.z < 600:
        out.append("%-40s %-28s origin (%.0f, %.0f, %.0f) ext (%.0f, %.0f, %.0f)" % (a.get_actor_label(), a.get_class().get_name(), o.x, o.y, o.z, e.x, e.y, e.z))
with open(os.path.join(unreal.Paths.project_saved_dir(), "InTunnel.txt"), "w") as f:
    f.write("\n".join(out))
