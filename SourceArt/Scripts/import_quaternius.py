# Project Eldoria - imports the CC0 Quaternius packs (glTF) into /Game/Project/Art/<Pack>/<Model>/
# Run inside the editor:  py "<path>/import_quaternius.py"
# Skips models already imported, works in small batches and frees memory between them.
# Writes a report to Saved/ImportReport.txt

import os
import unreal

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "Downloads"))
PACKS = {
    "Nature":   os.path.join(ROOT, "StylizedNature", "extracted", "glTF"),
    "Props":    os.path.join(ROOT, "FantasyProps", "extracted", "Exports", "glTF"),
    "Village":  os.path.join(ROOT, "MedievalVillage", "extracted", "glTF"),
}
BATCH = 15

def pending_files():
    content = unreal.Paths.project_content_dir()
    files = []
    for pack, folder in PACKS.items():
        if not os.path.isdir(folder):
            continue
        for name in sorted(os.listdir(folder)):
            if not name.lower().endswith(".gltf"):
                continue
            model = os.path.splitext(name)[0]
            meshes = os.path.join(content, "Project", "Art", pack, model, "StaticMeshes")
            if os.path.isdir(meshes) and os.listdir(meshes):
                continue
            files.append((pack, os.path.join(folder, name)))
    return files

def make_task(pack, path):
    task = unreal.AssetImportTask()
    task.filename = path
    task.destination_path = "/Game/Project/Art/" + pack
    task.automated = True
    task.replace_existing = True
    task.save = True
    return task

def main():
    files = pending_files()
    unreal.log("Eldoria import: %d files pending" % len(files))
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    failed = []
    done = 0
    for start in range(0, len(files), BATCH):
        tasks = [make_task(p, f) for p, f in files[start:start + BATCH]]
        tools.import_asset_tasks(tasks)
        for t in tasks:
            if t.imported_object_paths:
                done += 1
            else:
                failed.append(os.path.basename(t.filename))
        unreal.SystemLibrary.collect_garbage()
        unreal.log("Eldoria import: batch done (%d/%d)" % (min(start + BATCH, len(files)), len(files)))
    report = "pending=%d imported=%d failed=%d\n%s" % (len(files), done, len(failed), "\n".join(failed))
    with open(os.path.join(unreal.Paths.project_saved_dir(), "ImportReport.txt"), "w") as f:
        f.write(report)
    unreal.log("Eldoria import done: " + report.splitlines()[0])

main()
