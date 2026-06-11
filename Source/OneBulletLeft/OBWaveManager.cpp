#include "OBWaveManager.h"

#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "OBCharacter.h"
#include "OBGameState.h"

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

void AOBWaveManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopWaves();
	Super::EndPlay(EndPlayReason);
}

void AOBWaveManager::ConfigureSpawner(
	TSubclassOf<AOBEnemy> InEnemyClass,
	TSubclassOf<AOBEnemy> InFastEnemyClass,
	TSubclassOf<AOBEnemy> InHeavyEnemyClass,
	bool bInSpawnOnlyInFront,
	float InFrontSpawnMinDot,
	bool bInAllowAnySpawn)
{
	if (InEnemyClass)
	{
		EnemyClass = InEnemyClass;
	}
	if (InFastEnemyClass)
	{
		FastEnemyClass = InFastEnemyClass;
	}
	if (InHeavyEnemyClass)
	{
		HeavyEnemyClass = InHeavyEnemyClass;
	}
	bSpawnEnemiesOnlyInFrontOfPlayer = bInSpawnOnlyInFront;
	FrontSpawnMinDot = InFrontSpawnMinDot;
	bAllowAnySpawnIfNoFrontPoint = bInAllowAnySpawn;
}

void AOBWaveManager::RestartWaves()
{
	if (!GetWorld())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	GetWorldTimerManager().ClearTimer(StateTimerHandle);
	GetWorldTimerManager().ClearTimer(CompletionCheckTimerHandle);

	CurrentWaveNumber = 0;
	EnemiesRemainingToSpawn = 0;
	LivingEnemyCount = CountLiveEnemies();
	CurrentDifficultyMultiplier = 1.0f;
	RemainingFastEnemies = 0;
	RemainingHeavyEnemies = 0;
	SetWaveState(EOBWaveState::Waiting);

	const float WaitDuration = ResolveInitialWaitDuration();
	if (WaitDuration <= 0.0f)
	{
		StartNextWave();
	}
	else
	{
		GetWorldTimerManager().SetTimer(StateTimerHandle, this, &AOBWaveManager::StartNextWave, WaitDuration, false);
	}
}

void AOBWaveManager::StopWaves()
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
		GetWorldTimerManager().ClearTimer(StateTimerHandle);
		GetWorldTimerManager().ClearTimer(CompletionCheckTimerHandle);
	}

	RemainingFastEnemies = 0;
	RemainingHeavyEnemies = 0;
	EnemiesRemainingToSpawn = 0;
	SetWaveState(EOBWaveState::Waiting);
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

void AOBWaveManager::StartNextWave()
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

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Wave %d started: %d enemies, difficulty x%.2f"),
		CurrentWaveNumber,
		EnemiesRemainingToSpawn,
		CurrentDifficultyMultiplier);

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

	GetWorldTimerManager().SetTimer(
		CompletionCheckTimerHandle,
		this,
		&AOBWaveManager::CheckWaveCompletion,
		0.1f,
		true);
	CheckWaveCompletion();
}

void AOBWaveManager::SpawnEnemyTick()
{
	if (WaveState != EOBWaveState::Active || IsGameOver())
	{
		return;
	}

	LivingEnemyCount = CountLiveEnemies();
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

	if (!SpawnEnemyOfType(TypeToSpawn))
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
	LivingEnemyCount = CountLiveEnemies();
	if (EnemiesRemainingToSpawn <= 0)
	{
		GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	}
}

void AOBWaveManager::CheckWaveCompletion()
{
	if (WaveState != EOBWaveState::Active || IsGameOver())
	{
		return;
	}

	LivingEnemyCount = CountLiveEnemies();
	if (EnemiesRemainingToSpawn > 0 || LivingEnemyCount > 0)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	GetWorldTimerManager().ClearTimer(CompletionCheckTimerHandle);
	SetWaveState(EOBWaveState::Completed);
	OnWaveCompleted.Broadcast(CurrentWaveNumber);

	UE_LOG(LogTemp, Log, TEXT("Wave %d completed"), CurrentWaveNumber);

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
	GetWorldTimerManager().SetTimer(StateTimerHandle, this, &AOBWaveManager::StartNextWave, Duration, false);
}

bool AOBWaveManager::SpawnEnemyOfType(EOBEnemyType Type)
{
	FVector SpawnLocation = FVector::ZeroVector;
	if (!TryChooseSpawnLocation(SpawnLocation))
	{
		return false;
	}

	TSubclassOf<AOBEnemy> ClassToSpawn = Type == EOBEnemyType::Heavy ? HeavyEnemyClass : FastEnemyClass;
	if (!ClassToSpawn)
	{
		ClassToSpawn = EnemyClass;
	}
	if (!ClassToSpawn)
	{
		return false;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AOBEnemy* Enemy = GetWorld()->SpawnActor<AOBEnemy>(ClassToSpawn, SpawnLocation, FRotator::ZeroRotator, Params);
	if (!Enemy)
	{
		return false;
	}

	Enemy->Configure(Type);
	Enemy->SetDifficultySpeedMultiplier(CurrentDifficultyMultiplier);
	Enemy->TriggerSpawnFeedback();
	return true;
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

int32 AOBWaveManager::CountLiveEnemies() const
{
	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsOfClass(this, AOBEnemy::StaticClass(), Enemies);

	int32 Count = 0;
	for (const AActor* EnemyActor : Enemies)
	{
		const AOBEnemy* Enemy = Cast<AOBEnemy>(EnemyActor);
		if (Enemy && !Enemy->IsDead())
		{
			++Count;
		}
	}
	return Count;
}

AOBWaveManager::FRuntimeWave AOBWaveManager::BuildWave(int32 WaveNumber) const
{
	FRuntimeWave Result;
	const int32 SafeWaveNumber = FMath::Max(WaveNumber, 1);

	if (bUseScriptedWaves && WaveDefinitions.Num() > 0)
	{
		const int32 DefinitionIndex = FMath::Min(SafeWaveNumber - 1, WaveDefinitions.Num() - 1);
		const FOBWaveDefinition& Definition = WaveDefinitions[DefinitionIndex];
		Result.FastCount = FMath::Max(Definition.FastCount, 0);
		Result.HeavyCount = FMath::Max(Definition.HeavyCount, 0);
		Result.SpawnInterval = FMath::Max(Definition.SpawnInterval, MinimumSpawnInterval);
		Result.MaxLiveEnemies = FMath::Max(Definition.MaxLiveEnemies, 1);

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
	if (bUseScriptedWaves && WaveDefinitions.Num() > 0)
	{
		return FMath::Max(WaveDefinitions[0].DelayBeforeWave, 0.0f);
	}
	return FMath::Max(InitialWaitDuration, 0.0f);
}

float AOBWaveManager::ResolveIntermissionDuration() const
{
	float Duration = IntermissionDuration;
	const int32 NextDefinitionIndex = CurrentWaveNumber;
	if (bUseScriptedWaves && WaveDefinitions.IsValidIndex(NextDefinitionIndex))
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
