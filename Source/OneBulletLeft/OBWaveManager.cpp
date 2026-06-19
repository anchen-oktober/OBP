#include "OBWaveManager.h"

#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/Engine.h"
#include "Engine/PointLight.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "OBCharacter.h"
#include "OBEnemySpawnPoint.h"
#include "OBGameState.h"

DEFINE_LOG_CATEGORY_STATIC(LogOBWaveManager, Log, All);

AOBWaveManager::AOBWaveManager()
{
	PrimaryActorTick.bCanEverTick = false;
	EnemyClass = AOBEnemy::StaticClass();
	FastEnemyClass = AOBEnemy::StaticClass();
	HeavyEnemyClass = AOBEnemy::StaticClass();

	WaveDefinitions = {
		FOBWaveDefinition{2, 0, 1.0f, 0.8f, 2},
		FOBWaveDefinition{1, 1, 4.0f, 1.0f, 3},
		FOBWaveDefinition{3, 2, 4.0f, 0.9f, 5}
	};
}

void AOBWaveManager::BeginPlay()
{
	Super::BeginPlay();

	RefreshEnemySpawnPoints();

	if (bAutoStartWaves)
	{
		StartWaves();
	}
}

void AOBWaveManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopWaves();
	SpawnedEnemies.Reset();
	Super::EndPlay(EndPlayReason);
}

void AOBWaveManager::StartWaves()
{
	if (!GetWorld())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	GetWorldTimerManager().ClearTimer(StateTimerHandle);
	ClearSpawnedEnemies();
	RefreshEnemySpawnPoints();

	CurrentWaveNumber = 0;
	EnemiesRemainingToSpawn = 0;
	RefreshLivingEnemyCount();
	CurrentDifficultyMultiplier = 1.0f;
	PendingEnemySpawnTypes.Reset();
	RemainingFastEnemies = 0;
	RemainingHeavyEnemies = 0;
	SetWaveState(EOBWaveState::Waiting);

	const float WaitDuration = ResolveInitialWaitDuration();
	if (WaitDuration <= 0.0f)
	{
		BeginNextWave();
	}
	else
	{
		GetWorldTimerManager().SetTimer(StateTimerHandle, this, &AOBWaveManager::BeginNextWave, WaitDuration, false);
	}
}

void AOBWaveManager::StartWave()
{
	if (!GetWorld() || WaveState == EOBWaveState::Active)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(StateTimerHandle);
	BeginNextWave();
}

void AOBWaveManager::RestartWaves()
{
	StartWaves();
}

void AOBWaveManager::StopWave()
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
		GetWorldTimerManager().ClearTimer(StateTimerHandle);
	}

	RemainingFastEnemies = 0;
	RemainingHeavyEnemies = 0;
	PendingEnemySpawnTypes.Reset();
	EnemiesRemainingToSpawn = 0;
	SetWaveState(EOBWaveState::Waiting);
}

void AOBWaveManager::StopWaves()
{
	StopWave();
}

float AOBWaveManager::GetIntermissionTimeRemaining() const
{
	return GetWorld() ? GetWorldTimerManager().GetTimerRemaining(StateTimerHandle) : 0.0f;
}

void AOBWaveManager::SetWaveState(EOBWaveState NewState)
{
	if (WaveState == NewState)
	{
		return;
	}

	const EOBWaveState PreviousState = WaveState;
	WaveState = NewState;
	OnWaveStateChanged.Broadcast(WaveState, PreviousState);
}

