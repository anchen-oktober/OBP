# One Bullet Left

Короткий список важных настроек проекта.

## Карта

- Основная карта: `/Game/LevelPrototyping/Lvl_FirstPerson`
- Основная арена: `MapBloclout`
- Static Mesh арены: `/Game/LevelPrototyping/Meshes/MapBloclout_01`

## Коллизия арены

На `MapBloclout_01`:

- `Collision Complexity`: `Use Complex Collision As Simple`

На placed actor `MapBloclout` в карте:

- `Collision Enabled`: `Query and Physics`
- `Collision Preset`: `BlockAll`
- `Can Ever Affect Navigation`: `true`

Не добавлять отдельные collision boxes для арены. Коллизия должна идти от самой геометрии Static Mesh.

## NavMesh

- `Nav Mesh Bounds Volume` должен покрывать всю арену, включая платформы, рампы и возвышения.
- После правок арены перестроить NavMesh.
- Проверить, что зелёная сетка есть на полу, проходах и доступных возвышениях.
- Если NavMesh плохо строится на мелких деталях, сначала проверять `Agent Radius`, `Step Height`, `Walkable Slope`.

## Gameplay

- Игрок не проваливается через арену.
- Игрок бегает по полу, платформам, рампам и возвышениям.
- Игрок и враги не проходят через стены и крупные препятствия.
- Враги строят путь по NavMesh и обходят геометрию.

## Звуки

Основной manager:

- Blueprint: `/Game/OneBullet/Blueprints/BP_PanicAudioManager`
- C++: `AOBPanicAudioManager`
- GameMode сам создаёт/находит Panic Audio Manager.

Ключевые ассеты:

- `ShotSound`: звук выстрела
- `HeartbeatSound`: heartbeat loop после потери пули
- `LowDroneSound`: низкий тревожный слой
- `RoarSound1/2/3`: рыки толпы
- `FootstepSound1/2`: шаги/преследование
- `BulletPickupSound`: relief при возврате пули
- `AmbientHorrorSounds`: случайные хоррор-звуки

Где настраивать:

- `BP_PanicAudioManager`
- категория `OneBulletSettings|Panic Audio`

События:

- выстрел: запускает panic audio
- подбор пули: останавливает panic audio и играет pickup/relief
- Game Over / Reset: останавливает активные audio layers

## Сборка

Target:

- `OneBulletLeftEditor`
- `Win64`
- `Development`

Стабильная команда:

```powershell
& 'C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' OneBulletLeftEditor Win64 Development -Project='D:\MyProjects\OneBulletLeft\OneBulletLeft.uproject' -WaitMutex -NoHotReload -NoXGE
```
