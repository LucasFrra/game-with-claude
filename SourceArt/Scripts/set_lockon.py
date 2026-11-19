import os
import unreal
bp = unreal.load_asset("/Game/Project/Characters/BP_EldoriaCharacter")
cdo = unreal.get_default_object(bp.generated_class())
cdo.set_editor_property("lock_on_action", unreal.load_asset("/Game/Project/Input/IA_LockOn"))
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
unreal.EditorAssetLibrary.save_loaded_asset(bp)
imc = unreal.load_asset("/Game/Project/Input/IMC_Eldoria")
keys = [str(m.get_editor_property("key").get_editor_property("key_name")) for m in imc.get_editor_property("default_key_mappings").get_editor_property("mappings")]
with open(os.path.join(unreal.Paths.project_saved_dir(), "LockOn.txt"), "w") as f:
    f.write("lock_on=%s keys=%s" % (cdo.get_editor_property("lock_on_action"), keys))
