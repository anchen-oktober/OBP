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
| Уклонение по направлению движения | `Left Shift` / `Right Shift` + `WASD` |
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

### AI States

Враги используют явное состояние `EOBEnemyAIState` и роль `EOBEnemyRole`. Текущее состояние и роль доступны в `BP_OBEnemy_Fast` / `BP_OBEnemy_Heavy` через `Current AI State`, `Current Role`, `Get AI State` и `Get AI Role`.

#### Cautious

Активно, пока у игрока есть пуля (`Bullet: Ready`).

- роли: `20% Chaser`, `80% Flanker`;
- скорость: случайное значение `70-85%` базовой скорости с дополнительным разбросом `+-10%`;
- все движущиеся враги используют только `Walk Animation`; `Run Animation` в Cautious не выбирается;
- Chaser идёт прямо к игроку, постоянно сокращает дистанцию и не выполняет обходные манёвры;
- Flanker один раз получает стабильный слот слева, справа или позади игрока;
- Flanker корректирует слот относительно движения игрока и постепенно уменьшает радиус окружения от `Cautious Flanker Distance` до `Cautious Flanker Min Distance`;
- скорость сжатия задаётся `Cautious Compression Speed`; орбитальное смещение и бесконечное движение по кругу не используются.

#### Rush

Запускается после выстрела (`Bullet: Lost`) с индивидуальной задержкой каждого врага `0.0-0.35s`.

- роли: `70% Chaser`, `10% Flanker`, `20% Bullet Blocker`;
- скорость: случайное значение `110-130%` базовой скорости с дополнительным разбросом `+-10%`;
- все движущиеся враги используют `Run Animation`;
- Chaser преследует игрока;
- Flanker занимает ближнюю боковую позицию;
- Bullet Blocker занимает точку на прямом отрезке между игроком и активным bullet pickup, перекрывая кратчайший путь; положение на отрезке задаётся `Bullet Blocker Path Fraction`.

Роль и слот назначаются только при входе в состояние, поэтому они не скачут каждый кадр. Navigation target обновляется существующим таймером движения, а не в `Tick`. При возврате пули незавершённый Rush timer отменяется, враг сразу возвращается в Cautious и получает новую роль/скорость. Смена состояния проходит через `SetAIState`, сбрасывает текущую navigation target и вызывает `OnEnemyAIStateChanged`. В Output Log выводятся строки категории `LogOBEnemyAI` с прежним/новым состоянием, ролью и итоговым speed multiplier.

Все параметры находятся в Enemy Blueprint Details:

| Категория | Основные поля |
| --- | --- |
| `OneBulletSettings|AI State|Transition` | `Rush Transition Delay Min/Max` |
| `OneBulletSettings|AI State|Cautious` | role chances, speed min/max, variance, Flanker max/min distance, compression speed |
| `OneBulletSettings|AI State|Rush` | role chances, speed min/max, variance, Flanker distance min/max, Bullet Blocker acceptance radius |
| `OneBulletSettings|AI State|Debug` | `Draw AI Role Targets`, размер маркера цели |

### Enemy Roles

Роли реализованы отдельными функциями расчёта цели: `CalculateChaserTarget`, `CalculateFlankerTarget`, `CalculateBulletBlockerTarget`. Общая отправка navigation request остаётся в `RequestMove`, поэтому логика движения не дублируется.

| Роль | Цель |
| --- | --- |
| `Chaser` | текущая позиция игрока; преследование выполняется через `MoveToActor` |
| `Flanker` | случайно выбранная левая или правая точка относительно направления движения/взгляда игрока; offset случайно фиксируется в диапазоне `300-600` при назначении роли |
| `BulletBlocker` | точный `MidPoint(PlayerLocation, BulletLocation)` |

`BulletBlocker` участвует в распределении ролей только если активный bullet pickup уже существует. Если pickup исчез до обновления состояния, используется безопасная Flanker-цель, а возврат пули переводит врага обратно в `Cautious`.

Для проверки включить `Draw AI Role Targets` в Enemy BP:

| Цвет | Роль |
| --- | --- |
| красный | `Chaser` |
| жёлтый | `Flanker` |
| голубой | `BulletBlocker` |

Debug draw показывает линию от врага до navigation target, сферу цели и подпись `State / Role`.

### Group Movement Diversity

Распределение ролей выполняется общей функцией `AssignEnemyRoles()`:

- `1` живой враг: всегда `Chaser`;
- `2` живых врага: минимум один `Chaser`, второй получает доступную роль состояния;
- `3+` врага: используются проценты `Cautious` / `Rush`, но минимум один `Chaser` гарантирован;
- роли пересчитываются при появлении врага, смерти и смене AI state, а не каждый кадр.

`Chaser` всегда использует `MoveToActor(Player)` без бокового offset. `Flanker` получает отдельный угловой слот сбоку или сзади, случайный радиус и небольшое смещение. `BulletBlocker` выбирает собственную точку рядом с линией между игроком и пулей.

Не-Chaser цели используют reservation-проверку. Если новая точка находится ближе `Min Distance Between Enemy Targets` к уже назначенной цели, выполняется новая попытка. При исчерпании попыток выбирается кандидат с наибольшим разделением.

Настройки в `OneBulletSettings|AI State`:

| Категория | Параметры по умолчанию |
| --- | --- |
| `Movement Diversity` | target spacing `300 cm`, flank radius `500-900 cm`, random offset `100-250 cm`, `6` attempts |
| `Repath` Chaser | `0.2-0.5 s` |
| `Repath` Flanker | `0.8-1.5 s` |
| `Repath` Bullet Blocker | `0.4-0.8 s` |

Каждый враг использует индивидуальный one-shot repath timer с дополнительным случайным множителем, поэтому толпа не обновляет пути одновременно. `Draw AI Role Targets` показывает цвет роли над врагом, линию и сферу текущей target position.

#### Flanker Compression

Flanker закрепляется в одном из четырёх секторов: left, right, rear-left или rear-right. Сектор не меняется при обычном перераспределении ролей. Направление цели фиксируется на `Flank Side Lock Duration` (`2-4 s`), после чего мягко обновляется относительно движения игрока без перескока на противоположную сторону.

Радиус цели уменьшается непрерывно:

| State | Start Radius | Min Radius | Approach Speed |
| --- | ---: | ---: | ---: |
| `Cautious` | `900 cm` | `450 cm` | `80-150 cm/s` |
| `Rush` | `700 cm` | `250 cm` | `200-350 cm/s` |

Новая flank-цель не может оказаться дальше текущей дистанции врага до игрока более чем на `Max Allowed Flank Distance Increase` (`100 cm`). Случайность ограничена небольшим боковым offset внутри закреплённого сектора. Когда Flanker достигает минимального радиуса плюс `Flank Close Pressure Range`, обход прекращается и враг начинает напрямую давить игрока, исключая бесконечное orbit movement.

### Fast Enemy

- Быстро сокращает дистанцию.
- Убивает при контакте.
- Сильно отлетает от пинка.
- Создаёт панику и заставляет постоянно двигаться.

### Heavy Enemy

- Движется медленнее.
- Контролирует ближнюю зону: attack radius по умолчанию `200 cm`, вдвое больше Fast `100 cm`.
- Убивает игрока при входе в эффективный attack radius.
- Почти не отлетает от пинка, но получает короткий stun.
- Перекрывает путь к потерянной пуле и контролирует пространство.

#### Attack Radius

Настройки находятся в `BP_OBEnemy_Fast` / `BP_OBEnemy_Heavy`, категория `OneBulletSettings|Attack Radius`:

| Поле | Значение |
| --- | ---: |
| `Fast Attack Radius` | `100 cm` |
| `Heavy Attack Radius` | `200 cm` |
| `Touch Kill Extra Margin` | дополнительный запас для физического контакта капсул |
| `Draw Attack Radius` | включает визуализацию итоговой зоны атаки |
| `Attack Radius Debug Thickness` | толщина debug-круга |