void AOBWaveManager::BeginNextWave()
{
	if (!GetWorld() || IsGameOver())
	{
		return;
	}

	++CurrentWaveNumber;
	const FRuntimeWave RuntimeWave = BuildWave(CurrentWaveNumber);
	RemainingFastEnemies = RuntimeWave.FastCount;
	RemainingHeavyEnemies = RuntimeWave.HeavyCount;
	PendingEnemySpawnTypes.Reset(RemainingFastEnemies + RemainingHeavyEnemies);
	for (int32 Index = 0; Index < RemainingHeavyEnemies; ++Index)
	{
		PendingEnemySpawnTypes.Add(EOBEnemyType::Heavy);
	}
	for (int32 Index = 0; Index < RemainingFastEnemies; ++Index)
	{
		PendingEnemySpawnTypes.Add(EOBEnemyType::Fast);
	}
	EnemiesRemainingToSpawn = PendingEnemySpawnTypes.Num();
	CurrentMaxLiveEnemies = FMath::Max(RuntimeWave.MaxLiveEnemies, 1);

	const float UncappedDifficulty = FMath::Pow(
		1.0f + FMath::Max(EnemySpeedIncreasePerWave, 0.0f),
		FMath::Max(CurrentWaveNumber - 1, 0));
	CurrentDifficultyMultiplier = bScaleDifficulty
		? FMath::Min(UncappedDifficulty, FMath::Max(MaxEnemySpeedMultiplier, 1.0f))
		: 1.0f;

	SetWaveState(EOBWaveState::Active);
	OnWaveStarted.Broadcast(CurrentWaveNumber, EnemiesRemainingToSpawn, CurrentDifficultyMultiplier);

	const FString WaveSource = RuntimeWave.bFromScriptedDefinition
		? FString::Printf(
			TEXT("Scripted definition %d/%d"),
			RuntimeWave.DefinitionIndex + 1,
			WaveDefinitions.Num())
		: TEXT("Generated");
	DebugWaveMessage(
		FString::Printf(
			TEXT("Wave %d started [%s]: Fast=%d, Heavy=%d, Total=%d, Interval=%.2fs, MaxAlive=%d, Difficulty=x%.2f"),
			CurrentWaveNumber,
			*WaveSource,
			RuntimeWave.FastCount,
			RuntimeWave.HeavyCount,
			EnemiesRemainingToSpawn,
			RuntimeWave.SpawnInterval,
			CurrentMaxLiveEnemies,
			CurrentDifficultyMultiplier),
		FColor::Cyan);
	DebugWaveMessage(
		FString::Printf(
			TEXT("Wave %d spawn request: Requested NormalCount=%d, Requested HeavyCount=%d, PendingQueue=%d, NormalEnemyClass=%s, HeavyEnemyClass=%s, FallbackEnemyClass=%s"),
			CurrentWaveNumber,
			RuntimeWave.FastCount,
			RuntimeWave.HeavyCount,
			PendingEnemySpawnTypes.Num(),
			*GetNameSafe(FastEnemyClass.Get()),
			*GetNameSafe(HeavyEnemyClass.Get()),
			*GetNameSafe(EnemyClass.Get())),
		FColor::Cyan);

	SpawnEnemyTick();
	if (EnemiesRemainingToSpawn > 0)
	{
		GetWorldTimerManager().SetTimer(
			SpawnTimerHandle,
			this,
			&AOBWaveManager::SpawnEnemyTick,
			FMath::Max(RuntimeWave.SpawnInterval, MinimumSpawnInterval),
			true);
	}

	CheckWaveCompletion();
}

void AOBWaveManager::SpawnEnemyTick()
{
	if (WaveState != EOBWaveState::Active || IsGameOver())
	{
		return;
	}

	RefreshLivingEnemyCount();
	if (LivingEnemyCount >= CurrentMaxLiveEnemies)
	{
		return;
	}

	if (PendingEnemySpawnTypes.Num() <= 0)
	{
		EnemiesRemainingToSpawn = 0;
		GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
		return;
	}

	const EOBEnemyType TypeToSpawn = PendingEnemySpawnTypes[0];
	DebugWaveMessage(
		FString::Printf(
			TEXT("Spawn tick: next enemy type=%s, remaining normal=%d, remaining heavy=%d, pending=%d"),
			TypeToSpawn == EOBEnemyType::Heavy ? TEXT("Heavy") : TEXT("Normal"),
			RemainingFastEnemies,
			RemainingHeavyEnemies,
			PendingEnemySpawnTypes.Num()),
		FColor::Silver);

	if (!SpawnEnemy(TypeToSpawn))
	{
		DebugWaveMessage(
			FString::Printf(
				TEXT("Spawn tick failed: type=%s remains queued; will retry on next tick."),
				TypeToSpawn == EOBEnemyType::Heavy ? TEXT("Heavy") : TEXT("Normal")),
			FColor::Red);
		return;
	}

	PendingEnemySpawnTypes.RemoveAt(0);
	if (TypeToSpawn == EOBEnemyType::Heavy)
	{
		--RemainingHeavyEnemies;
	}
	else
	{
		--RemainingFastEnemies;
	}

	EnemiesRemainingToSpawn = PendingEnemySpawnTypes.Num();
	if (EnemiesRemainingToSpawn <= 0)
	{
		GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	}
	CheckWaveCompletion();
}

void AOBWaveManager::CheckWaveCompletion()
{
	if (WaveState != EOBWaveState::Active || IsGameOver())
	{
		return;
	}

	if (!bAutoCompleteWhenAllEnemiesDefeated)
	{
		return;
	}

	RefreshLivingEnemyCount();
	if (EnemiesRemainingToSpawn > 0 || LivingEnemyCount > 0)
	{
		return;
	}

	CompleteCurrentWave();
}

void AOBWaveManager::CompleteCurrentWave()
{
	if (!GetWorld() || WaveState != EOBWaveState::Active)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	RemainingFastEnemies = 0;
	RemainingHeavyEnemies = 0;
	PendingEnemySpawnTypes.Reset();
	EnemiesRemainingToSpawn = 0;
	SetWaveState(EOBWaveState::Completed);
	OnWaveCompleted.Broadcast(CurrentWaveNumber);

	DebugWaveMessage(
		FString::Printf(TEXT("Wave %d completed; living enemies: %d"), CurrentWaveNumber, LivingEnemyCount),
		FColor::Green);

	const float CompletedDuration = FMath::Max(CompletedStateDuration, 0.0f);
	if (CompletedDuration <= 0.0f)
	{
		EnterIntermission();
	}
	else
	{
		GetWorldTimerManager().SetTimer(StateTimerHandle, this, &AOBWaveManager::EnterIntermission, CompletedDuration, false);
	}
}

