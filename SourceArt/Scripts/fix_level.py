# Route B layout: pool where Tharum drinks (33, 2), the push block next to the stairs' high first step.
import math
import os
import unreal

out = []
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = {a.get_actor_label(): a for a in eas.get_all_level_actors()}


def move(a, x, y, z):
    a.modify()
    root = a.root_component
    if root:
        root.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE if a.get_actor_label().startswith("Block_") else root.get_editor_property("mobility"))
    a.set_actor_location(new_location=unreal.Vector(x, y, z), sweep=False, teleport=True)
    l = a.get_actor_location()
    out.append("%-20s -> (%.0f, %.0f, %.0f)" % (a.get_actor_label(), l.x, l.y, l.z))


water = actors.get("Pool_Water")
if water:
    move(water, 3300, 200, -25)
    water.set_actor_scale3d(unreal.Vector(12.5, 12.5, 0.06))
for k in range(9):
    a = actors.get("Pool_RimStone_%d" % k)
    if a:
        ang = math.radians(k * 40 + 10)
        move(a, 3300 + 680 * math.cos(ang), 200 + 680 * math.sin(ang), 15)

step = actors.get("Stair_Step0_High")
block = actors.get("Block_RouteB")
if step and block:
    o, e = step.get_actor_bounds(False)
    out.append("step0 origin %s extent %s" % (o, e))
    # in front of the step's west face, on the way in from the clearing
    move(block, o.x - e.x - 160, o.y, 62)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
with open(os.path.join(unreal.Paths.project_saved_dir(), "FixLevel.txt"), "w") as f:
    f.write("\n".join(out))
