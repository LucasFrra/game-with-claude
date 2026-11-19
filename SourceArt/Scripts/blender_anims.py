# Project Eldoria - J4 animations authored on the UE mannequin skeleton, headless:
#   blender -b --factory-startup --python blender_anims.py
# Bones are posed by *directions* in armature space (rotate the bone so that its child joint points that way),
# so the result does not depend on the internal bone axes. Character faces -Y, left is +X, up is +Z.
#   A_Climb      climbing cycle (arms and legs alternate)      A_ClimbHang  holding still on a wall
#   A_PushWalk   walk legs (from MF_Unarmed_Walk_Fwd) + leaning torso, arms pushing forward
#   A_CarryWalk  walk legs + object held above the head         A_CarryIdle  idle legs + object above the head
import math
import os

import bpy
from mathutils import Matrix, Vector

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
SRC = os.path.join(ROOT, "Generated", "Anims", "Source")
OUT = os.path.join(ROOT, "Generated", "Anims")
F, B, UP, DOWN, L, R = Vector((0, -1, 0)), Vector((0, 1, 0)), Vector((0, 0, 1)), Vector((0, 0, -1)), Vector((1, 0, 0)), Vector((-1, 0, 0))


def d(*parts):
    v = Vector((0, 0, 0))
    for w, axis in parts:
        v += axis * w
    return v.normalized()


def load(fbx):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=os.path.join(SRC, fbx), automatic_bone_orientation=False,
                             primary_bone_axis="X", secondary_bone_axis="Y")
    arm = next(o for o in bpy.context.scene.objects if o.type == "ARMATURE")
    for o in list(bpy.context.scene.objects):
        if o.type == "MESH":
            bpy.data.objects.remove(o, do_unlink=True)
    for pb in arm.pose.bones:
        pb.rotation_mode = "QUATERNION"
    return arm


def ordered(arm):
    out = []
    def walk(b):
        out.append(b.name)
        for c in b.children:
            walk(c)
    for b in arm.data.bones:
        if b.parent is None:
            walk(b)
    return out


def snapshot(arm):
    return {pb.name: pb.matrix.copy() for pb in arm.pose.bones}


def restore(arm, mats, order):
    for name in order:
        arm.pose.bones[name].matrix = mats[name]
        bpy.context.view_layer.update()


def aim(arm, bone, child, direction):
    pb, ch = arm.pose.bones[bone], arm.pose.bones[child]
    cur = (ch.head - pb.head).normalized()
    q = cur.rotation_difference(direction)
    t = Matrix.Translation(pb.head)
    pb.matrix = t @ q.to_matrix().to_4x4() @ t.inverted() @ pb.matrix
    bpy.context.view_layer.update()


def turn(arm, bone, axis, degrees):
    pb = arm.pose.bones[bone]
    t = Matrix.Translation(pb.head)
    pb.matrix = t @ Matrix.Rotation(math.radians(degrees), 4, axis) @ t.inverted() @ pb.matrix
    bpy.context.view_layer.update()


def pose_arm(arm, side, upper, fore):
    aim(arm, "upperarm_" + side, "lowerarm_" + side, upper)
    aim(arm, "lowerarm_" + side, "hand_" + side, fore)


def pose_leg(arm, side, thigh, calf, foot=None):
    aim(arm, "thigh_" + side, "calf_" + side, thigh)
    aim(arm, "calf_" + side, "foot_" + side, calf)
    if foot is not None:
        aim(arm, "foot_" + side, "ball_" + side, foot)


def key_all(arm, frame, pelvis_offset=None):
    if pelvis_offset is not None:
        arm.pose.bones["pelvis"].location += pelvis_offset
    for pb in arm.pose.bones:
        pb.keyframe_insert("rotation_quaternion", frame=frame)
        pb.keyframe_insert("location", frame=frame)


def export(arm, action, name, frame_end):
    arm.animation_data.action = action
    bpy.context.scene.frame_start = 1
    bpy.context.scene.frame_end = frame_end
    bpy.ops.object.select_all(action="DESELECT")
    arm.select_set(True)
    bpy.context.view_layer.objects.active = arm
    bpy.ops.export_scene.fbx(filepath=os.path.join(OUT, name + ".fbx"), use_selection=True, object_types={"ARMATURE"},
                             add_leaf_bones=False, primary_bone_axis="X", secondary_bone_axis="Y",
                             bake_anim=True, bake_anim_use_all_actions=False, bake_anim_use_nla_strips=False,
                             bake_anim_force_startend_keying=True, bake_anim_simplify_factor=0.0)
    print("exported", name, frame_end)


