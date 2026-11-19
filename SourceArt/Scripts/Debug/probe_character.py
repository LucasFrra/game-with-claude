import os
import unreal
out = []
for path in ["/Game/Project/Characters/BP_EldoriaCharacter", "/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"]:
    bp = unreal.load_asset(path)
    if not bp:
        out.append("%s missing" % path); continue
    cdo = unreal.get_default_object(bp.generated_class())
    m = cdo.get_editor_property("mesh")
    out.append("%s mesh=%s anim_class=%s mode=%s" % (path, m.get_editor_property("skeletal_mesh_asset") if hasattr(m, "skeletal_mesh_asset") else m.get_skeletal_mesh_asset(),
               m.get_editor_property("anim_class"), m.get_editor_property("animation_mode")))
reg = unreal.AssetRegistryHelpers.get_asset_registry()
for a in reg.get_assets_by_path("/Game/Characters", recursive=True):
    if str(a.asset_class_path.asset_name) in ("AnimBlueprint", "AnimSequence", "BlendSpace", "SkeletalMesh"):
        out.append("  %s %s" % (a.asset_class_path.asset_name, a.package_name))
with open(os.path.join(unreal.Paths.project_saved_dir(), "Character.txt"), "w") as f:
    f.write("\n".join(str(x) for x in out[:60]))
