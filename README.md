# One Bullet Left

Прототип arena shooter на Unreal Engine 5.7, построенный вокруг одного рискованного решения:

> Выстрелить легко. Вернуть единственную пулю - опасно.

У игрока есть только одна пуля. После выстрела она физически летит в мир и остаётся там, куда попала: на теле убитого врага или в точке промаха. Пока игрок не подберёт её обратно, стрелять нельзя и оружие скрыто.

Игровой ритм:

`shot -> lost control -> pressure -> search -> risk -> pickup -> relief`

## Управление

| Действие | Клавиша |
| --- | --- |
| Движение | `WASD` |
| Обзор | `Mouse` |
| Прыжок | `Space` |
| Выстрел | `Left Mouse Button` |
| Пинок / push | `Right Mouse Button` или `F` |
| Уклонение в свободную сторону | `Left Shift` / `Right Shift` |
| Переключить first / third person | `1` |
| Immortal Mode | `2` |
| Перезапуск после Game Over | `R` |

## Gameplay

1. Раунд начинается с заряженной пулей: `Bullet: Ready`; оружие находится в руке.
2. Выстрел сразу переводит состояние в `Bullet: Lost` и скрывает оружие.
3. При попадании враг умирает с одного выстрела, а пуля прилетает на его тело.
4. Тело остаётся на арене, пока пуля не подобрана.
5. При промахе пуля остаётся в точке окончания выстрела.
6. Без пули игрок выживает за счёт движения, уклонения и пинка.
7. Подбор возвращает пулю, оружие и возможность снова выстрелить.

Pickup имеет мягкое притягивание на близкой дистанции, звук, эффект и Blueprint-hook для UI pulse. Цель feedback - сделать момент возвращения пули коротким и очень понятным.

## Оружие И Состояние Пули

Оружие настраивается в `BP_OBCharacter`, а состояние определяется основной механикой:

| Состояние | Визуал | Анимации движения |
| --- | --- | --- |
| `Bullet: Ready` | оружие видно в руке | armed idle/run (`Pistol ...` поля) |
| `Bullet: Lost` | оружие скрыто | standard idle/run |

Выстрел целится из камеры, поэтому попадание соответствует прицелу. Визуальный вылет пули и muzzle flash могут стартовать из сокета оружия `MuzzleFlash`, чтобы выстрел читался корректно и в third person.

Для текущего дробовика:

1. В `Weapon Model` назначить `Shotgun_A`.
2. На скелете персонажа создать удобный socket на кости кисти, например `ShotgunSocket`, и указать его в `Weapon Attach Socket Name`.
3. Положение и поворот оружия довести в `Weapon Ready Relative Transform`.
4. В `Weapon Shoot Animation` назначить анимацию оружия, подходящую дробовику, например `Fire_Shotgun_W`.
5. В `Shoot Animation` назначить анимацию стрельбы самого персонажа, ретаргетнутую под его текущий скелет.
6. Включить `Use Weapon Muzzle For Bullet Flight`, если полёт должен начинаться от ствола.

`Weapon Lost Relative Transform` сохранён в настройках, но при текущем варианте с исчезающим оружием визуально не используется в состоянии `Bullet: Lost`.

## Враги

### Fast Enemy

- Быстро сокращает дистанцию.
- Убивает при контакте.
- Сильно отлетает от пинка.
- Создаёт панику и заставляет постоянно двигаться.

### Heavy Enemy

- Движется медленнее.
- Убивает при контакте.
- Почти не отлетает от пинка, но получает короткий stun.
- Перекрывает путь к потерянной пуле и контролирует пространство.

Враги стараются подходить к игроку с разных направлений. По умолчанию спавн за спиной и убийство из явной слепой зоны отключены, потому что основной режим игры - first person.

## Волны

В `BP_OBGameMode` включена короткая pacing-структура:

| Волна | Состав |
| --- | --- |
| 1 | 2 Fast |
| 2 | 1 Fast + 1 Heavy |
| 3 | 3 Fast + 2 Heavy |

Все параметры меняются в `Wave Definitions`: количество врагов, задержка, интервал спавна и лимит живых врагов.

## Основные Blueprint Assets

Игровые Blueprint находятся в `/Game/OneBullet/Blueprints`.