# ---------------------------------------------------------------- climbing (authored from the idle pose)
def climb_pose(arm, phase):
    """phase 0: left hand high / right foot high; phase 1: mirrored."""
    hi, lo = ("l", "r") if phase == 0 else ("r", "l")
    sx = {"l": 1.0, "r": -1.0}
    turn(arm, "spine_01", L, -8)                     # chest towards the wall
    pose_arm(arm, hi, d((0.25 * sx[hi], L), (0.35, F), (0.9, UP)), d((0.05 * sx[hi], L), (0.3, F), (0.95, UP)))
    pose_arm(arm, lo, d((0.5 * sx[lo], L), (0.55, F), (0.25, UP)), d((0.05 * sx[lo], L), (0.75, F), (0.65, UP)))
    pose_leg(arm, lo, d((0.18 * sx[lo], L), (0.55, F), (0.8, DOWN)), d((0.35, B), (0.94, DOWN)), d((0.9, F), (0.4, DOWN)))
    pose_leg(arm, hi, d((0.25 * sx[hi], L), (0.85, F), (0.25, DOWN)), d((0.2, B), (0.98, DOWN)), d((0.9, F), (0.4, DOWN)))


def climb_hang(arm):
    turn(arm, "spine_01", L, -8)
    for s, sx in (("l", 1.0), ("r", -1.0)):
        pose_arm(arm, s, d((0.35 * sx, L), (0.45, F), (0.75, UP)), d((0.05 * sx, L), (0.45, F), (0.85, UP)))
        pose_leg(arm, s, d((0.2 * sx, L), (0.6, F), (0.75, DOWN)), d((0.35, B), (0.94, DOWN)), d((0.9, F), (0.4, DOWN)))


arm = load("MM_Idle.fbx")
order = ordered(arm)
idle_action = arm.animation_data.action
bpy.context.scene.frame_set(1)
base = snapshot(arm)

for name, frames in (("A_Climb", [(1, 0), (16, 1), (31, 0)]), ("A_ClimbHang", [(1, None), (31, None)])):
    act = bpy.data.actions.new(name)
    arm.animation_data.action = act
    for frame, phase in frames:
        restore(arm, base, order)
        if phase is None:
            climb_hang(arm)
            off = Vector((0, 0, 0))
        else:
            climb_pose(arm, phase)
            off = Vector((0, 0, 3.0 if phase == 0 else -3.0))
        key_all(arm, frame, off)
    export(arm, act, name, frames[-1][0])


# ---------------------------------------------------------------- upper body overrides on top of walk / idle
def push_upper(arm):
    turn(arm, "spine_01", L, -14)
    turn(arm, "spine_03", L, -10)
    for s, sx in (("l", 1.0), ("r", -1.0)):
        pose_arm(arm, s, d((0.18 * sx, L), (0.95, F), (0.12, UP)), d((0.06 * sx, L), (0.98, F), (0.18, UP)))


def carry_upper(arm):
    turn(arm, "spine_03", L, 4)
    for s, sx in (("l", 1.0), ("r", -1.0)):
        pose_arm(arm, s, d((0.35 * sx, L), (0.15, F), (0.92, UP)), d((-0.3 * sx, L), (0.08, F), (0.95, UP)))


def layered(source_fbx, upper, name, every=1):
    arm = load(source_fbx)
    order = ordered(arm)
    src = arm.animation_data.action
    start, end = [int(v) for v in src.frame_range]
    frames = list(range(start, end + 1, every))
    poses = []
    for f in frames:
        arm.animation_data.action = src
        bpy.context.scene.frame_set(f)
        poses.append(snapshot(arm))
    act = bpy.data.actions.new(name)
    arm.animation_data.action = act
    for i, f in enumerate(frames):
        restore(arm, poses[i], order)
        upper(arm)
        key_all(arm, f - start + 1)
    export(arm, act, name, frames[-1] - start + 1)


layered("MF_Walk_Fwd.fbx", push_upper, "A_PushWalk", every=2)
layered("MF_Walk_Fwd.fbx", carry_upper, "A_CarryWalk", every=2)
layered("MM_Idle.fbx", carry_upper, "A_CarryIdle", every=5)
