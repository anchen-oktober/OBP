import unreal


CHARACTER_BP_PATH = "/Game/OneBullet/Blueprints/BP_OBCharacter"
DODGE_DISTANCE = 950.0
DODGE_DURATION = 0.10


def set_prop(obj, name, value):
    obj.set_editor_property(name, value)


bp_character = unreal.EditorAssetLibrary.load_asset(CHARACTER_BP_PATH)
if not bp_character:
    raise RuntimeError(f"Could not load {CHARACTER_BP_PATH}")

generated_class = unreal.load_class(None, f"{CHARACTER_BP_PATH}.BP_OBCharacter_C")
if not generated_class:
    raise RuntimeError(f"Could not load generated class for {CHARACTER_BP_PATH}")

char_cdo = unreal.get_default_object(generated_class)
set_prop(char_cdo, "dodge_distance", DODGE_DISTANCE)
set_prop(char_cdo, "dodge_duration", DODGE_DURATION)

unreal.EditorAssetLibrary.save_loaded_asset(bp_character)
unreal.EditorAssetLibrary.save_asset(CHARACTER_BP_PATH)
unreal.log(f"Updated dodge tuning: distance={DODGE_DISTANCE}, duration={DODGE_DURATION}")