Урон и debug draw используют один getter `GetEffectiveAttackRadius`. Итоговый радиус не может быть меньше суммы радиусов капсул игрока/врага и `Touch Kill Extra Margin`. Размер collision capsule Heavy остаётся прежним (`58 cm` radius): увеличение attack radius меняет зону угрозы, но не физическую навигационную коллизию.

При включённом `Draw Attack Radius` Fast показывает оранжевый круг, Heavy - красный. Подпись над врагом отображает фактический радиус в сантиметрах.

Для Heavy включено `Can Touch Kill From Behind`, поэтому красный круг является полноценной зоной угрозы на 360 градусов. Fast сохраняет фронтальное ограничение контактной атаки.

Враги стараются подходить к игроку с разных направлений. По умолчанию спавн за спиной и убийство из явной слепой зоны отключены, потому что основной режим игры - first person.

## Волны

Волнами и спавном врагов управляет отдельный `BP_OBWaveManager`. `BP_OBGameMode` только находит существующий Wave Manager на уровне или создаёт экземпляр класса, указанного в `Wave Manager Class`.

Состояния волны:

`Waiting -> Active -> Completed -> Intermission -> Active`

- `Waiting`: начальная задержка перед первой волной;
- `Active`: враги постепенно спавнятся, игрок продолжает сражаться;
- `Completed`: короткое состояние для сообщения `Wave Cleared`;
- `Intermission`: пауза `3-5 s`, спавн отключён, управление игроком сохраняется;
- следующая волна начинается автоматически после завершения таймера.

Волна завершается только тогда, когда `Enemies Remaining To Spawn == 0` и `Living Enemy Count == 0`. Смерть и уничтожение каждого врага отслеживаются Wave Manager, поэтому параллельного счётчика в Game Mode нет.

Стартовая pacing-структура:

| Волна | Состав |
| --- | --- |
| 1 | 2 Fast |
| 2 | 1 Fast + 1 Heavy |
| 3 | 3 Fast + 2 Heavy |

Количество врагов разных видов задаётся в `BP_OBWaveManager -> Wave Definitions (Overrides Generated Waves)`:

| Поле элемента | Назначение |
| --- | --- |
| `Fast Count` | количество Fast-врагов |
| `Heavy Count` | количество Heavy-врагов |
| `Delay Before Wave` | задержка перед этой волной |
| `Spawn Interval` | интервал между попытками спавна |
| `Max Live Enemies` | лимит одновременно живых врагов |

Важно: если `Wave Definitions` не пуст, именно этот массив является источником состава волн. Поля `Base Enemies Per Wave` и остальные настройки секции `Generated` используются только при пустом массиве, а после последнего scripted-элемента участвуют в дальнейшем масштабировании.

### Централизованный Спавн

`BP_OBWaveManager` является единственной точкой создания врагов. Внешние системы могут запускать или останавливать волны через публичный API, но не должны вызывать `SpawnActor` для врагов самостоятельно.

Основной Blueprint API:

- `Start Waves`, `Start Wave`, `Restart Waves`;
- `Stop Wave`, `Stop Waves`, `Complete Current Wave`;
- `Spawn Enemy`, `Clear Spawned Enemies`;
- события `On Wave State Changed`, `On Wave Started`, `On Wave Completed`;
- события `On Enemy Spawned`, `On Enemy Died`, `On Enemy Count Changed`, `On Spawn Warning`.

При спавне Wave Manager передаёт врагу тип и множитель сложности, подписывается на смерть/уничтожение и запускает spawn protection. Затем обычная AI-система сама выбирает актуальное состояние `Cautious` или `Rush` и распределяет роли `Chaser`, `Flanker`, `BulletBlocker`.

### Безопасный Спавн

Точка спавна должна находиться не ближе `Minimum Spawn Distance From Player` (`800-1200 units`). Среди допустимых точек система предпочитает:

- точки вне экрана;
- точки, закрытые геометрией по `Visibility` trace;
- точки позади направления камеры;
- точки ближе к краям арены.