| Asset | Ответственность |
| --- | --- |
| `BP_OBGameMode` | flow раунда, волны, классы врагов и pickup, правила спавна |
| `BP_OBCharacter` | внешний вид игрока, оружие, анимации, звуки и feedback действий |
| `BP_OBEnemy_Fast` | вид и feedback быстрого врага |
| `BP_OBEnemy_Heavy` | вид и feedback тяжёлого врага |
| `BP_OBBulletPickup` | вид пули, pickup feedback, Niagara trail и читаемость |
| `BP_OBHUD` | создаёт виджет HUD |
| `WBP_OBHUD` | визуальная сборка UI и UI-анимации |
| `BP_OBGameState` | состояние пули, kills, Game Over и статистика |

Уровень прототипа: `/Game/FirstPerson/Lvl_FirstPerson`.

## Что В C++

C++ поддерживает стабильную игровую механику:

- состояние одной пули `Ready / Lost`;
- выстрел, hit detection и убийство с одного попадания;
- видимость оружия в зависимости от наличия пули;
- вылет presentation-пули и muzzle flash из оружия при включённой настройке;
- создание pickup и физический возврат пули в мир;
- Niagara trail летящей пули;
- overlap, magnet pickup и восстановление выстрела;
- AI-преследование, окружение и touch-kill;
- различие Fast / Heavy, pushback и stun;
- уклонение в свободную от препятствий и врагов сторону;
- first / third person режим и Immortal Mode;
- короткие волны, рестарт раунда и игровая статистика;
- передача gameplay state и событий в Blueprint.

Оформление, VFX, звуки, модели и анимации намеренно оставлены для Blueprint.

## Настройка В Blueprint

### `BP_OBGameMode`

Ключевые поля в `One Bullet Settings`:

| Поле | Назначение |
| --- | --- |
| `Use Scripted Waves` | включает короткие настроенные волны |
| `Wave Definitions` | состав и timing каждой волны |
| `Fast Enemy Class` / `Heavy Enemy Class` | используемые классы врагов |
| `Bullet Pickup Class` | класс физической пули |
| `Spawn Enemies Only In Front Of Player` | запрещает спавн за спиной |
| `Front Spawn Min Dot` | ограничивает допустимый передний сектор |
| `Allow Any Spawn If No Front Point` | fallback, если впереди нет точки |
| `Bullet Pickup Drop Height` | высота пули относительно точки попадания |
| `Force Windowed Mode` | запуск прототипа в окне |

### `BP_OBCharacter`

#### Анимации

| Поле | Когда используется |
| --- | --- |
| `Standard Idle Animation (Bullet Lost)` | игрок стоит без пули |
| `Standard Run Animation (Bullet Lost)` | игрок движется без пули |
| `Pistol Idle Animation (Bullet Ready)` | armed idle с оружием; имя поля историческое и подходит для дробовика |
| `Pistol Run Animation (Bullet Ready)` | armed run с оружием; сюда назначается текущий `Pistol_run` или другой armed run |
| `Shoot Animation` | анимация стрельбы персонажа |
| `Death Animation` | смерть персонажа |
| `Kick Animation` | пинок персонажа |
| `Use Simple Locomotion Animations` | включает переключение idle/run в зависимости от скорости и пули |

`Shoot Animation`, idle и run должны быть совместимы со скелетом текущего персонажа. Если animation asset создан для другого skeleton, Unreal пропустит проигрывание или выдаст ошибку позы; такую анимацию нужно ретаргетнуть.

#### Оружие И Выстрел

| Поле | Назначение |
| --- | --- |
| `Weapon Model` | skeletal mesh оружия, сейчас можно использовать `Shotgun_A` |
| `Weapon Attach Socket Name` | socket на руке персонажа, например `ShotgunSocket` |
| `Weapon Ready Relative Transform` | положение оружия в руке при `Bullet: Ready` |
| `Weapon Lost Relative Transform` | резервное поле позы без пули; оружие сейчас скрывается |
| `Weapon Pose Blend Speed` | скорость перехода позы, если Lost-вид снова понадобится |
| `Shoot Effect` | muzzle flash / VFX выстрела |
| `Shoot Effect Socket Name` | socket эффекта на оружии, обычно `MuzzleFlash` |
| `Shoot Sound` | звук выстрела |
| `Weapon Shoot Animation` | отдельная анимация самого оружия, для дробовика - `Fire_Shotgun_W` |
| `Use Weapon Muzzle For Bullet Flight` | начинает видимый полёт пули от ствола |

Также здесь настраиваются `Dry Fire Sound`, `Kick Sound`, `Dodge Sound`, `Death Sound`, `Hit Confirm Sound`, recoil, camera shake, hit-stop, дистанции/cooldown пинка и уклонения, third-person camera и стартовый Immortal Mode.

