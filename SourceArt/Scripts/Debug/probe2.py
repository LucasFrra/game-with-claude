import os
import unreal

out = []
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in eas.get_all_level_actors():
    lbl = a.get_actor_label()
    if lbl.startswith(("Pool_", "Block_RouteB", "Lift_", "Collapsible_RuinTower", "Encounter_", "Boulder_", "Aqueduct", "Stair")):
        line = "%-28s %s" % (lbl, a.get_actor_location())
        if lbl.startswith("Collapsible_RuinTower"):
            line += " fixed=%s dir=%s" % (a.get_editor_property("use_fixed_fall_direction"), a.get_editor_property("fixed_fall_direction"))
        if lbl.startswith("Encounter_"):
            line += " deity=%s" % a.get_editor_property("deity")
        out.append(line)
with open(os.path.join(unreal.Paths.project_saved_dir(), "Probe2.txt"), "w") as f:
    f.write("\n".join(sorted(out)))
