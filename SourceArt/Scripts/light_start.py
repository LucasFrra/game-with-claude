# Brighter arrival corridor: sun along the canyon axis + soft unshadowed fill lights.
import os
import sys
sys.path.append(os.path.dirname(__file__))
import unreal
import eldoria_lib as L

out = []
for a in L.EAS.get_all_level_actors():
    lbl = a.get_actor_label()
    if lbl == "DirectionalLight":
        # light travels east (+X) along the corridor, from behind the arriving player
        a.set_actor_rotation(unreal.Rotator(pitch=-62.0, yaw=12.0, roll=0.0), False)
        out.append("sun")
    elif lbl == "SkyLight":
        a.light_component.set_editor_property("intensity", 2.0)

L.destroy(["Light_CorridorFill_"])
for i, (x, y, z) in enumerate([(-9300, 0, 900), (-8200, 150, 700), (-7100, -100, 600), (-6000, 0, 500)]):
    p = L.EAS.spawn_actor_from_class(unreal.PointLight, unreal.Vector(x, y, z))
    p.set_actor_label("Light_CorridorFill_%d" % i)
    p.set_folder_path("Jungle/Lighting")
    c = p.point_light_component
    c.set_editor_property("intensity", 9000.0)
    c.set_editor_property("intensity_units", unreal.LightUnits.UNITLESS)
    c.set_editor_property("attenuation_radius", 1600.0)
    c.set_editor_property("light_color", unreal.Color(r=255, g=226, b=190, a=255))
    c.set_editor_property("cast_shadows", False)
    c.set_editor_property("source_radius", 200.0)
    out.append(p.get_actor_label())
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
with open(os.path.join(unreal.Paths.project_saved_dir(), "LightStart.txt"), "w") as f:
    f.write("\n".join(out))
