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

    background_music_tracks = []
    for asset_name, filename in [
        ("painkiller_01", "painkiller_01.mp3"),
        ("painkiller_02", "painkiller_02.mp3"),
    ]:
        asset = import_sound(asset_name, filename)
        if asset:
            background_music_tracks.append(asset)

    set_property(defaults, "BackgroundMusicTracks", background_music_tracks[:5])
    if background_music_tracks:
        set_property(defaults, "MusicSound", background_music_tracks[0])

    default_values = {
        "bAmbientHorrorEnabled": True,
        "MinAmbientInterval": 20.0,
        "MaxAmbientInterval": 60.0,
        "AmbientPlayChance": 0.62,
        "bMusicEnabled": True,
        "bRandomizeMusicOnStart": True,
        "bLoopMusic": True,
        "bEnableMusicDucking": True,
        "bAudioBalanceTestMode": False,
        "bEnableAudioDebugLogs": False,
        "bRoar1Enabled": True,
        "bRoar2Enabled": True,
        "bRoar3Enabled": True,
        "ShotVolume": 1.0,
        "BulletPickupVolume": 1.0,
        "HeartbeatVolume": 1.0,
        "LowDroneVolume": 0.65,
        "MusicVolume": 0.25,
        "NormalMusicVolume": 0.25,
        "PanicMusicVolume": 0.12,
        "MusicDuckFadeTime": 0.35,
        "MusicRestoreFadeTime": 0.75,
        "Roar1Volume": 1.0,
        "Roar2Volume": 0.9,
        "Roar3Volume": 0.9,
        "FootstepRun1Volume": 0.85,
        "FootstepRun2Volume": 0.85,
        "WhisperVolume": 0.5,
        "GhostVolume": 0.5,
        "HeartbeatFadeIn": 0.3,
        "HeartbeatFadeOut": 0.5,
        "LowDroneFadeIn": 0.25,
        "LowDroneFadeOut": 1.0,
        "MusicFadeIn": 3.0,
        "MusicFadeOut": 3.0,
        "RoarFadeIn": 0.8,
        "RoarFadeOut": 1.2,
        "FootstepsFadeIn": 0.5,
        "FootstepsFadeOut": 1.0,
        "AmbientFadeIn": 2.0,
        "AmbientFadeOut": 2.0,
        "PickupReliefFadeOut": 1.0,
    }
    for property_name, value in default_values.items():
        set_property(defaults, property_name, value)

    unreal.EditorAssetLibrary.save_loaded_asset(bp)
    unreal.log("[PanicAudioSetup] Configured BP_PanicAudioManager asset holder fields")


main()
