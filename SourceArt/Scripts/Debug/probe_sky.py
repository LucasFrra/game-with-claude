import os
import unreal
out = []
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    c = a.get_class().get_name()
    if any(k in c for k in ("Sky", "Cloud", "Fog", "Light", "PostProcess", "Atmosphere")):
        out.append("%-30s %-28s %s" % (a.get_actor_label(), c, a.get_actor_location()))
with open(os.path.join(unreal.Paths.project_saved_dir(), "Sky.txt"), "w") as f:
    f.write("\n".join(out))