void AOBWaveManager::EnterIntermission()
{
	if (!GetWorld() || IsGameOver())
	{
		return;
	}

	SetWaveState(EOBWaveState::Intermission);
	const float Duration = ResolveIntermissionDuration();
	GetWorldTimerManager().SetTimer(StateTimerHandle, this, &AOBWaveManager::BeginNextWave, Duration, false);
}

AOBEnemy* AOBWaveManager::SpawnEnemy(EOBEnemyType Type)
{
	TSubclassOf<AOBEnemy> ClassToSpawn = nullptr;
	if (Type == EOBEnemyType::Heavy)
	{
		ClassToSpawn = HeavyEnemyClass;
		if (!ClassToSpawn)
		{
			UE_LOG(
				LogOBWaveManager,
				Error,
				TEXT("Spawn failed: Enemy class is None for Heavy. HeavyEnemyClass must be assigned; Heavy will not be replaced with Normal."));
			return nullptr;
		}
	}
	else
	{
		ClassToSpawn = FastEnemyClass ? FastEnemyClass : EnemyClass;
	}
	if (!ClassToSpawn)
	{
		UE_LOG(
			LogOBWaveManager,
			Error,
			TEXT("Spawn failed: Enemy class is None for %s. FastEnemyClass or EnemyClass must be assigned."),
			Type == EOBEnemyType::Heavy ? TEXT("Heavy") : TEXT("Normal"));
		return nullptr;
	}
	DebugWaveMessage(
		FString::Printf(
			TEXT("Enemy class selected: type=%s, class=%s"),
			Type == EOBEnemyType::Heavy ? TEXT("Heavy") : TEXT("Normal"),
			*GetNameSafe(ClassToSpawn.Get())),
		Type == EOBEnemyType::Heavy ? FColor::Orange : FColor::Silver);

	FEnemySpawnCandidate SpawnCandidate;
	if (!TryChooseSpawnPoint(Type, SpawnCandidate))
	{
		UE_LOG(
			LogOBWaveManager,
			Error,
			TEXT("Spawn result: Failed. WaveManager could not find a valid spawn point for %s class=%s."),
			Type == EOBEnemyType::Heavy ? TEXT("Heavy") : TEXT("Normal"),
			*GetNameSafe(ClassToSpawn.Get()));
		return nullptr;
	}
	check(SpawnCandidate.SpawnPoint);

	UE_LOG(
		LogOBWaveManager,
		Log,
		TEXT("SpawnActor called: type=%s, class=%s, SpawnPoint=%s, marker=%s, safe=%s"),
		Type == EOBEnemyType::Heavy ? TEXT("Heavy") : TEXT("Normal"),
		*GetNameSafe(ClassToSpawn.Get()),
		*GetNameSafe(SpawnCandidate.SpawnPoint),
		*SpawnCandidate.SpawnPoint->GetActorLocation().ToCompactString(),
		*SpawnCandidate.SpawnLocation.ToCompactString());

	const FTransform SpawnTransform(FRotator::ZeroRotator, SpawnCandidate.SpawnLocation);
	AOBEnemy* Enemy = GetWorld()->SpawnActorDeferred<AOBEnemy>(
		ClassToSpawn,
		SpawnTransform,
		this,
		nullptr,
		ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding);
	if (!Enemy)
	{
		UE_LOG(
			LogOBWaveManager,
			Error,
			TEXT("Spawn result: Failed. Enemy class %s at %s was blocked by SpawnActor collision handling."),
			*GetNameSafe(ClassToSpawn),
			*SpawnCandidate.SpawnLocation.ToCompactString());
		return nullptr;
	}

	Enemy->Configure(Type);
	Enemy->SetDifficultySpeedMultiplier(CurrentDifficultyMultiplier);
	Enemy->OnDeathReported.AddUniqueDynamic(this, &AOBWaveManager::HandleEnemyDeathReported);
	Enemy->OnDestroyed.AddUniqueDynamic(this, &AOBWaveManager::HandleEnemyDestroyed);
	SpawnedEnemies.Add(TWeakObjectPtr<AOBEnemy>(Enemy));
	RefreshLivingEnemyCount();

	AOBEnemy* FinishedEnemy = Cast<AOBEnemy>(UGameplayStatics::FinishSpawningActor(Enemy, SpawnTransform));
	if (!FinishedEnemy)
	{
		SpawnedEnemies.Remove(TWeakObjectPtr<AOBEnemy>(Enemy));
		RefreshLivingEnemyCount();
		UE_LOG(
			LogOBWaveManager,
			Error,
			TEXT("Spawn result: Failed. FinishSpawningActor returned null for class %s at %s."),
			*GetNameSafe(ClassToSpawn),
			*SpawnCandidate.SpawnLocation.ToCompactString());
		return nullptr;
	}

	if (!FinishedEnemy->GetController())
	{
		FinishedEnemy->SpawnDefaultController();
	}

	bool bSpawnedOnNavMesh = false;
	if (UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		FNavLocation ProjectedSpawnLocation;
		bSpawnedOnNavMesh = NavSystem->ProjectPointToNavigation(
			FinishedEnemy->GetActorLocation(),
			ProjectedSpawnLocation,
			FVector(320.0f, 320.0f, 500.0f));
	}

	const UCapsuleComponent* SpawnedCapsule = FinishedEnemy->GetCapsuleComponent();
	const UCharacterMovementComponent* SpawnedMovement = FinishedEnemy->GetCharacterMovement();
	DebugWaveMessage(
		FString::Printf(
			TEXT("Post-spawn check: type=%s, actor=%s, class=%s, controller=%s, capsule=%s radius=%.0f halfHeight=%.0f, movement=%s mode=%d, hidden=%s, onNavMesh=%s"),
			Type == EOBEnemyType::Heavy ? TEXT("Heavy") : TEXT("Normal"),
			*GetNameSafe(FinishedEnemy),
			*GetNameSafe(FinishedEnemy->GetClass()),
			*GetNameSafe(FinishedEnemy->GetController()),
			SpawnedCapsule ? TEXT("valid") : TEXT("missing"),
			SpawnedCapsule ? SpawnedCapsule->GetScaledCapsuleRadius() : 0.0f,
			SpawnedCapsule ? SpawnedCapsule->GetScaledCapsuleHalfHeight() : 0.0f,
			SpawnedMovement ? TEXT("valid") : TEXT("missing"),
			SpawnedMovement ? static_cast<int32>(SpawnedMovement->MovementMode) : INDEX_NONE,
			FinishedEnemy->IsHidden() ? TEXT("yes") : TEXT("no"),
			bSpawnedOnNavMesh ? TEXT("yes") : TEXT("no")),
		Type == EOBEnemyType::Heavy ? FColor::Orange : FColor::Silver);

	const float SafeWarningDuration = FMath::Clamp(SpawnWarningDuration, 0.3f, 0.7f);
	const float SafeGracePeriod = FMath::Clamp(SpawnGracePeriod, 0.3f, 0.5f);
	Enemy->BeginSpawnProtection(SafeWarningDuration, SafeGracePeriod);
	ShowSpawnWarning(Type, SpawnCandidate.SpawnLocation);
	OnEnemySpawned.Broadcast(Enemy, Type, SpawnCandidate.SpawnLocation, LivingEnemyCount);
	DebugWaveMessage(
		FString::Printf(
			TEXT("Spawn result: Success. Wave %d spawned %s class=%s at %s; appears in %.2fs, protected for %.2fs; living: %d"),
			CurrentWaveNumber,
			Type == EOBEnemyType::Heavy ? TEXT("Heavy") : TEXT("Normal"),
			*GetNameSafe(ClassToSpawn.Get()),
			*SpawnCandidate.SpawnLocation.ToCompactString(),
			SafeWarningDuration,
			SafeGracePeriod,
			LivingEnemyCount),
		FColor::Yellow);
	return Enemy;
}

