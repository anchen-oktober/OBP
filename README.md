# One Bullet Left

`One Bullet Left` - прототип FPS arena shooter на Unreal Engine 5.7. Главная идея: у игрока одна ключевая пуля. Выстрел решает момент, но после него нужно выжить под давлением врагов, вернуть пулю и заново открыть себе окно контроля.

## Быстрый старт

Открыть проект:

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe' 'D:\MyProjects\OneBulletLeft\OneBulletLeft.uproject'
```

Собрать editor target:

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' OneBulletLeftEditor Win64 Development -Project='D:\MyProjects\OneBulletLeft\OneBulletLeft.uproject' -WaitMutex -NoHotReload -NoXGE
```

Если сборка пишет `Unable to build while Live Coding is active`, закройте Editor/Game или нажмите `Ctrl+Alt+F11`, затем повторите build.

## Основные ассеты

- Карта: `/Game/LevelPrototyping/Lvl_FirstPerson`
- Игрок: `/Game/OneBullet/Blueprints/BP_OBCharacter`
- GameMode: `/Game/OneBullet/Blueprints/BP_OBGameMode`
- WaveManager: `/Game/OneBullet/Blueprints/BP_OBWaveManager`
- Fast enemy: `/Game/OneBullet/Blueprints/BP_OBEnemy_Fast`
- Heavy enemy: `/Game/OneBullet/Blueprints/BP_OBEnemy_Heavy`
- Spawn point: `/Game/OneBullet/Blueprints/BP_EnemySpawnPoint`
- Panic audio: `/Game/OneBullet/Blueprints/BP_PanicAudioManager`

## Управление

- `WASD` - движение
- `Mouse` - обзор
- `Left Mouse Button` - выстрел
- `Right Mouse Button` / `F` - Kick
- `Left Shift` - dodge
- `Space` - jump
- `R` - restart run

## Kick

Kick настроен как быстрый FPS-удар: нажал, получил короткий anticipation, затем через задержку происходит impact, и только в этот момент враги получают knockback.

Ключевой runtime flow в `AOBCharacter`:

1. `Kick()` вызывается по input.
2. `Kick()` не делает trace и не применяет knockback.
3. `Kick()` запускает action animation, блокирует повторный Kick, включает whoosh/anticipation и ставит таймер `KickImpactDelay`.
4. `ApplyKickImpact()` вызывается таймером.
5. Только внутри `ApplyKickImpact()` выполняется sphere trace, собираются враги и вызывается `Enemy->ApplyKick(...)`.
6. Impact feedback - звук удара, camera shake, FOV punch, VFX/dust, короткий hit-stop - тоже запускается в `ApplyKickImpact()`.

Главные BP-параметры в `BP_OBCharacter`, категория `OneBulletSettings|Kick`:

- `KickPlayRate`: скорость проигрывания Kick-анимации. Текущая стартовая настройка: `2.0`. Для теста: `1.7`, `2.0`, `2.3`.
- `KickImpactDelay`: задержка до impact. Текущая стартовая настройка: `0.16`. Для теста: `0.12`, `0.16`, `0.20`.
- `KickRecoveryTime`: когда управление после Kick снова считается свободным.
- `KickKnockbackStrength`: дальность/сила отлета врага. Рабочий диапазон: `600-1000`.
- `KickTraceDistance`: дальность trace перед игроком. Рабочий диапазон: `140-190 uu`.
- `KickTraceRadius`: радиус sphere trace. Рабочий диапазон: `50-80 uu`.
- `KickPushDuration`: длительность самого отлета врага.
- `KickStunDuration`: stagger/stun после пинка.
- `KickSlowMultiplier` и `KickSlowDuration`: временное замедление врага после Kick.

Feedback-параметры Kick в `OneBulletSettings|Kick|Feel`:

- `KickCameraShake`: camera shake на impact.
- `KickWhooshSound`: короткий whoosh на старте Kick.
- `KickImpactSound`: тяжелый звук попадания в момент `ApplyKickImpact()`.
- `KickImpactVFX`: dust puff / impact puff перед игроком.
- `KickPushEffect`: старый fallback VFX, используется если `KickImpactVFX` пустой.
- `KickFOVPunchAmount`, `KickFOVPunchInDuration`, `KickFOVPunchOutDuration`: FOV punch на impact.
- `KickCameraTiltDownAmount`, `KickCameraTiltRecoverySpeed`: anticipation камеры на старте.
- `KickImpactCameraPitchPunch`, `KickImpactCameraYawPunch`: процедурный camera punch на impact, полезен если `KickCameraShake` не назначен.
- `KickImpactHitStopDuration`: очень короткий freeze на impact.
- `KickWeaponSwayBack`, `KickWeaponSwayDown`, `KickWeaponSwayPitch`, `KickWeaponSwayDuration`: короткий sway оружия/рук на старте.

