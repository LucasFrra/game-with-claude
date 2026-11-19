import os
import unreal
out = []
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if a.get_actor_label() in ("VolumetricCloud", "SkyAtmosphere", "ExponentialHeightFog", "SkyLight"):
        for c in a.get_components_by_class(unreal.ActorComponent):
            for prop in ("layer_bottom_altitude", "layer_height", "material", "rayleigh_scattering", "mie_scattering_scale", "fog_cutoff_distance", "source_type", "real_time_capture", "cubemap", "sky_luminance_factor", "multi_scattering_factor"):
                try:
                    out.append("%s.%s = %s" % (a.get_actor_label(), prop, c.get_editor_property(prop)))
                except Exception:
                    pass
with open(os.path.join(unreal.Paths.project_saved_dir(), "Cloud.txt"), "w") as f:
    f.write("\n".join(str(x) for x in out))