void AOBWaveManager::RefreshEnemySpawnPoints()
{
	EnemySpawnPoints.Reset();

	if (!GetWorld())
	{
		return;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AOBEnemySpawnPoint::StaticClass(), FoundActors);
	for (AActor* Actor : FoundActors)
	{
		if (AOBEnemySpawnPoint* SpawnPoint = Cast<AOBEnemySpawnPoint>(Actor))
		{
			EnemySpawnPoints.Add(SpawnPoint);
		}
	}

	UE_LOG(LogOBWaveManager, Log, TEXT("Total spawn points found: %d"), EnemySpawnPoints.Num());
	if (EnemySpawnPoints.Num() == 0)
	{
		UE_LOG(LogOBWaveManager, Warning, TEXT("No Enemy Spawn Points found on level"));
	}
}

bool AOBWaveManager::TryChooseSpawnPoint(EOBEnemyType Type, FEnemySpawnCandidate& OutCandidate)
{
	OutCandidate = FEnemySpawnCandidate();
	RefreshEnemySpawnPoints();

	if (EnemySpawnPoints.Num() == 0)
	{
		DebugWaveMessage(TEXT("Spawn failed: no spawn points."), FColor::Red);
		return false;
	}

	const AOBCharacter* Player = Cast<AOBCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	const FVector PlayerLocation = Player ? Player->GetActorLocation() : FVector::ZeroVector;
	const bool bHasPlayer = Player != nullptr;
	const float MinimumDistance = FMath::Max(MinimumSpawnDistanceFromPlayer, 0.0f);
	TArray<AOBEnemySpawnPoint*> CandidatePoints;
	AOBEnemySpawnPoint* FarthestTooClosePoint = nullptr;
	float FarthestTooCloseDistance = -1.0f;
	int32 InvalidPointCount = 0;
	int32 TooClosePointCount = 0;

	DebugWaveMessage(
		FString::Printf(
			TEXT("Spawn point search: Total spawn points found=%d, PlayerLocation=%s, MinSpawnDistanceFromPlayer=%.0f, MaxSpawnAttempts=%d"),
			EnemySpawnPoints.Num(),
			bHasPlayer ? *PlayerLocation.ToCompactString() : TEXT("none"),
			MinimumDistance,
			FMath::Max(MaxSpawnAttempts, 1)),
		FColor::Silver);

	for (AOBEnemySpawnPoint* SpawnPoint : EnemySpawnPoints)
	{
		if (!IsValid(SpawnPoint))
		{
			++InvalidPointCount;
			continue;
		}

		const FVector MarkerLocation = SpawnPoint->GetActorLocation();
		const float DistanceToPlayer = bHasPlayer ? FVector::Dist2D(MarkerLocation, PlayerLocation) : MinimumDistance;
		if (bHasPlayer && DistanceToPlayer < MinimumDistance)
		{
			++TooClosePointCount;
			if (DistanceToPlayer > FarthestTooCloseDistance)
			{
				FarthestTooCloseDistance = DistanceToPlayer;
				FarthestTooClosePoint = SpawnPoint;
			}
			continue;
		}

		CandidatePoints.Add(SpawnPoint);
	}

	DebugWaveMessage(
		FString::Printf(
			TEXT("Spawn point filter: valid candidates=%d, filtered too close=%d, invalid refs=%d"),
			CandidatePoints.Num(),
			TooClosePointCount,
			InvalidPointCount),
		TooClosePointCount > 0 ? FColor::Orange : FColor::Silver);

	bool bAllowTooClose = false;
	if (CandidatePoints.Num() == 0 && FarthestTooClosePoint)
	{
		DebugWaveMessage(
			FString::Printf(
				TEXT("Spawn warning: all %d spawn points are closer than %.0f units to the player; trying farthest point %s at %.0f."),
				EnemySpawnPoints.Num(),
				MinimumDistance,
				*GetNameSafe(FarthestTooClosePoint),
				FarthestTooCloseDistance),
			FColor::Orange);
		CandidatePoints.Add(FarthestTooClosePoint);
		bAllowTooClose = true;
	}

	if (CandidatePoints.Num() == 0)
	{
		DebugWaveMessage(TEXT("Spawn failed: no usable spawn points after distance filtering."), FColor::Red);
		return false;
	}

	const int32 AttemptLimit = FMath::Min(FMath::Max(MaxSpawnAttempts, 1), CandidatePoints.Num());
	FString LastFailureReason;
	for (int32 AttemptIndex = 0; AttemptIndex < AttemptLimit && CandidatePoints.Num() > 0; ++AttemptIndex)
	{
		const int32 RandomIndex = FMath::RandRange(0, CandidatePoints.Num() - 1);
		AOBEnemySpawnPoint* SpawnPoint = CandidatePoints[RandomIndex];
		CandidatePoints.RemoveAtSwap(RandomIndex, 1, EAllowShrinking::No);

		FString FailureReason;
		if (TryResolveSpawnPointCandidate(Type, SpawnPoint, PlayerLocation, bHasPlayer, bAllowTooClose, OutCandidate, FailureReason))
		{
			DebugWaveMessage(
				FString::Printf(
					TEXT("Selected spawn point: attempt=%d/%d, name=%s, distanceToPlayer=%.0f, location=%s"),
					AttemptIndex + 1,
					AttemptLimit,
					*GetNameSafe(OutCandidate.SpawnPoint),
					OutCandidate.DistanceToPlayer,
					*OutCandidate.SpawnLocation.ToCompactString()),
				FColor::Green);
			return true;
		}

		LastFailureReason = FailureReason;
		DebugWaveMessage(
			FString::Printf(
				TEXT("Spawn attempt failed: attempt=%d/%d, point=%s, reason=%s"),
				AttemptIndex + 1,
				AttemptLimit,
				*GetNameSafe(SpawnPoint),
				*FailureReason),
			FColor::Red);
	}

	DebugWaveMessage(
		FString::Printf(
			TEXT("Spawn failed: no random spawn point passed validation after %d attempts. Last failure: %s"),
			AttemptLimit,
			LastFailureReason.IsEmpty() ? TEXT("unknown") : *LastFailureReason),
		FColor::Red);
	return false;
}

