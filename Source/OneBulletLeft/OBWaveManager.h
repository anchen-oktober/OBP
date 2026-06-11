#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OBEnemy.h"
#include "OBWaveManager.generated.h"

UENUM(BlueprintType)
enum class EOBWaveState : uint8
{
	Waiting,
	Active,
	Completed,
	Intermission
};

USTRUCT(BlueprintType)
struct FOBWaveDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Wave", meta=(ClampMin="0"))
	int32 FastCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Wave", meta=(ClampMin="0"))
	int32 HeavyCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Wave", meta=(ClampMin="0.0"))
	float DelayBeforeWave = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Wave", meta=(ClampMin="0.05"))
	float SpawnInterval = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Wave", meta=(ClampMin="1"))
	int32 MaxLiveEnemies = 4;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOBWaveStateChangedSignature, EOBWaveState, NewState, EOBWaveState, PreviousState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOBWaveStartedSignature, int32, WaveNumber, int32, EnemyCount, float, DifficultyMultiplier);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOBWaveCompletedSignature, int32, WaveNumber);

UCLASS(Blueprintable, PrioritizeCategories = "OneBulletSettings")
class ONEBULLETLEFT_API AOBWaveManager : public AActor
{
	GENERATED_BODY()

public:
	AOBWaveManager();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Waves")
	bool bUseScriptedWaves = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Waves")
	TArray<FOBWaveDefinition> WaveDefinitions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Waves|Timing", meta=(ClampMin="0.0"))
	float InitialWaitDuration = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Waves|Timing", meta=(ClampMin="3.0", ClampMax="5.0"))
	float IntermissionDuration = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Waves|Timing", meta=(ClampMin="0.0", ClampMax="1.0"))
	float CompletedStateDuration = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Waves|Generated", meta=(ClampMin="1"))
	int32 BaseEnemiesPerWave = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Waves|Generated", meta=(ClampMin="0"))
	int32 AdditionalEnemiesPerWave = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Waves|Generated", meta=(ClampMin="1"))
	int32 HeavyEnemyEveryNWaves = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Waves|Scaling")
	bool bScaleDifficulty = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Waves|Scaling", meta=(ClampMin="0.0", ClampMax="1.0"))
	float EnemySpeedIncreasePerWave = 0.06f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Waves|Scaling", meta=(ClampMin="1.0"))
	float MaxEnemySpeedMultiplier = 1.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Waves|Scaling", meta=(ClampMin="0.1", ClampMax="1.0"))
	float SpawnIntervalMultiplierPerWave = 0.96f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Waves|Scaling", meta=(ClampMin="0.05"))
	float MinimumSpawnInterval = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Waves|Scaling", meta=(ClampMin="1"))
	int32 MaxLiveEnemyGrowthEveryNWaves = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Spawning", meta=(ClampMin="0.05"))
	float BaseSpawnInterval = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Spawning", meta=(ClampMin="1"))
	int32 BaseMaxLiveEnemies = 8;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="OneBulletSettings|Spawning")
	TSubclassOf<AOBEnemy> EnemyClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="OneBulletSettings|Spawning")
	TSubclassOf<AOBEnemy> FastEnemyClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="OneBulletSettings|Spawning")
	TSubclassOf<AOBEnemy> HeavyEnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Spawning")
	TArray<FVector> SpawnPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Spawning|Visibility")
	bool bSpawnEnemiesOnlyInFrontOfPlayer = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Spawning|Visibility")
	float FrontSpawnMinDot = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Spawning|Visibility")
	bool bAllowAnySpawnIfNoFrontPoint = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="OneBulletSettings|Waves|Runtime")
	EOBWaveState WaveState = EOBWaveState::Waiting;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="OneBulletSettings|Waves|Runtime")
	int32 CurrentWaveNumber = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="OneBulletSettings|Waves|Runtime")
	int32 EnemiesRemainingToSpawn = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="OneBulletSettings|Waves|Runtime")
	int32 LivingEnemyCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="OneBulletSettings|Waves|Runtime")
	float CurrentDifficultyMultiplier = 1.0f;

	UPROPERTY(BlueprintAssignable, Category="OneBulletSettings|Waves|Events")
	FOBWaveStateChangedSignature OnWaveStateChanged;

	UPROPERTY(BlueprintAssignable, Category="OneBulletSettings|Waves|Events")
	FOBWaveStartedSignature OnWaveStarted;

	UPROPERTY(BlueprintAssignable, Category="OneBulletSettings|Waves|Events")
	FOBWaveCompletedSignature OnWaveCompleted;

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Waves")
	void RestartWaves();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Waves")
	void StopWaves();

	UFUNCTION(BlueprintPure, Category="OneBulletSettings|Waves")
	float GetIntermissionTimeRemaining() const;

	void ConfigureSpawner(
		TSubclassOf<AOBEnemy> InEnemyClass,
		TSubclassOf<AOBEnemy> InFastEnemyClass,
		TSubclassOf<AOBEnemy> InHeavyEnemyClass,
		bool bInSpawnOnlyInFront,
		float InFrontSpawnMinDot,
		bool bInAllowAnySpawn);

private:
	struct FRuntimeWave
	{
		int32 FastCount = 0;
		int32 HeavyCount = 0;
		float SpawnInterval = 1.0f;
		int32 MaxLiveEnemies = 1;
	};

	FTimerHandle SpawnTimerHandle;
	FTimerHandle StateTimerHandle;
	FTimerHandle CompletionCheckTimerHandle;
	int32 RemainingFastEnemies = 0;
	int32 RemainingHeavyEnemies = 0;
	int32 CurrentMaxLiveEnemies = 1;

	void SetWaveState(EOBWaveState NewState);
	void StartNextWave();
	void SpawnEnemyTick();
	void CheckWaveCompletion();
	void EnterIntermission();
	bool SpawnEnemyOfType(EOBEnemyType Type);
	bool TryChooseSpawnLocation(FVector& OutLocation) const;
	int32 CountLiveEnemies() const;
	FRuntimeWave BuildWave(int32 WaveNumber) const;
	float ResolveInitialWaitDuration() const;
	float ResolveIntermissionDuration() const;
	bool IsGameOver() const;
};
