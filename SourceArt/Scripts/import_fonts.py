# Imports the OFL fonts of SourceArt/Fonts as Font Faces in /Game/Project/UI/Fonts.
import os
import unreal
SRC = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "Fonts"))
DST = "/Game/Project/UI/Fonts"
tasks = []
for f in sorted(os.listdir(SRC)):
    if f.lower().endswith(".ttf"):
        t = unreal.AssetImportTask()
        t.filename = os.path.join(SRC, f)
        t.destination_path = DST
        t.automated = True
        t.replace_existing = True
        t.save = True
        tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
out = []
for p in unreal.EditorAssetLibrary.list_assets(DST, recursive=True):
    a = unreal.load_asset(p)
    out.append("%s %s" % (p, a.get_class().get_name()))
with open(os.path.join(unreal.Paths.project_saved_dir(), "Fonts.txt"), "w") as fh:
    fh.write("\n".join(out))