bool AOBWaveManager::TryResolveSpawnPointCandidate(
	EOBEnemyType Type,
	AOBEnemySpawnPoint* SpawnPoint,
	const FVector& PlayerLocation,
	bool bHasPlayer,
	bool bAllowTooClose,
	FEnemySpawnCandidate& OutCandidate,
	FString& OutFailureReason) const
{
	OutCandidate = FEnemySpawnCandidate();
	OutFailureReason = TEXT("Unknown spawn point failure");

	if (!IsValid(SpawnPoint))
	{
		OutFailureReason = TEXT("invalid spawn point");
		return false;
	}

	const FVector MarkerLocation = SpawnPoint->GetActorLocation();
	FVector SafeSpawnLocation = FVector::ZeroVector;
	if (!TryResolveSafeSpawnLocation(Type, MarkerLocation, SafeSpawnLocation, OutFailureReason))
	{
		return false;
	}

	const float DistanceToPlayer = bHasPlayer ? FVector::Dist2D(SafeSpawnLocation, PlayerLocation) : 0.0f;
	if (bHasPlayer && !bAllowTooClose && DistanceToPlayer < FMath::Max(MinimumSpawnDistanceFromPlayer, 0.0f))
	{
		OutFailureReason = FString::Printf(
			TEXT("too close to player after NavMesh projection: %.0f < %.0f"),
			DistanceToPlayer,
			FMath::Max(MinimumSpawnDistanceFromPlayer, 0.0f));
		return false;
	}

	OutCandidate.SpawnPoint = SpawnPoint;
	OutCandidate.SpawnLocation = SafeSpawnLocation;
	OutCandidate.DistanceToPlayer = DistanceToPlayer;
	return true;
}

