# One Bullet Left

`One Bullet Left` is a UE5 first-person arena prototype. The player has one important bullet, has to survive pressure from enemies, recover the bullet, and create space with a fast emergency kick.

## Quick Start

Open the project:

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe' 'D:\MyProjects\OneBulletLeft\OneBulletLeft.uproject'
```

Build the editor target:

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' OneBulletLeftEditor Win64 Development -Project='D:\MyProjects\OneBulletLeft\OneBulletLeft.uproject' -WaitMutex -NoHotReload -NoXGE
```

If Unreal reports that Live Coding is active, close the editor/game or press `Ctrl+Alt+F11`, then build again.

## Main Assets

- Level: `/Game/LevelPrototyping/Lvl_FirstPerson`
- Player BP: `/Game/OneBullet/Blueprints/BP_OBCharacter`
- GameMode BP: `/Game/OneBullet/Blueprints/BP_OBGameMode`
- WaveManager BP: `/Game/OneBullet/Blueprints/BP_OBWaveManager`
- Fast enemy BP: `/Game/OneBullet/Blueprints/BP_OBEnemy_Fast`
- Heavy enemy BP: `/Game/OneBullet/Blueprints/BP_OBEnemy_Heavy`
- First-person kick mesh: `/Game/Assets/Player_Leg`
- First-person kick animation: `/Game/Assets/Animations/KickingLeg_Anim`

## Controls

- `WASD`: move
- `Mouse`: look
- `Left Mouse Button`: shoot
- `Right Mouse Button` / `F`: kick
- `Left Shift`: dodge
- `Space`: jump
- `R`: restart run

## First-Person Kick

Kick is now an immediate emergency action, not a delayed animation event.

Runtime flow in `AOBCharacter::Kick()`:

1. Start the first-person `Player_Leg` visual immediately.
2. Start weapon sway and kick camera feedback immediately.
3. Run `ApplyKickImpact()` immediately on input press.
4. If an enemy is inside the forward kick zone, apply knockback/stun and hit feedback.
5. If no enemy is hit, play only the miss/whoosh feedback.

Important rules:

- No third-person kick montage is used.
- No third-person camera/view mode is used.
- Kick gameplay does not wait for animation notify, montage notify, or a timer.
- Camera shake happens once, immediately on kick input.
- Hit sound only plays on hit; miss/whoosh only plays on miss.

## Player_Leg Visual

`Player_Leg` is a `USkeletalMeshComponent` attached to `FirstPersonCamera`.

Expected hierarchy:

```text
Character
`-- FirstPersonCamera
    `-- Player_Leg
```

`Player_Leg` should be edited in `BP_OBCharacter` viewport. By default, the component transform from the Blueprint viewport is the source of truth.

Relevant BP settings:

- `Kick Leg Relative Location`
- `Kick Leg Relative Rotation`
- `Kick Leg Relative Scale`
- `bOverrideKickLegTransformFromVariables`
- `bKickVisualCalibrationMode`
- `KickVisualCalibrationPlayRate`
- `bKickUsesRightLeg`
- `KickVisualHideEarlyTime`

Use `bOverrideKickLegTransformFromVariables = false` when positioning the component directly in the Blueprint viewport. Turn it on only if you want the C++ exposed transform variables to drive the component.

## Kick Leg-Only Mask

The kick can use a full-body skeletal mesh as the animation source, but the first-person view must show only the kicking leg.

`ApplyKickLegOnlyMask()` runs every time before `KickingLeg_Anim` starts:

- Keeps `root` and `pelvis`.
- Keeps only the selected kicking leg:
  - right: `thigh_r`, `calf_r`, `foot_r`, `ball_r`
  - left: `thigh_l`, `calf_l`, `foot_l`, `ball_l`
- Hides all other bones, including arms, fingers, upper body, head, and the non-kicking leg.

`KickVisualHideEarlyTime` hides `Player_Leg` slightly before the animation ends, so the player does not see a one-frame ref-pose flash with hands or upper body.

Calibration mode keeps `Player_Leg` visible, but the leg-only mask stays active.

## Debug Logs

Kick visual logs are written through `LogTemp`.

Useful kick log entries include:

- runtime character class and expected class
- `Player_Leg` parent
- Blueprint/component relative transform before and after configure
- `bOverrideKickLegTransformFromVariables`
- `bKickUsesRightLeg`
- hidden bone count and hidden bone list
- visible leg whitelist
- animation length
- actual hide delay
- whether `Player_Leg` was hidden before ref-pose reset

If `bKickVisualDebug` is enabled, short on-screen messages also show mask status and hide timing.

## Performance Debug

`AOBCharacter` also exposes performance snapshot helpers:

- `TogglePerformanceDebug()`
- `SetPerformanceDebugEnabled(bool)`
- `DumpPerformanceSnapshot()`

BP/config settings:

- `bPerformanceDebugEnabled`
- `PerformanceDebugLogInterval`
- `bPerformanceDebugScreenMessages`

Snapshots are logged under `LogOBPerformance` and include FPS/frame time, enemy counts, AI controllers, active audio, widgets, wave counters, bullet pickups, bullet trails, and approximate VFX/debug actor counts.

## Smoke Test

Before committing gameplay changes:

1. Build `OneBulletLeftEditor`.
2. Open `/Game/LevelPrototyping/Lvl_FirstPerson`.
3. Press kick with an enemy directly in front: enemy should immediately knock back/stun.
4. Press kick with no enemy in range: only leg animation and miss/whoosh should play.
5. Confirm there is only one kick camera shake and it happens immediately.
6. Confirm `Player_Leg` is visible from first person.
7. Confirm no hands, upper body, or non-kicking leg appear during the kick.
8. Move `Player_Leg` in `BP_OBCharacter` viewport and confirm the runtime visual follows without C++ recompilation.

## Files To Avoid Committing

Do not commit generated or local cache folders:

- `Binaries`
- `Intermediate`
- `Saved`
- `DerivedDataCache`
- `.vs`
- local zip/backup archives