Если все точки слишком близко к игроку, спавн откладывается до следующего тика таймера. Перед появлением создаётся warning VFX и/или короткая вспышка света. Враг остаётся скрытым и безопасным во время warning, а после появления получает grace period, в течение которого не атакует и не наносит урон.

Рекомендуемые диапазоны:

| Поле | Диапазон |
| --- | ---: |
| `Spawn Warning Duration` | `0.3-0.7 s` |
| `Spawn Grace Period` | `0.3-0.5 s` |
| `Minimum Spawn Distance From Player` | `800-1200 units` |

Debug вывод категории `LogOBWaveManager` сообщает номер и состав волны, выбранную точку, дистанцию до игрока, видимость точки, количество живых врагов и завершение волны.

## Основные Blueprint Assets

Игровые Blueprint находятся в `/Game/OneBullet/Blueprints`.

| Asset | Ответственность |
| --- | --- |
| `BP_OBGameMode` | flow раунда, создание/поиск Wave Manager, класс pickup и рестарт |
| `BP_OBWaveManager` | волны, состав врагов, масштабирование сложности, безопасный спавн и учёт живых врагов |
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
- glow, вертикальный пульсирующий луч, drop trail и звук падения пули;
- AI-преследование, окружение и touch-kill;
- различие Fast / Heavy, pushback и stun;
- уклонение в свободную от препятствий и врагов сторону;
- first / third person режим и Immortal Mode;
- централизованные волны, безопасный спавн, рестарт раунда и игровая статистика;
- Wave UI с количеством оставшихся врагов и countdown;
- передача gameplay state и событий в Blueprint.

Оформление, VFX, звуки, модели и анимации намеренно оставлены для Blueprint.

## Настройка В Blueprint

### `BP_OBGameMode`

Ключевые поля в `One Bullet Settings`:

| Поле | Назначение |
| --- | --- |
| `Wave Manager Class` | Blueprint-класс единого менеджера волн и спавна |
| `Bullet Pickup Class` | класс физической пули |
| `Bullet Pickup Drop Height` | высота пули относительно точки попадания |
| `Force Windowed Mode` | запуск прототипа в окне |

Настройки состава волн, классов врагов и точек спавна в Game Mode больше не используются: они находятся в `BP_OBWaveManager`.

### `BP_OBWaveManager`

#### Волны И Масштабирование

| Категория | Основные поля |
| --- | --- |
| `Waves` | `Auto Start Waves`, `Wave Definitions` |
| `Waves / Timing` | `Initial Wait Duration`, `Intermission Duration`, `Completed State Duration` |
| `Waves / Completion` | `Auto Complete When All Enemies Defeated` |
| `Waves / Generated` | базовое количество, прирост врагов, частота Heavy |
| `Waves / Scaling` | рост скорости, минимальный spawn interval, рост лимита живых |
| `Waves / Runtime` | текущее состояние, номер волны, осталось заспавнить, живые враги, difficulty multiplier |

#### Спавн

| Категория | Основные поля |
| --- | --- |
| `Spawning` | `Fast Enemy Class`, `Heavy Enemy Class`, `Spawn Points`, базовый интервал и лимит |
| `Spawning / Safety` | минимальная дистанция, предпочтение вне экрана, screen padding, direct-view dot |
| `Spawning / Warning` | warning duration, grace period, Niagara effect, цвет/радиус/интенсивность вспышки |
| `Debug` | логи, screen messages и длительность сообщений |

Для настройки количества Fast и Heavy по волнам редактировать нужно `Wave Definitions` именно в `BP_OBWaveManager`. Значения из старых полей Game Mode игнорируются намеренно.

### `BP_OBCharacter`

#### Уклонение

Уклонение срабатывает по текущему направлению управления: `Shift + W` - вперёд, `Shift + A` - влево, `Shift + S` - назад, `Shift + D` - вправо. Диагонали тоже поддерживаются. Если направление не зажато, используется рывок вперёд, чтобы нажатие `Shift` не превращалось в незаметный no-op.