bool AOBWaveManager::TryResolveSafeSpawnLocation(
	EOBEnemyType Type,
	const FVector& MarkerLocation,
	FVector& OutSpawnLocation,
	FString& OutFailureReason) const
{
	OutSpawnLocation = FVector::ZeroVector;
	OutFailureReason = TEXT("Unknown failure");

	float CapsuleRadius = 0.0f;
	float CapsuleHalfHeight = 0.0f;
	GetSpawnCapsule(Type, CapsuleRadius, CapsuleHalfHeight);

	const float SearchRadius = FMath::Max(SpawnSafeSearchRadius, 0.0f);
	const float Step = FMath::Max(CapsuleRadius * 1.5f, 80.0f);
	const int32 RingCount = SearchRadius > 0.0f ? FMath::CeilToInt(SearchRadius / Step) : 0;
	FString LastFailureReason;

	for (int32 RingIndex = 0; RingIndex <= RingCount; ++RingIndex)
	{
		const float Radius = RingIndex == 0 ? 0.0f : FMath::Min(Step * RingIndex, SearchRadius);
		const int32 DirectionCount = RingIndex == 0 ? 1 : 8;
		for (int32 DirectionIndex = 0; DirectionIndex < DirectionCount; ++DirectionIndex)
		{
			const float Angle = DirectionCount == 1 ? 0.0f : (2.0f * PI * DirectionIndex) / DirectionCount;
			const FVector Offset(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0f);
			const FVector TestLocation = MarkerLocation + Offset;

			UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
			if (!NavSystem)
			{
				OutFailureReason = TEXT("NavSystem unavailable");
				return false;
			}

			FNavLocation ProjectedLocation;
			const bool bProjectedToNav = NavSystem->ProjectPointToNavigation(
				TestLocation,
				ProjectedLocation,
				FVector(SearchRadius + CapsuleRadius, SearchRadius + CapsuleRadius, CapsuleHalfHeight * 2.0f));
			if (!bProjectedToNav)
			{
				LastFailureReason = TEXT("not on NavMesh");
				continue;
			}

			if (FVector::Dist2D(ProjectedLocation.Location, MarkerLocation) > SearchRadius + KINDA_SMALL_NUMBER)
			{
				LastFailureReason = TEXT("nearest NavMesh is too far from marker");
				continue;
			}

			FString SafetyFailureReason;
			if (IsSpawnLocationSafe(Type, ProjectedLocation.Location, OutSpawnLocation, SafetyFailureReason))
			{
				DebugWaveMessage(
					FString::Printf(
						TEXT("Spawn point passed NavMesh/collision: marker=%s, nav=%s, spawn=%s"),
						*MarkerLocation.ToCompactString(),
						*ProjectedLocation.Location.ToCompactString(),
						*OutSpawnLocation.ToCompactString()),
					FColor::Green);
				return true;
			}

			LastFailureReason = SafetyFailureReason;
		}
	}

	OutFailureReason = LastFailureReason.IsEmpty() ? TEXT("no safe nearby position") : LastFailureReason;
	return false;
}

bool AOBWaveManager::IsSpawnLocationSafe(
	EOBEnemyType Type,
	const FVector& NavLocation,
	FVector& OutSpawnLocation,
	FString& OutFailureReason) const
{
	OutSpawnLocation = FVector::ZeroVector;
	OutFailureReason = TEXT("Unknown safety failure");

	float CapsuleRadius = 0.0f;
	float CapsuleHalfHeight = 0.0f;
	GetSpawnCapsule(Type, CapsuleRadius, CapsuleHalfHeight);
	const float PaddedRadius = CapsuleRadius + FMath::Max(SpawnCollisionPadding, 0.0f);
	const FCollisionShape SpawnShape = FCollisionShape::MakeCapsule(PaddedRadius, CapsuleHalfHeight);
	const FVector SpawnLocation = NavLocation + FVector::UpVector * CapsuleHalfHeight;

	FCollisionQueryParams CollisionQuery(SCENE_QUERY_STAT(EnemySpawnCollision), false, this);
	CollisionQuery.AddIgnoredActor(this);
	const bool bBlocked = GetWorld()->OverlapBlockingTestByChannel(
		SpawnLocation,
		FQuat::Identity,
		ECC_Pawn,
		SpawnShape,
		CollisionQuery);
	if (bBlocked)
	{
		OutFailureReason = TEXT("collision capsule overlaps geometry or actor");
		return false;
	}

	if (!CanReachPlayerFromSpawn(NavLocation))
	{
		OutFailureReason = TEXT("no NavMesh path from spawn to player");
		return false;
	}

	OutSpawnLocation = SpawnLocation;
	return true;
}

