import unreal


ENEMY_BLUEPRINT_PATHS = (
    "/Game/OneBullet/Blueprints/BP_OBEnemy_Fast",
    "/Game/OneBullet/Blueprints/BP_OBEnemy_Heavy",
)

STATE_TUNING = {
    "rush_transition_delay_min": 0.0,
    "rush_transition_delay_max": 0.35,
    "cautious_chaser_chance": 0.20,
    "cautious_flanker_chance": 0.80,
    "cautious_speed_multiplier_min": 0.70,
    "cautious_speed_multiplier_max": 0.85,
    "cautious_speed_random_variance": 0.10,
    "cautious_flanker_distance": 600.0,
    "cautious_flanker_min_distance": 300.0,
    "cautious_compression_speed": 28.0,
    "rush_chaser_chance": 0.70,
    "rush_flanker_chance": 0.10,
    "rush_bullet_blocker_chance": 0.20,
    "rush_speed_multiplier_min": 1.10,
    "rush_speed_multiplier_max": 1.30,
    "rush_speed_random_variance": 0.10,
    "rush_flanker_distance_min": 300.0,
    "rush_flanker_distance_max": 600.0,
    "bullet_blocker_acceptance_radius": 110.0,
    "draw_ai_role_targets": False,
    "min_distance_between_enemy_targets": 300.0,
    "flank_radius_min": 500.0,
    "flank_radius_max": 900.0,
    "target_random_offset_min": 100.0,
    "target_random_offset_max": 250.0,
    "target_reservation_attempts": 6,
    "cautious_flank_start_radius": 900.0,
    "cautious_flank_min_radius": 450.0,
    "cautious_flank_approach_speed_min": 80.0,
    "cautious_flank_approach_speed_max": 150.0,
    "rush_flank_start_radius": 700.0,
    "rush_flank_min_radius": 250.0,
    "rush_flank_approach_speed_min": 200.0,
    "rush_flank_approach_speed_max": 350.0,
    "flank_side_lock_duration_min": 2.0,
    "flank_side_lock_duration_max": 4.0,
    "max_allowed_flank_distance_increase": 100.0,
    "flank_target_lateral_offset_max": 90.0,
    "flank_close_pressure_range": 75.0,
    "chaser_repath_interval_min": 0.20,
    "chaser_repath_interval_max": 0.50,
    "flanker_repath_interval_min": 0.80,
    "flanker_repath_interval_max": 1.50,
    "blocker_repath_interval_min": 0.40,
    "blocker_repath_interval_max": 0.80,
    "fast_attack_radius": 100.0,
    "heavy_attack_radius": 200.0,
    "draw_attack_radius": False,
}


for blueprint_path in ENEMY_BLUEPRINT_PATHS:
    blueprint = unreal.EditorAssetLibrary.load_asset(blueprint_path)
    if not blueprint:
        raise RuntimeError(f"Could not load {blueprint_path}")

    asset_name = blueprint_path.rsplit("/", 1)[-1]
    generated_class = unreal.load_class(None, f"{blueprint_path}.{asset_name}_C")
    if not generated_class:
        raise RuntimeError(f"Could not load generated class for {blueprint_path}")

    enemy_cdo = unreal.get_default_object(generated_class)
    for property_name, value in STATE_TUNING.items():
        enemy_cdo.set_editor_property(property_name, value)
    enemy_cdo.set_editor_property("can_touch_kill_from_behind", asset_name == "BP_OBEnemy_Heavy")

    unreal.EditorAssetLibrary.save_loaded_asset(blueprint)
    unreal.EditorAssetLibrary.save_asset(blueprint_path)
    unreal.log(
        f"Updated AI state tuning on {blueprint_path}: "
        f"cautious={enemy_cdo.get_editor_property('cautious_flanker_min_distance')}-"
        f"{enemy_cdo.get_editor_property('cautious_flanker_distance')}, "
        f"rush={enemy_cdo.get_editor_property('rush_flanker_distance_min')}-"
        f"{enemy_cdo.get_editor_property('rush_flanker_distance_max')}, "
        f"attack={enemy_cdo.get_editor_property('fast_attack_radius')}/"
        f"{enemy_cdo.get_editor_property('heavy_attack_radius')}, "
        f"spacing={enemy_cdo.get_editor_property('min_distance_between_enemy_targets')}, "
        f"flank={enemy_cdo.get_editor_property('cautious_flank_start_radius')}->"
        f"{enemy_cdo.get_editor_property('cautious_flank_min_radius')}/"
        f"{enemy_cdo.get_editor_property('rush_flank_start_radius')}->"
        f"{enemy_cdo.get_editor_property('rush_flank_min_radius')}, "
        f"all_around={enemy_cdo.get_editor_property('can_touch_kill_from_behind')}, "
        f"debug={enemy_cdo.get_editor_property('draw_ai_role_targets')}"
    )
