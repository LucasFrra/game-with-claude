# J4: exports the mannequin skeletal mesh and the idle animation to FBX so Blender can author new
# animations on the exact same skeleton (blender_anims.py). Output: SourceArt/Generated/Anims/Source
import os
import unreal

OUT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "Generated", "Anims", "Source"))
os.makedirs(OUT, exist_ok=True)
log = []


def export(asset_path, filename, exporter):
    task = unreal.AssetExportTask()
    task.object = unreal.load_asset(asset_path)
    task.filename = os.path.join(OUT, filename)
    task.automated = True
    task.replace_identical = True
    task.prompt = False
    task.exporter = exporter
    opts = unreal.FbxExportOption()
    opts.set_editor_property("fbx_export_compatibility", unreal.FbxExportCompatibility.FBX_2020)
    opts.set_editor_property("ascii", False)
    opts.set_editor_property("export_morph_targets", False)
    opts.set_editor_property("export_preview_mesh", True)
    task.options = opts
    ok = unreal.Exporter.run_asset_export_task(task)
    log.append("%s -> %s (%s)" % (asset_path, filename, ok))


export("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple", "SKM_Quinn.fbx", unreal.SkeletalMeshExporterFBX())
export("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle", "MM_Idle.fbx", unreal.AnimSequenceExporterFBX())
export("/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Fwd", "MF_Walk_Fwd.fbx", unreal.AnimSequenceExporterFBX())
with open(os.path.join(unreal.Paths.project_saved_dir(), "ExportMannequin.txt"), "w") as f:
    f.write("\n".join(log))