bool AOBWaveManager::CanReachPlayerFromSpawn(const FVector& SpawnLocation) const
{
	const AOBCharacter* Player = Cast<AOBCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!Player)
	{
		return true;
	}

	UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSystem)
	{
		return false;
	}

	FNavLocation PlayerNavLocation;
	if (!NavSystem->ProjectPointToNavigation(Player->GetActorLocation(), PlayerNavLocation, FVector(320.0f, 320.0f, 500.0f)))
	{
		return true;
	}

	FVector::FReal PathLength = 0.0;
	return NavSystem->GetPathLength(SpawnLocation, PlayerNavLocation.Location, PathLength) == ENavigationQueryResult::Success;
}

void AOBWaveManager::GetSpawnCapsule(EOBEnemyType Type, float& OutRadius, float& OutHalfHeight) const
{
	if (Type == EOBEnemyType::Heavy)
	{
		OutRadius = 58.0f;
		OutHalfHeight = 110.0f;
		return;
	}

	OutRadius = 38.0f;
	OutHalfHeight = 92.0f;
}

void AOBWaveManager::ShowSpawnWarning(EOBEnemyType Type, const FVector& SpawnLocation)
{
	if (!GetWorld())
	{
		return;
	}

	const float SafeWarningDuration = FMath::Clamp(SpawnWarningDuration, 0.3f, 0.7f);
	const FVector WarningLocation = SpawnLocation + FVector::UpVector * 70.0f;
	OnSpawnWarning.Broadcast(Type, SpawnLocation, SafeWarningDuration);

	if (SpawnWarningEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			SpawnWarningEffect,
			WarningLocation);
	}

	if (bUseSpawnWarningLight)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (APointLight* WarningLight = GetWorld()->SpawnActor<APointLight>(
			APointLight::StaticClass(),
			WarningLocation,
			FRotator::ZeroRotator,
			SpawnParameters))
		{
			if (UPointLightComponent* LightComponent = Cast<UPointLightComponent>(WarningLight->GetLightComponent()))
			{
				LightComponent->SetIntensity(FMath::Max(SpawnWarningLightIntensity, 0.0f));
				LightComponent->SetAttenuationRadius(FMath::Max(SpawnWarningLightRadius, 0.0f));
				LightComponent->SetLightColor(SpawnWarningLightColor);
				LightComponent->SetCastShadows(false);
			}
			WarningLight->SetLifeSpan(SafeWarningDuration);
		}
	}
}

void AOBWaveManager::ClearSpawnedEnemies()
{
	TArray<TWeakObjectPtr<AOBEnemy>> EnemiesToClear = SpawnedEnemies.Array();
	SpawnedEnemies.Reset();

	for (const TWeakObjectPtr<AOBEnemy>& EnemyPtr : EnemiesToClear)
	{
		if (AOBEnemy* Enemy = EnemyPtr.Get())
		{
			Enemy->OnDeathReported.RemoveDynamic(this, &AOBWaveManager::HandleEnemyDeathReported);
			Enemy->OnDestroyed.RemoveDynamic(this, &AOBWaveManager::HandleEnemyDestroyed);
			Enemy->Disappear();
		}
	}

	RefreshLivingEnemyCount();
}

void AOBWaveManager::RefreshLivingEnemyCount()
{
	int32 NewLivingEnemyCount = 0;
	for (auto It = SpawnedEnemies.CreateIterator(); It; ++It)
	{
		AOBEnemy* Enemy = It->Get();
		if (!IsValid(Enemy))
		{
			It.RemoveCurrent();
			continue;
		}
		if (!Enemy->IsDead())
		{
			++NewLivingEnemyCount;
		}
	}

	if (LivingEnemyCount != NewLivingEnemyCount)
	{
		LivingEnemyCount = NewLivingEnemyCount;
		OnEnemyCountChanged.Broadcast(LivingEnemyCount);
	}
}

void AOBWaveManager::HandleEnemyDeathReported(AOBEnemy* Enemy)
{
	if (!IsValid(Enemy) || !SpawnedEnemies.Contains(TWeakObjectPtr<AOBEnemy>(Enemy)))
	{
		return;
	}

	RefreshLivingEnemyCount();
	OnEnemyDied.Broadcast(Enemy, LivingEnemyCount);
	DebugWaveMessage(
		FString::Printf(
			TEXT("Wave %d enemy died: %s; living: %d; remaining to spawn: %d"),
			CurrentWaveNumber,
			*GetNameSafe(Enemy),
			LivingEnemyCount,
			EnemiesRemainingToSpawn),
		FColor::Orange);
	CheckWaveCompletion();
}

