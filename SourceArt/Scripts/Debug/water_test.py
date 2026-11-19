import unreal
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if a.get_actor_label() == "Pool_Water":
        a.set_actor_location(unreal.Vector(3300, 200, -10), False, True)