Blueprint-события игрока:

| Событие | Для чего использовать |
| --- | --- |
| `OnPlayerShoot` | дополнительный shot VFX / feedback |
| `OnPlayerHitConfirmed` | impact VFX и hit feedback |
| `OnPlayerBulletRecovered` | звук облегчения и pulse подбора |
| `OnPlayerWeaponStateChanged` | реакция на поднятое/скрытое оружие |
| `OnPlayerDryFire` | feedback попытки стрелять без пули |
| `OnPlayerKick` | дополнительная анимация и VFX пинка |
| `OnPlayerDodge` / `OnPlayerDodgeFailed` | whoosh, dash trail, feedback cooldown |
| `OnPlayerDeath` | death VFX и presentation |
| `OnPlayerViewModeChanged` | UI/камера при смене вида |
| `OnPlayerImmortalModeChanged` | feedback debug-режима |

### `BP_OBEnemy_Fast` / `BP_OBEnemy_Heavy`

Настраиваются `Enemy Type`, скорость, touch-kill, pressure/окружение, `Idle Animation`, `Run Animation`, `Death Animation`, `Death Sound`, а также крепление пули к телу: `Bullet Attach Bone` и `Bullet Attach Offset`.

#### Настройки поведения врагов

Открывать в Unreal Editor: `/Game/OneBullet/Blueprints/BP_OBEnemy_Fast` и `/Game/OneBullet/Blueprints/BP_OBEnemy_Heavy`.
Все поля ниже находятся в Details у Blueprint-класса врага, в категориях `OneBulletSettings` и `OneBulletSettings|Pressure`.

| Поле | Сейчас | Что делает |
| --- | ---: | --- |
| `Fast Speed` | `760` | Базовая скорость Fast-врага, когда пули нет. |
| `Heavy Speed` | `260` | Базовая скорость Heavy-врага, когда пули нет. |
| `Player Has Bullet Speed Multiplier` | `0.35` | Множитель скорости в режиме патруля, когда у игрока есть пуля. `0.35` значит 35% от базовой скорости. |
| `Player Has Bullet Attack Speed Multiplier` | `0.30` | Множитель скорости, когда пуля есть, но игрок вошел в радиус атаки. |
| `Player Has Bullet Attack Radius` | `700` | Радиус реакции при наличии пули. Снаружи враг патрулирует; внутри начинает нападать. |
| `Patrol Radius` | `2600` | Размер территории патруля вокруг `Patrol Center`. |
| `Patrol Center` | `(0, 0, 0)` | Центр арены для выбора дальних патрульных точек. |
| `Patrol Perimeter Radius Multiplier` | `0.78` | Насколько близко к периметру арены враг выбирает патрульные точки. |
| `Patrol Perimeter Step Degrees` | `45` | Угол следующего сектора периметра для патруля. |
| `Fast Surround Radius` | `320` | Радиус окружения Fast-врага, когда пули нет. |
| `Heavy Surround Radius` | `420` | Радиус окружения Heavy-врага, когда пули нет. |

Логика состояний:

1. `Bullet: Ready`: враги медленно патрулируют по территории вокруг `Patrol Center`.
2. `Bullet: Ready` и игрок ближе `Player Has Bullet Attack Radius`: враги атакуют со скоростью `Base Speed * Player Has Bullet Attack Speed Multiplier`.
3. `Bullet: Lost`: враги используют обычную скорость и окружают игрока через `Fast Surround Radius` / `Heavy Surround Radius`.

| Событие | Данные | Применение |
| --- | --- | --- |
| `OnEnemyDeath` | `Drop Location` | death effect, звук, дополнительная реакция |
| `OnEnemyKicked` | `Direction`, `Type` | hit reaction / эффект пинка |
| `OnEnemySpawned` | `Type`, `Location` | spawn VFX |
| `OnEnemyDisappearing` | `Type`, `Location` | dissolve / disappear VFX |

Spawn/disappear VFX собираются нодами в каждом BP врага, поэтому их легко заменить без изменения C++.

### `BP_OBBulletPickup`

| Поле | Назначение |
| --- | --- |
| `Pickup Sound` | звук успешного возврата пули |
| `Pickup Effect` | Niagara при подборе |
| `Trail Niagara System` | след летящей пули |
| `Trail Flight Duration` / `Trail Travel Speed` | читаемая скорость полёта |
| `Main Mesh Scale` | размер основной пули в мире |
| `Traveling Bullet Scale` | размер представления пули в полёте / fallback |
| `Pickup Radius` / `Magnet Radius` / `Magnet Speed` | forgiving pickup |
| `Use Native Presentation Animation` | встроенные bob/spin/pulse |

