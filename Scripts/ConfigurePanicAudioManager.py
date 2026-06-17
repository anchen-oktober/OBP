import os
import unreal


PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SOUND_DIR = os.path.join(PROJECT_DIR, "Content", "Sound")
BP_PATH = "/Game/OneBullet/Blueprints/BP_PanicAudioManager"
DEST_PATH = "/Game/Sound"


def import_sound(asset_name, filename):
    asset_path = f"{DEST_PATH}/{asset_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        return unreal.EditorAssetLibrary.load_asset(asset_path)

    source_file = os.path.join(SOUND_DIR, filename)
    if not os.path.exists(source_file):
        unreal.log_warning(f"[PanicAudioSetup] Source file missing: {source_file}")
        return None

    task = unreal.AssetImportTask()
    task.filename = source_file
    task.destination_path = DEST_PATH
    task.destination_name = asset_name
    task.automated = True
    task.replace_existing = False
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    return unreal.EditorAssetLibrary.load_asset(asset_path)


def set_property(target, name, value):
    try:
        target.set_editor_property(name, value)
    except Exception as exc:
        unreal.log_warning(f"[PanicAudioSetup] Could not set {name}: {exc}")


def main():
    bp = unreal.EditorAssetLibrary.load_asset(BP_PATH)
    if not bp:
        unreal.log_warning(f"[PanicAudioSetup] Blueprint not found: {BP_PATH}")
        return

    defaults = unreal.get_default_object(bp.generated_class())
    asset_map = {
        "HeartbeatSound": ("heartbeat_loop", "heartbeat_loop.wav"),
        "CrowdRoar1": ("Roar1", "Roar1.wav"),
        "CrowdRoar2": ("Roar2", "Roar2.wav"),
        "CrowdRoar3": ("Roar3", "Roar3.mp3"),
        "CrowdFootstepRun1": ("run1", "run1.mp3"),
        "CrowdFootstepRun2": ("run2", "run2.mp3"),
        "BulletPickupSound": ("bullet_pickup", "bullet_pickup.mp3"),
        "WhisperAmbientSound": ("whisper", "whisper.wav"),
        "GhostAmbientSound": ("ghost", "ghost.wav"),
    }

    for property_name, (asset_name, filename) in asset_map.items():
        asset = import_sound(asset_name, filename)
        if asset:
            set_property(defaults, property_name, asset)

    shot_sound = unreal.EditorAssetLibrary.load_asset("/Game/Weapons/GrenadeLauncher/Audio/FirstPersonTemplateWeaponFire02")
    if shot_sound:
        set_property(defaults, "ShotSound", shot_sound)

    unreal.EditorAssetLibrary.save_loaded_asset(bp)
    unreal.log("[PanicAudioSetup] Configured BP_PanicAudioManager asset holder fields")


main()
