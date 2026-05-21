import unreal


BP_DIR = "/Game/OneBullet/Blueprints"
MAP_PATH = "/Game/FirstPerson/Lvl_FirstPerson"


def ensure_dir(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def load_class(path):
    cls = unreal.load_class(None, path)
    if not cls:
        raise RuntimeError(f"Could not load class: {path}")
    return cls


def backup_broken_asset(asset_path):
    asset_name = asset_path.rsplit("/", 1)[-1]
    backup_path = f"{BP_DIR}/_Broken_{asset_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(backup_path):
        backup_path = unreal.AssetToolsHelpers.get_asset_tools().create_unique_asset_name(backup_path, "")[0]

    if unreal.EditorAssetLibrary.rename_asset(asset_path, backup_path):
        unreal.log_warning(f"Moved broken asset {asset_path} to {backup_path}")
        return

    unreal.log_warning(f"Could not move broken asset {asset_path}; deleting redirector so it can be recreated")
    unreal.EditorAssetLibrary.delete_asset(asset_path)


def can_use_existing_blueprint(asset_path, expected_type):
    existing = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not existing:
        return None

    if isinstance(existing, expected_type):
        return existing

    class_name = existing.get_class().get_name()
    if class_name == "ObjectRedirector":
        backup_broken_asset(asset_path)
        return None

    generated_class = unreal.load_class(None, f"{asset_path}.{asset_path.rsplit('/', 1)[-1]}_C")
    if generated_class:
        return existing

    raise RuntimeError(f"Existing asset is not a usable Blueprint: {existing} ({class_name})")


def make_blueprint(name, parent_path):
    asset_path = f"{BP_DIR}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        existing = can_use_existing_blueprint(asset_path, unreal.Blueprint)
        if existing:
            return existing

    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", load_class(parent_path))
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    bp = asset_tools.create_asset(name, BP_DIR, unreal.Blueprint, factory)
    if not bp:
        raise RuntimeError(f"Could not create {asset_path}")
    return bp


def make_widget_blueprint(name, parent_path):
    asset_path = f"{BP_DIR}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        existing = can_use_existing_blueprint(asset_path, unreal.WidgetBlueprint)
        if existing:
            return existing

    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", load_class(parent_path))
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    bp = asset_tools.create_asset(name, BP_DIR, unreal.WidgetBlueprint, factory)
    if not bp:
        raise RuntimeError(f"Could not create {asset_path}")
    unreal.EditorAssetLibrary.save_loaded_asset(bp)
    unreal.EditorAssetLibrary.save_asset(asset_path)
    return bp


def bp_class(bp):
    if not isinstance(bp, unreal.Blueprint):
        generated_class = unreal.load_class(None, f"{bp.get_path_name()}_C")
        if generated_class:
            return generated_class
        raise RuntimeError(f"Asset is not a Blueprint and generated class was not found: {bp}")
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    return unreal.BlueprintEditorLibrary.generated_class(bp)


def cdo(bp):
    return unreal.get_default_object(bp_class(bp))


def set_prop(obj, prop, value):
    try:
        obj.set_editor_property(prop, value)
    except Exception as exc:
        unreal.log_warning(f"Could not set {prop} on {obj}: {exc}")


ensure_dir("/Game/OneBullet")
ensure_dir(BP_DIR)

bp_game_state = make_blueprint("BP_OBGameState", "/Script/OneBulletLeft.OBGameState")
bp_hud = make_blueprint("BP_OBHUD", "/Script/OneBulletLeft.OBHUD")
wbp_hud = make_widget_blueprint("WBP_OBHUD", "/Script/OneBulletLeft.OBHUDWidget")
bp_character = make_blueprint("BP_OBCharacter", "/Script/OneBulletLeft.OBCharacter")
bp_pickup = make_blueprint("BP_OBBulletPickup", "/Script/OneBulletLeft.OBBulletPickup")
bp_enemy_fast = make_blueprint("BP_OBEnemy_Fast", "/Script/OneBulletLeft.OBEnemy")
bp_enemy_heavy = make_blueprint("BP_OBEnemy_Heavy", "/Script/OneBulletLeft.OBEnemy")
bp_game_mode = make_blueprint("BP_OBGameMode", "/Script/OneBulletLeft.OBGameMode")

hud_cdo = cdo(bp_hud)
set_prop(hud_cdo, "hud_widget_class", bp_class(wbp_hud))

wbp_hud_cdo = cdo(wbp_hud)

fast_cdo = cdo(bp_enemy_fast)
set_prop(fast_cdo, "enemy_type", unreal.OBEnemyType.FAST)
set_prop(fast_cdo, "fast_speed", 820.0)
set_prop(fast_cdo, "heavy_speed", 260.0)

heavy_cdo = cdo(bp_enemy_heavy)
set_prop(heavy_cdo, "enemy_type", unreal.OBEnemyType.HEAVY)
set_prop(heavy_cdo, "fast_speed", 760.0)
set_prop(heavy_cdo, "heavy_speed", 230.0)

pickup_cdo = cdo(bp_pickup)
set_prop(pickup_cdo, "use_native_presentation_animation", True)
set_prop(pickup_cdo, "mesh_base_height", 12.0)
set_prop(pickup_cdo, "bob_height", 4.0)
set_prop(pickup_cdo, "bob_speed", 5.0)
set_prop(pickup_cdo, "spin_speed", 180.0)

char_cdo = cdo(bp_character)
set_prop(char_cdo, "hide_head_for_first_person", True)
set_prop(char_cdo, "kick_cooldown", 2.4)
set_prop(char_cdo, "kick_range", 320.0)
set_prop(char_cdo, "kick_radius", 125.0)
set_prop(char_cdo, "shoot_range", 5000.0)
set_prop(char_cdo, "dodge_distance", 420.0)
set_prop(char_cdo, "dodge_duration", 0.18)
set_prop(char_cdo, "dodge_cooldown", 1.15)
set_prop(char_cdo, "dodge_enemy_clearance", 180.0)
set_prop(char_cdo, "prefer_movement_direction_dodge", True)

gm_cdo = cdo(bp_game_mode)
set_prop(gm_cdo, "default_pawn_class", bp_class(bp_character))
set_prop(gm_cdo, "game_state_class", bp_class(bp_game_state))
set_prop(gm_cdo, "hud_class", bp_class(bp_hud))
set_prop(gm_cdo, "enemy_class", load_class("/Script/OneBulletLeft.OBEnemy"))
set_prop(gm_cdo, "fast_enemy_class", bp_class(bp_enemy_fast))
set_prop(gm_cdo, "heavy_enemy_class", bp_class(bp_enemy_heavy))
set_prop(gm_cdo, "bullet_pickup_class", bp_class(bp_pickup))
set_prop(gm_cdo, "force_windowed_mode", True)
set_prop(gm_cdo, "windowed_resolution_x", 1280)
set_prop(gm_cdo, "windowed_resolution_y", 800)
set_prop(gm_cdo, "spawn_interval", 3.0)
set_prop(gm_cdo, "max_live_enemies", 8)
set_prop(gm_cdo, "build_greybox_arena", True)
set_prop(gm_cdo, "spawn_enemies_only_in_front_of_player", True)
set_prop(gm_cdo, "front_spawn_min_dot", 0.35)
set_prop(gm_cdo, "allow_any_spawn_if_no_front_point", False)
set_prop(gm_cdo, "bullet_pickup_drop_height", 5.0)

for bp in [bp_game_state, bp_hud, wbp_hud, bp_character, bp_pickup, bp_enemy_fast, bp_enemy_heavy, bp_game_mode]:
    if isinstance(bp, unreal.Blueprint):
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    unreal.EditorAssetLibrary.save_loaded_asset(bp)

unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
world = unreal.EditorLevelLibrary.get_editor_world()
settings = world.get_world_settings()
set_prop(settings, "default_game_mode", bp_class(bp_game_mode))
unreal.EditorLoadingAndSavingUtils.save_current_level()

unreal.log("One Bullet Blueprint assets created and Lvl_FirstPerson GameMode override set to BP_OBGameMode.")