Важное правило для Kick: если враг отлетает сразу при нажатии, это баг. Проверять нужно, что `SweepMultiByChannel` и `Enemy->ApplyKick(...)` находятся только в `ApplyKickImpact()`, а не в `Kick()`.

## Звуки

Звуки разделены на короткие gameplay one-shot и panic audio слои.

### Игрок и оружие

В `BP_OBCharacter`:

- `DryFireSound`: попытка стрелять без пули.
- `KickSound`: старый общий fallback для Kick.
- `KickWhooshSound`: стартовый свист/рывок Kick.
- `KickImpactSound`: тяжелый удар в момент impact.
- `DodgeSound`: dodge.
- `DeathSound`: смерть игрока.
- `HitConfirmSound`: подтверждение попадания.
- `BulletImpactSound`: попадание пули в геометрию.
- `ShootCameraShake`, `KickCameraShake`: camera shake для выстрела и Kick.

Рекомендации по миксу Kick:

- Whoosh должен быть коротким, сухим и раньше impact.
- Impact sound должен быть заметно тяжелее whoosh.
- Если impact не читается, сначала проверьте `KickImpactSound`, `KickCameraShake` и `KickFOVPunchAmount`.
- Не использовать магические вспышки или sci-fi эффект, если нужен обычный удар ногой.

### Panic Audio

Главный BP: `/Game/OneBullet/Blueprints/BP_PanicAudioManager`.

Основные слои:

- `ShotSound`: звук начала panic после выстрела.
- `BulletPickupSound`: облегчение/подбор пули.
- `HeartbeatSound`: сердцебиение.
- `LowDroneSound`: низкий напряженный слой.
- `CrowdRoar1`, `CrowdRoar2`, `CrowdRoar3`: рев/толпа.
- `CrowdFootstepRun1`, `CrowdFootstepRun2`: бегущая масса.
- `WhisperAmbientSound`, `GhostAmbientSound`: редкие ambient horror one-shots.
- `BackgroundMusicPlaylist`: музыка.

Настройки микса:

- `ShotVolume`, `BulletPickupVolume`, `HeartbeatVolume`, `LowDroneVolume`
- `Roar1Volume`, `Roar2Volume`, `Roar3Volume`
- `FootstepRun1Volume`, `FootstepRun2Volume`
- `MusicVolume`, `PanicMusicVolume`, `bEnableMusicDucking`
- `MusicDuckFadeTime`, `MusicRestoreFadeTime`
- `PanicSFXSoundClass`, `SFXSoundClass`, `MusicSoundClass`

Panic audio включается после выстрела и смягчается/останавливается после возврата пули. Для проверки баланса включайте `bEnableAudioDebugLogs` или временно используйте audio debug pass в `BP_PanicAudioManager`.

## Враги

Главный C++ класс: `AOBEnemy`. Основные BP: `BP_OBEnemy_Fast` и `BP_OBEnemy_Heavy`.

Базовые настройки в BP врага:

- `EnemyType`: `Fast` или `Heavy`.
- `FastSpeed`: скорость Fast.
- `HeavySpeed`: скорость Heavy.
- `DifficultySpeedMultiplier`: задается WaveManager-ом при росте сложности.
- `CurrentAIState`: runtime-состояние AI, например cautious/rush/guard.
- `CurrentRole`: runtime-роль, например chaser/flanker/blocker.

Поведение:

- Fast обычно быстрее и давит количеством.
- Heavy медленнее, но должен ощущаться как более опасная цель.
- AI использует роли: преследование, фланг, блокирование/охрана пули.
- После Kick враг входит в knockback/stun через `ApplyKick(...)`, затем возвращается к обычному поведению.

Параметры AI State:

- `RushTransitionDelayMin/Max`: задержка перехода в rush.
- `CautiousChaserChance`, `CautiousFlankerChance`
- `CautiousSpeedMultiplierMin/Max`
- `RushChaserChance`, `RushFlankerChance`, `RushBulletBlockerChance`
- `RushSpeedMultiplierMin/Max`
- `BulletGuardRadius`, `BulletGuardInterceptRadius`, `PlayerNearBulletRadius`
- `MinDistanceBetweenEnemyTargets`, `FlankRadiusMin/Max`, `TargetRandomOffsetMin/Max`

Параметры атаки и столкновения:

- `GetEffectiveAttackRadius()` учитывает тип врага и настройки радиуса.
- Враг не должен умирать или отлетать от Kick напрямую в AI коде: реакция начинается из `AOBCharacter::ApplyKickImpact()` и идет через `AOBEnemy::ApplyKick(...)`.
- Knockback внутри врага интерполируется во времени: расстояние, длительность и stun приходят из параметров Kick игрока.

Отладка врагов:

