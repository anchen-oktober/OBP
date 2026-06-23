# One Bullet Left

`One Bullet Left` - прототип арена-шутера на Unreal Engine 5.7. Игрок выживает на арене с одной ключевой пулей: выстрел запускает давление толпы, а возврат пули даёт короткое облегчение и шанс перестроиться.

## Быстрый Старт

Открыть проект:

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe' 'D:\MyProjects\OneBulletLeft\OneBulletLeft.uproject'
```

Собрать editor target:

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' OneBulletLeftEditor Win64 Development -Project='D:\MyProjects\OneBulletLeft\OneBulletLeft.uproject' -WaitMutex -NoHotReload -NoXGE
```

Запустить standalone на основной карте:

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe' 'D:\MyProjects\OneBulletLeft\OneBulletLeft.uproject' /Game/LevelPrototyping/Lvl_FirstPerson -game -windowed -ResX=1280 -ResY=800 -log
```

## Основные Ассеты

- Основная карта: `/Game/LevelPrototyping/Lvl_FirstPerson`
- GameMode: `/Game/OneBullet/Blueprints/BP_OBGameMode`
- WaveManager: `/Game/OneBullet/Blueprints/BP_OBWaveManager`
- Fast enemy: `/Game/OneBullet/Blueprints/BP_OBEnemy_Fast`
- Heavy enemy: `/Game/OneBullet/Blueprints/BP_OBEnemy_Heavy`
- Enemy spawn point: `/Game/OneBullet/Blueprints/BP_EnemySpawnPoint`
- Panic audio manager: `/Game/OneBullet/Blueprints/BP_PanicAudioManager`

## Управление

- `WASD` - движение
- `Mouse` - обзор
- `Left Mouse Button` - выстрел
- `Right Mouse Button` / `F` - удар
- `Left Shift` - dodge
- `Space` - jump
- `R` - restart run

## Архитектура

Ключевые C++ классы находятся в `Source/OneBulletLeft`:

- `AOBCharacter` - игрок, стрельба, dodge, удар, состояние run.
- `AOBEnemy` - общий enemy character, типы Fast/Heavy, AI movement, атака, spawn protection.
- `AOBWaveManager` - волны, выбор spawn point, проверка NavMesh/collision, спавн Fast/Heavy.
- `AOBEnemySpawnPoint` - простой actor-маркер позиции. Для логики спавна используется только `GetActorLocation()`.
- `AOBGameMode` - создаёт/находит WaveManager и PanicAudioManager, управляет reset/run flow.
- `AOBPanicAudioManager` - heartbeat, drone, roars, footsteps, pickup relief и music ducking.
- `UOBHUDWidget` / `AOBHUD` - HUD и отображение состояния волны/игры.

Модули проекта:

- `Core`
- `CoreUObject`
- `Engine`
- `InputCore`
- `EnhancedInput`
- `AIModule`
- `NavigationSystem`
- `UMG`
- `Niagara`

## WaveManager И Спавн

`BP_OBWaveManager` хранит настройки волн в `Wave Definitions`. Для каждой волны задаются:

- `FastCount`
- `HeavyCount`
- `DelayBeforeWave`
- `SpawnInterval`
- `MaxLiveEnemies`

WaveManager обязан спавнить ровно заданные количества типов. Heavy не заменяются Normal врагами. Если `HeavyCount > 0`, используется `HeavyEnemyClass`; если он не назначен, WaveManager пишет явную ошибку в лог.

Runtime path:

1. `BP_OBGameMode` создаёт `BP_OBWaveManager`.
2. `WaveManager` строит очередь врагов волны.
3. В очередь добавляются Heavy и Fast типы согласно `HeavyCount` и `FastCount`.
4. Для каждого врага выбирается actor spawn point на карте.
5. Точка проецируется на NavMesh.
6. Проверяется collision capsule.
7. Вызывается `SpawnActorDeferred` для конкретного класса врага.
8. После spawn проверяются AIController, capsule, movement и NavMesh.

Важно: `BP_EnemySpawnPoint` должен оставаться простым маркером позиции. Не использовать rotation, forward vector, arrow или дополнительную логику внутри spawn point actor.

## Spawn Point Checklist

Если враги не появляются:

- На карте должны быть actor-точки `BP_EnemySpawnPoint`.
- `BP_OBWaveManager` должен находить точки через `GetAllActorsOfClass`.
- Spawn point actor должен стоять рядом с доступной областью NavMesh.
- `Nav Mesh Bounds Volume` должен покрывать арену и точки спавна.
- После изменения геометрии нужно пересобрать NavMesh.
- В логах смотреть `LogOBWaveManager`: выбранный класс, точка, результат SpawnActor, controller/movement/navmesh.

Для Heavy отдельно проверить:

- `HeavyEnemyClass = BP_OBEnemy_Heavy_C`
- `BP_OBEnemy_Heavy` наследуется от enemy character.
- Capsule и CharacterMovement существуют.
- `AIControllerClass` назначен.
- `Auto Possess AI = Placed in World or Spawned`
- actor не hidden, scale не нулевой.

## Arena Collision И NavMesh

Основная карта использует арену `/Game/LevelPrototyping/Lvl_FirstPerson`.

Для static mesh арены:

- `Collision Complexity`: `Use Complex Collision As Simple`
- placed actor должен блокировать игрока и врагов
- `Can Ever Affect Navigation`: `true`

Рекомендации:

- Не добавлять случайные collision boxes вместо нормальной collision геометрии арены.
- Проверять зелёную NavMesh-сетку на полу, проходах, платформах, рампах и возвышениях.
- Если spawn validation пишет `not on NavMesh`, сначала смотреть покрытие NavMesh и высоту/позицию spawn point actor.

## Panic Audio

Panic Audio запускается после выстрела и останавливается/смягчается при возврате пули.

Главный manager:

- Blueprint: `/Game/OneBullet/Blueprints/BP_PanicAudioManager`
- C++: `AOBPanicAudioManager`

Основные слои:

- `ShotSound`
- `HeartbeatSound`
- `LowDroneSound`
- `RoarSound1/2/3`
- `FootstepSound1/2`
- `BulletPickupSound`
- `AmbientHorrorSounds`
- `BackgroundMusicTracks`

Настройка: `BP_PanicAudioManager`, категории `OneBulletSettings|Panic Audio` и `Panic Audio | Music`.

## Проверка Перед Коммитом

Минимальный набор:

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' OneBulletLeftEditor Win64 Development -Project='D:\MyProjects\OneBulletLeft\OneBulletLeft.uproject' -WaitMutex -NoHotReload -NoXGE
```

Gameplay smoke test:

1. Открыть `/Game/LevelPrototyping/Lvl_FirstPerson`.
2. Запустить Play/Standalone.
3. Проверить, что WaveManager создаётся как `BP_OBWaveManager_C`.
4. Проверить, что Fast и Heavy появляются согласно `Wave Definitions`.
5. Проверить, что Heavy виден, движется, получает AIController и атакует игрока.
6. Проверить, что в игре нет debug spheres, debug labels и временных spawn markers.

## Логи

Основные логи смотреть в:

```text
Saved/Logs/OneBulletLeft.log
```

Полезные категории:

- `LogOBWaveManager`
- `LogOBEnemyAI`
- `LogTemp`

## Замечания По Репозиторию

- Не коммитить `Binaries`, `Intermediate`, `Saved`, `DerivedDataCache`, `.vs`.
- Большие архивы вроде `OneBulletLeft_Sources.zip` или локальных zip-копий лучше держать вне git.
- Для правок Blueprint assets обязательно проверять, что изменения действительно сохранены в Editor.
