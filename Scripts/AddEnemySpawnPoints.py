import unreal


LEVEL_PATH = "/Game/LevelPrototyping/Lvl_FirstPerson"
SPAWN_POINT_CLASS_PATH = "/Script/OneBulletLeft.OBEnemySpawnPoint"

SPAWN_POINTS = [
    ("Actor1", unreal.Vector(1200.0, 1200.0, 120.0), unreal.Rotator(0.0, 0.0, 0.0)),
    ("Actor2", unreal.Vector(-1200.0, 1200.0, 120.0), unreal.Rotator(0.0, 0.0, 0.0)),
    ("Actor3", unreal.Vector(1200.0, -1200.0, 120.0), unreal.Rotator(0.0, 0.0, 0.0)),
    ("Actor4", unreal.Vector(-1200.0, -1200.0, 120.0), unreal.Rotator(0.0, 0.0, 0.0)),
    ("Actor5", unreal.Vector(0.0, 1450.0, 120.0), unreal.Rotator(0.0, 0.0, 0.0)),
    ("Actor6", unreal.Vector(0.0, -1450.0, 120.0), unreal.Rotator(0.0, 0.0, 0.0)),
]


def main():
    unreal.EditorLevelLibrary.load_level(LEVEL_PATH)

    spawn_point_class = unreal.load_class(None, SPAWN_POINT_CLASS_PATH)
    if not spawn_point_class:
        raise RuntimeError(f"Unable to load class: {SPAWN_POINT_CLASS_PATH}")

    def is_enemy_spawn_point(actor):
        actor_class = actor.get_class()
        class_path = actor_class.get_path_name()
        class_name = actor_class.get_name()
        return class_path == SPAWN_POINT_CLASS_PATH or "EnemySpawnPoint" in class_name

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    existing_spawn_points = [
        actor
        for actor in actor_subsystem.get_all_level_actors()
        if actor and is_enemy_spawn_point(actor)
    ]

    if existing_spawn_points:
        unreal.log(f"Enemy spawn points already exist: {len(existing_spawn_points)}. No actors were added.")
        return

    for label, location, rotation in SPAWN_POINTS:
        actor = actor_subsystem.spawn_actor_from_class(spawn_point_class, location, rotation)
        if not actor:
            raise RuntimeError(f"Failed to spawn enemy spawn point: {label}")
        actor.set_actor_label(label, mark_dirty=True)
        actor.set_actor_location(location, False, False)
        actor.set_actor_rotation(rotation, False)
        unreal.log(f"Added enemy spawn point {label} at {location}")

    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log(f"Added Enemy Spawn Points: {len(SPAWN_POINTS)}")


if __name__ == "__main__":
    main()