- `AOBEnemy::ToggleDetectionRadiusVisualization()` включает/выключает визуализацию detection radius.
- `bDrawAIRoleTargets` показывает цели ролей AI.
- Для проблем со спавном смотрите `LogOBWaveManager`.
- Для проблем с поведением смотрите `LogOBEnemyAI` и текущие `CurrentAIState` / `CurrentRole` в Details.

## WaveManager и спавн

Главный BP: `/Game/OneBullet/Blueprints/BP_OBWaveManager`.

Если `WaveDefinitions` заполнен, он переопределяет generated waves. Для каждой волны:

- `FastCount`: сколько Fast.
- `HeavyCount`: сколько Heavy.
- `DelayBeforeWave`: задержка перед стартом.
- `SpawnInterval`: интервал между spawn.
- `MaxLiveEnemies`: лимит живых врагов.

Generated/scaling настройки:

- `BaseEnemiesPerWave`
- `AdditionalEnemiesPerWave`
- `HeavyEnemyEveryNWaves`
- `bScaleDifficulty`
- `EnemySpeedIncreasePerWave`
- `MaxEnemySpeedMultiplier`
- `SpawnIntervalMultiplierPerWave`
- `MinimumSpawnInterval`
- `MaxLiveEnemyGrowthEveryNWaves`

Spawning настройки:

- `EnemyClass`, `FastEnemyClass`, `HeavyEnemyClass`
- `BaseSpawnInterval`, `BaseMaxLiveEnemies`
- `MinimumSpawnDistanceFromPlayer`
- `MaxSpawnAttempts`
- `bPreferSpawnPointsOutsidePlayerView`
- `SpawnScreenEdgePadding`
- `DirectViewMinDot`
- `SpawnSafeSearchRadius`

Spawn point должен оставаться простым marker actor. Не завязывать gameplay на rotation/forward vector spawn point. WaveManager выбирает точку, проверяет NavMesh/collision и спавнит нужный enemy class.

## Настройка BP: быстрые рецепты

### Kick ощущается медленным

1. В `BP_OBCharacter` поставить `KickPlayRate = 2.0`.
2. Проверить `KickImpactDelay = 0.16`.
3. Если удар все еще поздний, пробовать `KickImpactDelay = 0.12`.
4. Если impact слишком ранний, пробовать `0.20`.

### Враг отлетает сразу

1. Проверить код: `Enemy->ApplyKick(...)` должен быть только в `ApplyKickImpact()`.
2. Проверить Blueprint graph: не должно быть отдельного knockback на input Kick.
3. Проверить, что Anim Notify или timer вызывает только `ApplyKickImpact()`.

### Kick незаметен

1. Назначить `KickWhooshSound`.
2. Назначить `KickImpactSound`.
3. Назначить `KickCameraShake`.
4. Увеличить `KickFOVPunchAmount`.
5. Настроить `KickImpactCameraPitchPunch` и `KickImpactCameraYawPunch`.
6. Назначить `KickImpactVFX` с dust/impact puff.

### Враги слишком быстрые

1. Уменьшить `FastSpeed` / `HeavySpeed` в enemy BP.
2. Уменьшить `RushSpeedMultiplierMin/Max`.
3. Уменьшить `EnemySpeedIncreasePerWave`.
4. Проверить `MaxEnemySpeedMultiplier`.

### Враги плохо появляются

1. Проверить `BP_EnemySpawnPoint` на карте.
2. Проверить NavMesh.
3. Проверить `MinimumSpawnDistanceFromPlayer`.
4. Проверить `MaxSpawnAttempts`.
5. Проверить, что `FastEnemyClass` и `HeavyEnemyClass` назначены в `BP_OBWaveManager`.

## Проверка перед коммитом

Минимальная сборка:

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' OneBulletLeftEditor Win64 Development -Project='D:\MyProjects\OneBulletLeft\OneBulletLeft.uproject' -WaitMutex -NoHotReload -NoXGE
```

Gameplay smoke test:

1. Открыть `/Game/LevelPrototyping/Lvl_FirstPerson`.
2. Проверить выстрел, потерю пули и возврат пули.
3. Проверить Kick: враг не отлетает при нажатии, а только после `KickImpactDelay`.
4. Проверить Fast и Heavy spawn по `WaveDefinitions`.
5. Проверить, что после Kick управление возвращается нормально.
6. Проверить, что panic audio начинается после выстрела и смягчается после подбора пули.

## Логи

Основной лог:

```text
Saved/Logs/OneBulletLeft.log
```

Полезные категории:

- `LogOBWaveManager`
- `LogOBEnemyAI`
- `LogTemp`

## Репозиторий

Не коммитить:

- `Binaries`
- `Intermediate`
- `Saved`
- `DerivedDataCache`
- `.vs`
- локальные zip/backup архивы

Перед сохранением Blueprint assets убедитесь, что Editor действительно сохранил изменения и что они отражаются в `git status`.
