# Route A: the weakened tower falls the way it is rammed (away from Tharum). Bringing it down knocks
# Tharum to its knees (heavy stun): its lowered head and neck become the ramp to the device.
import os
import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
out = []
for a in eas.get_all_level_actors():
    if a.get_actor_label().startswith("Collapsible_RuinTower"):
        a.set_editor_property("fall_toward_instigator", False)
        a.set_editor_property("use_fixed_fall_direction", False)
        out.append("%s toward=%s fixed=%s" % (a.get_actor_label(), a.get_editor_property("fall_toward_instigator"), a.get_editor_property("use_fixed_fall_direction")))
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
with open(os.path.join(unreal.Paths.project_saved_dir(), "TowerFall.txt"), "w") as f:
    f.write("\n".join(out))
