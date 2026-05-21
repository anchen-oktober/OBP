# One Bullet Left

Прототип шутера на Unreal Engine 5.7 с главным циклом:

`выстрел -> пуля потеряна -> давление врагов -> добежать до пули -> снова выстрел`

У игрока всегда только одна пуля. После выстрела она физически появляется в мире как pickup, и пока игрок не подберет ее через overlap, стрелять снова нельзя.

## Управление

- `WASD` - движение
- `Mouse` - обзор
- `Space` - прыжок
- `Left Mouse Button` - выстрел
- `Right Mouse Button` или `F` - kick / push
- `Left Shift` - уклонение
- `R` - мягкий рестарт после Game Over без перезагрузки уровня

## Где что лежит

Основные Blueprint-ассеты находятся в `/Game/OneBullet/Blueprints`:

- `BP_OBGameMode` - правила прототипа, спавн врагов, классы врагов, настройки спавна и bullet pickup.
- `BP_OBCharacter` - игрок, стрельба, пинок, уклонение, смерть, звуки и BP-события для эффектов.
- `BP_OBEnemy_Fast` - быстрый враг.
- `BP_OBEnemy_Heavy` - тяжелый враг.
- `BP_OBBulletPickup` - пуля в мире после выстрела.
- `BP_OBHUD` - HUD actor, который создает `WBP_OBHUD`.
- `WBP_OBHUD` - весь визуальный UI.
- `BP_OBGameState` - состояние пули, счетчик убийств, Game Over.

Уровень: `/Game/FirstPerson/Lvl_FirstPerson`.

## Что сделано в C++

C++ держит базовую игровую логику, чтобы прототип не разваливался от изменений в BP:

- состояние одной пули `Ready / Lost`;
- line trace выстрела;
- убийство врага с одного попадания;
- создание bullet pickup через единое место в `AOBGameMode::SpawnBulletPickup`;
- подбор пули через overlap;
- движение врагов к игроку;
- touch-kill врагов;
- запрет убийства с явной спины игрока;
- короткие scripted waves для pacing прототипа;
- kick / push;
- уклонение в свободную сторону;
- dodge cooldown state для HUD/BP;
- Game Over и restart;
- причина смерти, время выживания и best kills;
- мягкий restart run по `R`: удаляет врагов и pickup, возвращает игрока на PlayerStart, сбрасывает волны и GameState без `OpenLevel`;
- обновление базового HUD-состояния.

При этом почти все числа и реакции вынесены в Blueprint-friendly поля и события.

## Что настраивается в BP

### `BP_OBGameMode`

Отвечает за общий flow прототипа.

Важные настройки:

- `Build Greybox Arena` - строить простую тестовую арену из кода.
- `Spawn Interval` - частота спавна врагов.
- `Max Live Enemies` - максимум живых врагов.
- `Use Scripted Waves` - использовать короткую структуру волн вместо бесконечного равномерного спавна.
- `Wave Definitions` - список волн. В каждой волне задаются Fast count, Heavy count, задержка перед волной, интервал спавна и лимит живых врагов.
- `Fast Enemy Class` / `Heavy Enemy Class` - какие BP использовать для врагов.
- `Bullet Pickup Class` - какой BP использовать для пули.
- `Spawn Enemies Only In Front Of Player` - не спавнить врагов за спиной.
- `Front Spawn Min Dot` - насколько строго точка должна быть перед игроком. Сейчас дефолт `0.35`.
- `Allow Any Spawn If No Front Point` - разрешить fallback в любую точку, если впереди нет доступной.
- `Bullet Pickup Drop Height` - насколько поднять pickup над точкой попадания. Сейчас дефолт `5`, чтобы пуля была низко.

### `BP_OBCharacter`

Отвечает за игрока.

Настройки:

- `Shoot Range` - дальность выстрела.
- `Kick Cooldown`, `Kick Range`, `Kick Radius` - параметры пинка.
- `Dodge Distance`, `Dodge Duration`, `Dodge Cooldown` - параметры уклонения.
- `Dodge Enemy Clearance` - насколько далеко от врагов должна быть безопасная точка уклонения.
- `Prefer Movement Direction Dodge` - сначала пробовать уклонение по текущему движению.
- `Hide Head For First Person` - скрывать голову у видимой модели игрока.
- `Shoot Sound`, `Dry Fire Sound`, `Kick Sound`, `Death Sound`, `Dodge Sound` - звуки.

BP-события для эффектов:

- `OnPlayerShoot(TraceStart, TraceEnd, ImpactLocation, bHitSomething, bHitEnemy)`
- `OnPlayerDryFire()`
- `OnPlayerKick(KickStart, KickEnd, HitEnemyCount)`
- `OnPlayerDodge(DodgeDirection)`
- `OnPlayerDodgeFailed(FailReason)`
- `OnPlayerDeath()`

Сюда удобно вешать звук, camera shake, Niagara, montage, hit feedback.

### `BP_OBEnemy_Fast` и `BP_OBEnemy_Heavy`