Важно: при включённом `Use Native Presentation Animation` масштаб меша пули обновляется из `Main Mesh Scale` во время игры. Поэтому увеличить новую маленькую модель нужно именно этим полем в BP, а не `Transform / Scale` компонента. Удобная стартовая проба для маленького mesh: `Main Mesh Scale = 1.5`, `Traveling Bullet Scale = 0.3`, затем подогнать глазами.

### `WBP_OBHUD`

`WBP_OBHUD` отвечает за шрифты, контейнеры, цвета, расположение, видимость и UI-анимации. C++ передаёт данные и вызывает события.

| Getter | Значение |
| --- | --- |
| `Is Bullet Ready` | можно ли стрелять |
| `Get Kill Count` | количество убийств |
| `Is Game Over` | завершён ли раунд |
| `Get Last Run Time` | время законченной попытки |
| `Get Best Kill Count` | лучший результат |
| `Get Death Reason` | причина смерти |
| `Is Dodge Ready` | доступность уклонения |
| `Get Dodge Cooldown Normalized` | прогресс cooldown |
| `Is Immortal Mode` | активен ли бессмертный режим |

| Событие HUD | Рекомендуемая реакция |
| --- | --- |
| `OnHudInitialized` | собрать стартовое отображение |
| `OnBulletStateChanged` | изменить `Bullet: Ready / Lost` и warning |
| `OnBulletRecovered` | проиграть UI-анимацию `BulletReadyPulse` |
| `OnKillCountChanged` | обновить kills |
| `OnGameOverChanged` | показать Game Over, fade и статистику |
| `OnDodgeCooldownChanged` | обновить индикатор уклонения |
| `OnImmortalModeChanged` | показать или скрыть `ImmortalModeTxt` |

Пример статистики для `Format Text`:

```text
{Reason}
Killed: {Kills}
Time survived: {Time}s
Best kills: {Best}
```

## Проверка Перед Демонстрацией

1. В `BP_OBCharacter` проверить `Shotgun_A`, socket руки, `Fire_Shotgun_W`, player `Shoot Animation` и armed idle/run.
2. Сделать один выстрел: дробовик должен исчезнуть в `Bullet: Lost`, а пуля вылететь от ствола.
3. Подобрать пулю: дробовик должен вернуться, HUD - переключиться в `Bullet: Ready`.
4. В `BP_OBBulletPickup` подобрать читаемый `Main Mesh Scale`, чтобы пуля не терялась на арене.
5. Проверить смерть, `R`, `Shift`, `1` и `2` в Play mode.

## Сборка

Проект собирается под `Windows / Development`.

При packaging на этой машине XGE пытался одновременно компилировать много файлов и падал из-за виртуальной памяти:

```text
C3859: Failed to create virtual memory for PCH
C1076: compiler limit: internal heap limit reached
```

Для стабильной сборки в пользовательском UnrealBuildTool config отключён XGE и установлен один compile action:

`%APPDATA%/Unreal Engine/UnrealBuildTool/BuildConfiguration.xml`

```xml
<Configuration xmlns="https://www.unrealengine.com/BuildConfiguration">
    <BuildConfiguration>
        <bAllowXGE>false</bAllowXGE>
        <MaxParallelActions>1</MaxParallelActions>
    </BuildConfiguration>
</Configuration>
```

После изменения C++ при открытом Unreal Editor нужно применить Live Coding (`Ctrl+Alt+F11`) либо закрыть редактор и собрать target заново. Сборка идёт медленнее, но избегает прежнего падения по памяти.

## Заметки

- После добавления `UPROPERTY` поля появятся в Blueprint Details только после компиляции модуля и перезапуска/обновления BP.
- Если персонаж вытягивает шею или ломается поза, причина обычно в несовместимой анимации: проверьте skeleton и сделайте retarget.
- Player `Shoot Animation` и `Weapon Shoot Animation` - разные assets: первая двигает персонажа, вторая механизмы оружия.
- Для Niagara trail важны читаемые lifetime и скорость полёта; слишком быстрый эффект визуально превращается в вспышку.
- Это компактный playable prototype: логика сосредоточена на цикле `потерять пулю -> рискнуть -> вернуть контроль`.
