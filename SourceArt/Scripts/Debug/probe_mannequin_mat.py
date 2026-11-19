import os
import unreal
out = []
MEL = unreal.MaterialEditingLibrary
for p in ["/Game/Characters/Mannequins/Materials/Quinn/MI_Quinn_01", "/Game/Characters/Mannequins/Materials/Quinn/MI_Quinn_02"]:
    mi = unreal.load_asset(p)
    out.append("%s parent=%s" % (p, mi.get_editor_property("parent").get_name()))
    out.append("  scalars %s" % [str(n) for n in MEL.get_scalar_parameter_names(mi)])
    out.append("  vectors %s" % [str(n) for n in MEL.get_vector_parameter_names(mi)])
    out.append("  textures %s" % [str(n) for n in MEL.get_texture_parameter_names(mi)])
m = unreal.load_asset("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple")
out.append("slots %s" % [(str(s.get_editor_property("material_slot_name")), s.get_editor_property("material_interface").get_name()) for s in m.get_editor_property("materials")])
with open(os.path.join(unreal.Paths.project_saved_dir(), "MannequinMat.txt"), "w") as f:
    f.write("\n".join(out))