Оба наследуются от `AOBEnemy`.

Общие настройки:

- `Enemy Type` - Fast или Heavy.
- `Fast Speed` / `Heavy Speed` - скорости.
- `Touch Kill Radius` - ручной минимальный радиус убийства.
- `Touch Kill Extra Margin` - запас к реальным scaled-капсулам игрока и врага.
- `Can Touch Kill From Behind` - разрешить убийство за спиной игрока. Сейчас лучше держать `false`.
- `Touch Kill Front Min Dot` - насколько враг должен быть в зоне видимости игрока, чтобы убить.
- `Death Sound` - звук смерти.

Поведение:

- Fast быстро идет к игроку и сильно отлетает от kick.
- Heavy медленно идет к игроку, почти не отлетает, но получает stun примерно на 1 секунду.

BP-события:

- `OnEnemyDeath(DropLocation)` - смерть врага. Сюда удобно ставить звук, Niagara, dissolve, анимацию.
- `OnEnemyKicked(Direction, Type)` - реакция на kick.

### `BP_OBBulletPickup`

Отвечает за пулю, которую надо подобрать.

Настройки:

- `Pickup Sound` - звук подбора.
- `Use Native Presentation Animation` - использовать встроенное вращение/покачивание.
- `Mesh Base Height` - высота визуального меша над root. Сейчас дефолт `12`.
- `Bob Height` - амплитуда покачивания. Сейчас дефолт `4`.
- `Bob Speed` - скорость покачивания.
- `Spin Speed` - скорость вращения.

Если хочется полностью контролировать визуал в BP, можно выключить `Use Native Presentation Animation` и переопределить `Update Pickup Presentation`.

### `WBP_OBHUD`

Весь UI должен настраиваться здесь: шрифты, контейнеры, отступы, цвета, анимации, порядок слоев.

C++ автоматически обновляет эти виджеты, если они есть и названы так:

- `BulletStatusText`
- `KillCountText`
- `GameOverText`
- `RestartText`
- `DeathFade`
- `DeathStatsText`
- `DodgeStatusText`
- `DodgeCooldownBar`

Что делает код:

- обновляет текст `Bullet: Ready / Lost`;
- обновляет `Kills: N`;
- скрывает `GameOverText`, `RestartText`, `DeathFade`, пока игрок жив;
- показывает их при смерти.
- обновляет `DeathStatsText`: причина смерти, kills, time survived, best kills.
- обновляет `DodgeStatusText` и `DodgeCooldownBar`, если они есть в WBP.

Дополнительные BP-события:

- `OnBulletStateChanged(NewBulletState)`
- `OnKillCountChanged(NewKillCount)`
- `OnGameOverChanged(bNewGameOver)`
- `OnDodgeCooldownChanged(bNewDodgeReady, NewCooldownNormalized)`
- `OnHudStateRefreshed(NewBulletState, NewKillCount, bNewGameOver)`

Их можно использовать для анимаций, звуков UI и дополнительных контейнеров.

## Gameplay Loop

1. Игрок начинает с пулей.
2. Игрок стреляет.
3. Состояние пули становится `Lost`.
4. Если попал во врага, враг умирает и появляется pickup рядом с ним.
5. Если промахнулся, pickup появляется в точке попадания или в конце trace.
6. Пока pickup не подобран, стрелять нельзя.
7. Игрок должен физически добежать до пули.
8. После overlap с pickup состояние возвращается в `Ready`.

## Волны

По умолчанию включены три короткие волны:

- Wave 1: 2 Fast.
- Wave 2: 1 Fast + 1 Heavy.
- Wave 3: 3 Fast + 2 Heavy.

Это не полноценная production wave system, а простая pacing-структура, чтобы прототип за пару минут показывал нарастающее давление.

## Враги и видимость

Враги спавнятся только в зоне перед взглядом игрока, если включен `Spawn Enemies Only In Front Of Player`.

Дополнительно touch-kill не срабатывает, если враг явно оказался за спиной игрока. Это сделано специально для first person режима, чтобы игрок не умирал от угрозы, которую не мог видеть.

## Генерация и проверка BP

Скрипт:

`Scripts/CreateOneBulletBlueprints.py`

Создает и обновляет основные BP, назначает классы в `BP_OBGameMode`, выставляет GameMode Override на уровне и сохраняет ассеты.

Проверка:

`Scripts/VerifyOneBulletBlueprints.py`

Пишет результат в:

`Saved/OneBulletBPVerify.txt`

## Полезные заметки

- `BP_OBGameMode` должен быть настоящим Blueprint, не redirector. Если игра внезапно запускается как `GameModeBase`, почти наверняка сломалась ссылка на GameMode.
- Если меняется C++ с `UPROPERTY`, лучше закрыть Unreal Editor или выключить Live Coding перед полной сборкой.
- Debug trace выстрела сейчас отключен и не рисуется.
- Визуал placeholder/greybox, логика специально оставлена простой и удобной для настройки в BP.