Поля настраиваются в `BP_OBCharacter` в категории `OneBulletSettings|Dodge`:

| Поле | Сейчас | Что делает |
| --- | ---: | --- |
| `Dodge Distance` | `950` | Длина рывка в сантиметрах. Главный параметр заметности манёвра. |
| `Dodge Duration` | `0.10` | Время рывка в секундах. Меньше значение - резче и быстрее. |
| `Dodge Cooldown` | `1.15` | Задержка перед следующим уклонением. |
| `Dodge Enemy Clearance` | `180` | Запас дистанции от врагов для старых/проверочных сценариев; текущий рывок стартует по input-направлению. |
| `Dodge Sound` | `Whoosh_1-1_Cue` | Звук старта рывка. |

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
| `Detection Radius` | Fast `2200`, Heavy `1800` | Радиус обнаружения игрока в общих world units проекта. Снаружи враг продолжает патруль при `Bullet: Ready`; внутри начинает преследование, и контактное убийство разрешено. |
| `Patrol Radius` | `2600` | Размер территории патруля вокруг `Patrol Center`. |
| `Patrol Center` | `(0, 0, 0)` | Центр арены для выбора дальних патрульных точек. |
| `Patrol Perimeter Radius Multiplier` | `0.78` | Насколько близко к периметру арены враг выбирает патрульные точки. |
| `Patrol Perimeter Step Degrees` | `45` | Угол следующего сектора периметра для патруля. |
| `Fast Surround Radius` | `320` | Радиус окружения Fast-врага, когда пули нет. |
| `Heavy Surround Radius` | `420` | Радиус окружения Heavy-врага, когда пули нет. |

Логика состояний:

1. `Bullet: Ready`: `Cautious`, распределение `20% Chaser / 80% Flanker`, осторожное окружение с сохранением дистанции.
2. Выстрел: каждому врагу назначается случайная задержка `0.0-0.35s`.
3. После задержки: `Rush`, распределение `70% Chaser / 10% Flanker / 20% Bullet Blocker`.
4. Подбор пули: pending Rush отменяется, все живые враги возвращаются в `Cautious`.

Старые поля секции `Pressure` оставлены для совместимости существующих Blueprint assets, но основное переключение поведения и скорости теперь задаётся секциями `AI State`.

| Событие | Данные | Применение |
| --- | --- | --- |
| `OnEnemyDeath` | `Drop Location` | death effect, звук, дополнительная реакция |
| `OnEnemyKicked` | `Direction`, `Type` | hit reaction / эффект пинка |
| `OnEnemySpawned` | `Type`, `Location` | spawn VFX |
| `OnEnemyDisappearing` | `Type`, `Location` | dissolve / disappear VFX |
| `OnEnemyAIStateChanged` | `New State`, `New Role` | VFX/UI/debug feedback при переключении Cautious/Rush |

Spawn/disappear VFX собираются нодами в каждом BP врага, поэтому их легко заменить без изменения C++.

### `BP_OBBulletPickup`

| Поле | Назначение |
| --- | --- |
| `Pickup Sound` | звук успешного возврата пули |
| `Pickup Effect` | Niagara при подборе |
| `Landing Metal Sound` / `Landing Mystic Sound` | металлический удар и короткий мистический отклик после падения |
| `Landing Sound Volume` / `Landing Mystic Echo Delay` | громкость и задержка второго слоя звука |
| `Use Sacred Drop Trail` | использует отдельный световой след падения |
| `Sacred Drop Trail System` / `Trail Niagara System` | основной и fallback Niagara trail |
| `Trail Flight Duration` / `Trail Travel Speed` / `Trail Max Flight Duration` | читаемая скорость и длительность полёта |
| `Trail Tail Duration` | сколько остаётся видимым хвост после приземления |
| `Main Mesh Scale` | размер основной пули в мире |
| `Traveling Bullet Scale` | размер представления пули в полёте / fallback |
| `Traveling Bullet Light Color` / `Intensity` | свет летящей пули |
| `Pickup Radius` / `Magnet Radius` / `Magnet Speed` | forgiving pickup |
| `Use Native Presentation Animation` | встроенные bob/spin/pulse |

