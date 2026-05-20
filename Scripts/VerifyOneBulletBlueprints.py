import unreal

paths = [
    "/Game/OneBullet/Blueprints/BP_OBGameMode",
    "/Game/OneBullet/Blueprints/BP_OBCharacter",
    "/Game/OneBullet/Blueprints/BP_OBEnemy_Fast",
    "/Game/OneBullet/Blueprints/BP_OBEnemy_Heavy",
    "/Game/OneBullet/Blueprints/BP_OBBulletPickup",
    "/Game/OneBullet/Blueprints/BP_OBHUD",
    "/Game/OneBullet/Blueprints/BP_OBGameState",
]

lines = []
for path in paths:
    asset = unreal.EditorAssetLibrary.load_asset(path)
    lines.append(f"{path}: {'OK' if asset else 'MISSING'}")

unreal.EditorLoadingAndSavingUtils.load_map("/Game/FirstPerson/Lvl_FirstPerson")
world = unreal.EditorLevelLibrary.get_editor_world()
settings = world.get_world_settings()
lines.append(f"Level GameMode Override: {settings.get_editor_property('default_game_mode')}")

gm = unreal.EditorAssetLibrary.load_asset("/Game/OneBullet/Blueprints/BP_OBGameMode")
gm_class = unreal.BlueprintEditorLibrary.generated_class(gm)
gm_cdo = unreal.get_default_object(gm_class)
lines.append(f"Default Pawn: {gm_cdo.get_editor_property('default_pawn_class')}")
lines.append(f"Fast Enemy Class: {gm_cdo.get_editor_property('fast_enemy_class')}")
lines.append(f"Heavy Enemy Class: {gm_cdo.get_editor_property('heavy_enemy_class')}")

with open(r"D:\MyProjects\OneBulletLeft\Saved\OneBulletBPVerify.txt", "w", encoding="utf-8") as handle:
    handle.write("\n".join(lines))
