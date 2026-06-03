import unreal


ASSETS = [
    "/Game/OneBullet/Blueprints/BP_OBEnemy_Fast",
    "/Game/OneBullet/Blueprints/BP_OBEnemy_Heavy",
]

IDLE_ANIMATION = unreal.load_asset("/Game/Assets/Animations/Idle")
WALK_ANIMATION = unreal.load_asset("/Game/Assets/Animations/Standing_Run_Forward")
RUN_ANIMATION = unreal.load_asset("/Game/Assets/Animations/Fast_Run")

DEFAULTS = {
    "PlayerHasBulletSpeedMultiplier": 0.35,
    "PlayerHasBulletAttackSpeedMultiplier": 0.30,
    "PlayerHasBulletAttackRadius": 700.0,
    "PatrolRadius": 2600.0,
    "PatrolCenter": unreal.Vector(0.0, 0.0, 0.0),
    "PatrolPointJitter": 420.0,
    "PatrolMinTargetDistance": 900.0,
    "PatrolAcceptanceRadius": 120.0,
    "PatrolPerimeterRadiusMultiplier": 0.78,
    "PatrolPerimeterStepDegrees": 45.0,
    "PatrolObstacleProbeDistance": 260.0,
    "PatrolObstacleProbeRadius": 70.0,
    "FastSurroundRadius": 320.0,
    "HeavySurroundRadius": 420.0,
    "MinAggressiveSurroundRadius": 260.0,
    "bUseDirectLostBulletChase": True,
    "bUseSurroundMovement": True,
    "bUseSimpleLocomotionAnimations": True,
    "IdleAnimation": IDLE_ANIMATION,
    "WalkAnimation": WALK_ANIMATION or RUN_ANIMATION,
    "RunAnimation": RUN_ANIMATION,
    "WalkAnimationMinSpeed": 10.0,
    "RunAnimationMinSpeed": 340.0,
    "WalkAnimationPlayRate": 0.55,
    "RunAnimationPlayRate": 1.0,
}


def set_property(cdo, name, value):
    try:
        cdo.set_editor_property(name, value)
        return True
    except Exception:
        snake_name = []
        for index, char in enumerate(name):
            if index > 0 and char.isupper() and (name[index - 1].islower() or name[index - 1].isdigit()):
                snake_name.append("_")
            snake_name.append(char.lower())
        cdo.set_editor_property("".join(snake_name), value)
        return True


for asset_path in ASSETS:
    asset = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not asset:
        unreal.log_error(f"Enemy blueprint not found: {asset_path}")
        continue

    generated_class = asset.generated_class()
    cdo = unreal.get_default_object(generated_class)
    for prop_name, prop_value in DEFAULTS.items():
        set_property(cdo, prop_name, prop_value)
    cdo.set_editor_property("auto_possess_ai", unreal.AutoPossessAI.PLACED_IN_WORLD_OR_SPAWNED)
    cdo.set_editor_property("ai_controller_class", unreal.AIController)

    unreal.EditorAssetLibrary.save_asset(asset_path, only_if_is_dirty=False)
    unreal.log(f"Updated enemy pressure defaults: {asset_path}")
