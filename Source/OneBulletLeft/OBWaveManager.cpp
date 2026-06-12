#include "OBWaveManager.h"

#include "Engine/Engine.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "OBCharacter.h"
#include "OBGameState.h"

DEFINE_LOG_CATEGORY_STATIC(LogOBWaveManager, Log, All);

AOBWaveManager::AOBWaveManager()
{
	PrimaryActorTick.bCanEverTick = false;
	EnemyClass = AOBEnemy::StaticClass();
	FastEnemyClass = AOBEnemy::StaticClass();
	HeavyEnemyClass = AOBEnemy::StaticClass();

	SpawnPoints = {
		FVector(1200.0f, 1200.0f, 120.0f),
		FVector(-1200.0f, 1200.0f, 120.0f),
		FVector(1200.0f, -1200.0f, 120.0f),
		FVector(-1200.0f, -1200.0f, 120.0f),
		FVector(0.0f, 1450.0f, 120.0f),
		FVector(0.0f, -1450.0f, 120.0f)
	};

	WaveDefinitions = {
		FOBWaveDefinition{2, 0, 1.0f, 0.8f, 2},
		FOBWaveDefinition{1, 1, 4.0f, 1.0f, 3},
		FOBWaveDefinition{3, 2, 4.0f, 0.9f, 5}
	};
}

void AOBWaveManager::BeginPlay()
{
	Super::BeginPlay();

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

	CurrentWaveNumber = 0;
	EnemiesRemainingToSpawn = 0;
	RefreshLivingEnemyCount();
	CurrentDifficultyMultiplier = 1.0f;
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
	EnemiesRemainingToSpawn = RemainingFastEnemies + RemainingHeavyEnemies;
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

	const int32 TotalRemaining = RemainingFastEnemies + RemainingHeavyEnemies;
	if (TotalRemaining <= 0)
	{
		EnemiesRemainingToSpawn = 0;
		GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
		return;
	}

	EOBEnemyType TypeToSpawn = EOBEnemyType::Fast;
	if (RemainingFastEnemies <= 0)
	{
		TypeToSpawn = EOBEnemyType::Heavy;
	}
	else if (RemainingHeavyEnemies > 0)
	{
		const float HeavyChance = static_cast<float>(RemainingHeavyEnemies) / static_cast<float>(TotalRemaining);
		TypeToSpawn = FMath::FRand() < HeavyChance ? EOBEnemyType::Heavy : EOBEnemyType::Fast;
	}

	if (!SpawnEnemy(TypeToSpawn))
	{
		return;
	}

	if (TypeToSpawn == EOBEnemyType::Heavy)
	{
		--RemainingHeavyEnemies;
	}
	else
	{
		--RemainingFastEnemies;
	}

	EnemiesRemainingToSpawn = RemainingFastEnemies + RemainingHeavyEnemies;
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
	FVector SpawnLocation = FVector::ZeroVector;
	if (!TryChooseSpawnLocation(SpawnLocation))
	{
		UE_LOG(LogOBWaveManager, Warning, TEXT("WaveManager could not find a valid enemy spawn point."));
		return nullptr;
	}

	TSubclassOf<AOBEnemy> ClassToSpawn = Type == EOBEnemyType::Heavy ? HeavyEnemyClass : FastEnemyClass;
	if (!ClassToSpawn)
	{
		ClassToSpawn = EnemyClass;
	}
	if (!ClassToSpawn)
	{
		UE_LOG(LogOBWaveManager, Error, TEXT("WaveManager has no enemy class configured for type %d."), static_cast<int32>(Type));
		return nullptr;
	}

	const FTransform SpawnTransform(FRotator::ZeroRotator, SpawnLocation);
	AOBEnemy* Enemy = GetWorld()->SpawnActorDeferred<AOBEnemy>(
		ClassToSpawn,
		SpawnTransform,
		this,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Enemy)
	{
		UE_LOG(LogOBWaveManager, Error, TEXT("WaveManager failed to spawn enemy class %s."), *GetNameSafe(ClassToSpawn));
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
		return nullptr;
	}

	Enemy->TriggerSpawnFeedback();
	OnEnemySpawned.Broadcast(Enemy, Type, SpawnLocation, LivingEnemyCount);
	DebugWaveMessage(
		FString::Printf(
			TEXT("Wave %d spawned %s at %s; living: %d"),
			CurrentWaveNumber,
			Type == EOBEnemyType::Heavy ? TEXT("Heavy") : TEXT("Fast"),
			*SpawnLocation.ToCompactString(),
			LivingEnemyCount),
		FColor::Yellow);
	return Enemy;
}

bool AOBWaveManager::TryChooseSpawnLocation(FVector& OutLocation) const
{
	if (SpawnPoints.Num() == 0)
	{
		return false;
	}

	const AOBCharacter* Player = Cast<AOBCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!bSpawnEnemiesOnlyInFrontOfPlayer || !Player)
	{
		OutLocation = SpawnPoints[FMath::RandRange(0, SpawnPoints.Num() - 1)];
		return true;
	}

	TArray<FVector> FrontSpawnPoints;
	const FVector PlayerLocation = Player->GetActorLocation();
	FVector PlayerForward = Player->GetActorForwardVector().GetSafeNormal2D();
	if (const AController* PlayerController = Player->GetController())
	{
		PlayerForward = FRotationMatrix(FRotator(0.0f, PlayerController->GetControlRotation().Yaw, 0.0f))
			.GetUnitAxis(EAxis::X)
			.GetSafeNormal2D();
	}

	for (const FVector& SpawnPoint : SpawnPoints)
	{
		const FVector ToSpawn = (SpawnPoint - PlayerLocation).GetSafeNormal2D();
		if (FVector::DotProduct(PlayerForward, ToSpawn) >= FrontSpawnMinDot)
		{
			FrontSpawnPoints.Add(SpawnPoint);
		}
	}

	if (FrontSpawnPoints.Num() > 0)
	{
		OutLocation = FrontSpawnPoints[FMath::RandRange(0, FrontSpawnPoints.Num() - 1)];
		return true;
	}

	if (bAllowAnySpawnIfNoFrontPoint)
	{
		OutLocation = SpawnPoints[FMath::RandRange(0, SpawnPoints.Num() - 1)];
		return true;
	}

	return false;
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