void AOBWaveManager::HandleEnemyDestroyed(AActor* DestroyedActor)
{
	AOBEnemy* Enemy = Cast<AOBEnemy>(DestroyedActor);
	if (!Enemy)
	{
		return;
	}

	const int32 RemovedCount = SpawnedEnemies.Remove(TWeakObjectPtr<AOBEnemy>(Enemy));
	if (RemovedCount <= 0)
	{
		return;
	}

	RefreshLivingEnemyCount();
	CheckWaveCompletion();
}

void AOBWaveManager::DebugWaveMessage(const FString& Message, const FColor& Color) const
{
	if (bEnableWaveDebugLogs)
	{
		UE_LOG(LogOBWaveManager, Log, TEXT("%s"), *Message);
	}
	if (bEnableWaveDebugScreenMessages && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, DebugScreenMessageDuration, Color, Message);
	}
}

AOBWaveManager::FRuntimeWave AOBWaveManager::BuildWave(int32 WaveNumber) const
{
	FRuntimeWave Result;
	const int32 SafeWaveNumber = FMath::Max(WaveNumber, 1);

	if (WaveDefinitions.Num() > 0)
	{
		const int32 DefinitionIndex = FMath::Min(SafeWaveNumber - 1, WaveDefinitions.Num() - 1);
		const FOBWaveDefinition& Definition = WaveDefinitions[DefinitionIndex];
		Result.FastCount = FMath::Max(Definition.FastCount, 0);
		Result.HeavyCount = FMath::Max(Definition.HeavyCount, 0);
		Result.SpawnInterval = FMath::Max(Definition.SpawnInterval, MinimumSpawnInterval);
		Result.MaxLiveEnemies = FMath::Max(Definition.MaxLiveEnemies, 1);
		Result.bFromScriptedDefinition = true;
		Result.DefinitionIndex = DefinitionIndex;

		const int32 ExtraWaveCount = FMath::Max(SafeWaveNumber - WaveDefinitions.Num(), 0);
		if (bScaleDifficulty && ExtraWaveCount > 0)
		{
			Result.FastCount += ExtraWaveCount * FMath::Max(AdditionalEnemiesPerWave, 0);
			Result.HeavyCount += ExtraWaveCount / FMath::Max(HeavyEnemyEveryNWaves, 1);
			Result.MaxLiveEnemies += ExtraWaveCount / FMath::Max(MaxLiveEnemyGrowthEveryNWaves, 1);
			Result.SpawnInterval *= FMath::Pow(SpawnIntervalMultiplierPerWave, ExtraWaveCount);
		}
	}
	else
	{
		const int32 WaveOffset = SafeWaveNumber - 1;
		Result.FastCount = FMath::Max(BaseEnemiesPerWave, 1);
		Result.HeavyCount = 0;
		Result.SpawnInterval = FMath::Max(BaseSpawnInterval, MinimumSpawnInterval);
		Result.MaxLiveEnemies = FMath::Max(BaseMaxLiveEnemies, 1);

		if (bScaleDifficulty)
		{
			Result.FastCount += WaveOffset * FMath::Max(AdditionalEnemiesPerWave, 0);
			Result.HeavyCount = WaveOffset / FMath::Max(HeavyEnemyEveryNWaves, 1);
			Result.MaxLiveEnemies += WaveOffset / FMath::Max(MaxLiveEnemyGrowthEveryNWaves, 1);
			Result.SpawnInterval *= FMath::Pow(SpawnIntervalMultiplierPerWave, WaveOffset);
		}
	}

	Result.SpawnInterval = FMath::Max(Result.SpawnInterval, MinimumSpawnInterval);
	return Result;
}

float AOBWaveManager::ResolveInitialWaitDuration() const
{
	if (WaveDefinitions.Num() > 0)
	{
		return FMath::Max(WaveDefinitions[0].DelayBeforeWave, 0.0f);
	}
	return FMath::Max(InitialWaitDuration, 0.0f);
}

float AOBWaveManager::ResolveIntermissionDuration() const
{
	float Duration = IntermissionDuration;
	const int32 NextDefinitionIndex = CurrentWaveNumber;
	if (WaveDefinitions.IsValidIndex(NextDefinitionIndex))
	{
		Duration = WaveDefinitions[NextDefinitionIndex].DelayBeforeWave;
	}
	return FMath::Clamp(Duration, 3.0f, 5.0f);
}

bool AOBWaveManager::IsGameOver() const
{
	const AOBGameState* OneBulletState = GetWorld() ? GetWorld()->GetGameState<AOBGameState>() : nullptr;
	return OneBulletState && OneBulletState->bGameOver;
}
