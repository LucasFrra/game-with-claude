# Lists which /Game/Project/Art assets are actually referenced (recursively) by the project's levels,
# blueprints, materials and foliage. Writes Saved/UsedArt.txt (used package names) and a size summary.
import os
import unreal

reg = unreal.AssetRegistryHelpers.get_asset_registry()
opts = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True,
                                             include_searchable_names=False, include_soft_management_references=False,
                                             include_hard_management_references=False)
roots = []
for path in ["/Game/Project", "/Game/Characters", "/Game/Input", "/Game/ThirdPerson"]:
    for a in reg.get_assets_by_path(path, recursive=True):
        pkg = str(a.package_name)
        if not pkg.startswith("/Game/Project/Art"):
            roots.append(pkg)

used = set()
stack = list(roots)
seen = set()
while stack:
    pkg = stack.pop()
    if pkg in seen:
        continue
    seen.add(pkg)
    deps = reg.get_dependencies(pkg, opts) or []
    for d in deps:
        d = str(d)
        if d.startswith("/Game/"):
            if d.startswith("/Game/Project/Art"):
                used.add(d)
            stack.append(d)

content = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir())
total_used = 0
for pkg in used:
    f = os.path.join(content, pkg[len("/Game/"):] + ".uasset")
    if os.path.exists(f):
        total_used += os.path.getsize(f)
with open(os.path.join(unreal.Paths.project_saved_dir(), "UsedArt.txt"), "w") as fh:
    fh.write("# roots %d, used art packages %d, used art size %.1f MB\n" % (len(roots), len(used), total_used / 1e6))
    fh.write("\n".join(sorted(used)))