#### Читаемость Пули

Пуля использует локальное тёплое свечение и вертикальный луч. Ground Halo удалён: дополнительного светового круга на полу нет.

Настройки glow находятся в `OneBulletSettings|Readability`:

| Поле | Назначение |
| --- | --- |
| `Glow Material` | материал мягкой ауры вокруг пули |
| `Sacred Light Color` | цвет локального света и glow |
| `Glow Aura Scale` | размер светящейся ауры |

Настройки локального света находятся в `OneBulletSettings|Presentation`:

| Поле | Назначение |
| --- | --- |
| `Beacon Intensity` / `Beacon Pulse Amount` | базовая яркость и амплитуда пульсации локального света |
| `Beacon Attenuation Radius` | радиус освещения |

Настройки столба находятся в `OneBulletSettings|Readability|Beam`:

| Поле | Назначение |
| --- | --- |
| `Material` | материал вертикального луча |
| `Color` | цвет луча; можно сделать таким же, как glow |
| `Intensity` | HDR-множитель emissive-цвета |
| `Height` | высота столба |
| `Thickness` | базовая толщина столба |
| `Pulse Amount (Brightness + Thickness)` | амплитуда одновременной пульсации яркости и реальной толщины |
| `Pulse Speed` | скорость пульсации |

Пульсация геометрии не зависит от того, поддерживает ли выбранный материал параметры `Color`, `Tint` или `Emissive Color`, поэтому изменение толщины остаётся заметным даже с простым emissive-материалом.

Важно: при включённом `Use Native Presentation Animation` масштаб меша пули обновляется из `Main Mesh Scale` во время игры. Поэтому увеличить новую маленькую модель нужно именно этим полем в BP, а не `Transform / Scale` компонента. Удобная стартовая проба для маленького mesh: `Main Mesh Scale = 1.5`, `Traveling Bullet Scale = 0.3`, затем подогнать глазами.

### `WBP_OBHUD`

`WBP_OBHUD` отвечает за шрифты, контейнеры, цвета, расположение, видимость и UI-анимации. C++ передаёт данные и вызывает события.

Для Wave UI в виджете должны существовать Text Block с точными именами:

- `WaveTxt`;
- `EnemiesLeftTxt`.

Во время активной волны UI показывает `Wave N` и `Enemies Left: N`. Счётчик включает ещё не заспавненных и уже живых врагов, поэтому уменьшается в реальном времени только после фактического устранения угрозы. После завершения выводится `Wave Cleared`, затем `Next Wave in 3...2...1`. Текст плавно появляется/исчезает и слегка масштабируется.

Настройки анимации в `OneBulletSettings|HUD|Waves|Animation`:

| Поле | Назначение |
| --- | --- |
| `Wave Text Animation Speed` | скорость fade/scale переходов |
| `Wave Text Hidden Scale` | масштаб текста в скрытом состоянии |
| `Wave Cleared Display Duration` | сколько показывать `Wave Cleared` до countdown |

HUD самостоятельно находит `AOBWaveManager`, подписывается на его события и читает runtime-данные. Blueprint не должен хранить отдельный счётчик волн или врагов.

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
3. Проверить drop trail, металлический звук падения, glow и вертикальный пульсирующий луч без Ground Halo.
4. Подобрать пулю: дробовик должен вернуться, HUD - переключиться в `Bullet: Ready`.
5. Проверить `WaveTxt`, `EnemiesLeftTxt`, `Wave Cleared` и countdown между волнами.
6. Убедиться, что враги не появляются ближе минимальной дистанции, получают warning и не атакуют во время grace period.
7. Проверить в логах, что каждый враг создан `WaveManager` только один раз, а волна завершается при нулевом счётчике.
8. Проверить смерть, `R`, `Shift`, `1` и `2` в Play mode.

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
