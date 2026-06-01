import unreal


ENEMY_ASSETS = [
    "/Game/OneBullet/Blueprints/BP_OBEnemy_Fast",
    "/Game/OneBullet/Blueprints/BP_OBEnemy_Heavy",
]

ANIMATION_ASSETS = [
    "/Game/Assets/Animations/Idle",
    "/Game/Assets/Animations/Walking",
    "/Game/Assets/Animations/Fast_Run",
    "/Game/Assets/Animations/Standard_Run",
    "/Game/Assets/Animations/Standing_Run_Forward",
    "/Game/Assets/Animations/Orc_Idle",
]


def get_default_object(asset_path):
    asset = unreal.EditorAssetLibrary.load_asset(asset_path)
    generated_class = asset.generated_class()
    return unreal.get_default_object(generated_class)


for enemy_asset in ENEMY_ASSETS:
    cdo = get_default_object(enemy_asset)
    mesh_component = cdo.get_editor_property("mesh")
    skeletal_mesh = mesh_component.get_editor_property("skeletal_mesh_asset")
    mesh_skeleton = skeletal_mesh.get_editor_property("skeleton") if skeletal_mesh else None
    unreal.log(f"Enemy: {enemy_asset}")
    unreal.log(f"  Mesh: {skeletal_mesh.get_path_name() if skeletal_mesh else 'None'}")
    unreal.log(f"  Mesh skeleton: {mesh_skeleton.get_path_name() if mesh_skeleton else 'None'}")
    for anim_path in ANIMATION_ASSETS:
        anim = unreal.EditorAssetLibrary.load_asset(anim_path)
        anim_skeleton = anim.get_editor_property("skeleton") if anim else None
        compatible = mesh_skeleton == anim_skeleton
        unreal.log(f"  Anim {anim_path}: skeleton={anim_skeleton.get_path_name() if anim_skeleton else 'None'} compatible={compatible}")
