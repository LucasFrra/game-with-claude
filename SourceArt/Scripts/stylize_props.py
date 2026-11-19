# Project Eldoria - J1b: stylized young (toppling) trees and rocks matching docs/ART_DIRECTION.md.
# Run after dress_jungle_2.py (it is also called from there).
import os
import sys
import importlib

sys.path.append(os.path.dirname(__file__))
import unreal
import eldoria_lib as L
importlib.reload(L)

MEL = unreal.MaterialEditingLibrary
SMES = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
log = []


def young_tree_mesh():
    """Physics copy of the stylized small tree, with a simple convex collision for toppling."""
    path = "/Game/Project/Environment/Trees/SM_YoungTree_Stylized"
    if not L.EAL.does_asset_exist(path):
        src = L.find_asset("/Game/Project/Environment/Stylized", "SM_Tree_Broadleaf_B", unreal.StaticMesh)
        L.EAL.duplicate_asset(src.get_path_name().split(".")[0], path)
    mesh = unreal.load_asset(path)
    SMES.remove_collisions(mesh)
    SMES.add_simple_collisions(mesh, unreal.ScriptCollisionShapeType.NDOP26)
    L.EAL.save_loaded_asset(mesh)
    return mesh


def young_trees(mesh):
    n = 0
    for a in L.find_actors(["Collapsible_YoungTree_"]):
        for c in a.get_components_by_class(unreal.StaticMeshComponent):
            if c.get_name().endswith("_Canopy"):
                c.set_static_mesh(mesh)
                c.set_editor_property("override_materials", [])
                loc = c.get_editor_property("relative_location")
                c.set_editor_property("relative_location", unreal.Vector(loc.x, loc.y, -30.0))
                c.set_editor_property("relative_scale3d", unreal.Vector(0.85, 0.85, 0.85))
        n += 1
    return n


def rocks():
    """Warm light stone with moss on top (creature-skin master: noise + up-facing moss)."""
    parent = unreal.load_asset("/Game/Project/Materials/Master/M_Eldoria_CreatureSkin")
    mi = L.make_instance("MI_Rock_Stylized", parent, "/Game/Project/Materials/Stylized", None, None,
                         {"SkinA": (0.30, 0.28, 0.25), "SkinB": (0.46, 0.43, 0.38), "Moss": (0.20, 0.34, 0.07)})
    done = set()
    for a in L.find_actors(["Rock_Dress_", "Pool_RimStone_", "ForceTarget_"]):
        mesh = a.get_component_by_class(unreal.StaticMeshComponent).static_mesh if not isinstance(a, unreal.EldoriaPhysicsObject) else a.get_editor_property("mesh").static_mesh
        if mesh and mesh.get_name() not in done:
            for i, _ in enumerate(mesh.get_editor_property("static_materials")):
                mesh.set_material(i, mi)
            L.EAL.save_loaded_asset(mesh)
            done.add(mesh.get_name())
    return sorted(done)


try:
    log.append("young trees %d" % young_trees(young_tree_mesh()))
    log.append("rock meshes %s" % rocks())
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
except Exception as exc:
    log.append("[FATAL] %r" % exc)
with open(os.path.join(unreal.Paths.project_saved_dir(), "StylizeProps.txt"), "w") as f:
    f.write("\n".join(log))
