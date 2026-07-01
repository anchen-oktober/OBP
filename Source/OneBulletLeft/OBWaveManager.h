#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OBEnemy.h"
#include "OBWaveManager.generated.h"

class AOBEnemySpawnPoint;
class UNiagaraSystem;

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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FOBWaveEnemySpawnedSignature,
	AOBEnemy*,
	Enemy,
	EOBEnemyType,
	EnemyType,
	FVector,
	SpawnLocation,
	int32,
	LivingEnemies);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOBWaveEnemyDiedSignature, AOBEnemy*, Enemy, int32, LivingEnemies);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOBWaveEnemyCountChangedSignature, int32, LivingEnemies);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOBWaveSpawnWarningSignature,
	EOBEnemyType,
	EnemyType,
	FVector,
	SpawnLocation,
	float,
	WarningDuration);

UCLASS(Blueprintable, PrioritizeCategories = "OneBulletSettings")
class ONEBULLETLEFT_API AOBWaveManager : public AActor
{
	GENERATED_BODY()

public:
	AOBWaveManager();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Waves")
	bool bAutoStartWaves = true;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadWrite,
		Category="OneBulletSettings|Waves",
		meta=(DisplayName="Wave Definitions (Overrides Generated Waves)"))
	TArray<FOBWaveDefinition> WaveDefinitions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Waves|Completion")
	bool bAutoCompleteWhenAllEnemiesDefeated = true;

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

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="OneBulletSettings|Spawning")
	TArray<TObjectPtr<AOBEnemySpawnPoint>> EnemySpawnPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Spawning|Safety", meta=(ClampMin="0.0", UIMin="1200.0", UIMax="2000.0", DisplayName="Min Spawn Distance From Player"))
	float MinimumSpawnDistanceFromPlayer = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Spawning|Safety", meta=(ClampMin="1", UIMin="10", UIMax="20"))
	int32 MaxSpawnAttempts = 16;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Spawning|Safety")
	bool bPreferSpawnPointsOutsidePlayerView = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Spawning|Safety", meta=(ClampMin="0.0", UIMin="0.0", UIMax="256.0"))
	float SpawnScreenEdgePadding = 64.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Spawning|Safety", meta=(ClampMin="-1.0", ClampMax="1.0"))
	float DirectViewMinDot = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Spawning|Safety", meta=(ClampMin="0.0", UIMin="500.0", UIMax="2500.0"))
	float SpawnSafeSearchRadius = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Spawning|Safety", meta=(ClampMin="0.0", UIMin="0.0", UIMax="100.0"))
	float SpawnCollisionPadding = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Spawning|Warning", meta=(ClampMin="0.3", ClampMax="0.7", UIMin="0.3", UIMax="0.7"))
	float SpawnWarningDuration = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Spawning|Warning", meta=(ClampMin="0.3", ClampMax="0.5", UIMin="0.3", UIMax="0.5"))
	float SpawnGracePeriod = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Spawning|Warning")
	TObjectPtr<UNiagaraSystem> SpawnWarningEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Spawning|Warning")
	bool bUseSpawnWarningLight = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Spawning|Warning", meta=(EditCondition="bUseSpawnWarningLight", ClampMin="0.0"))
	float SpawnWarningLightIntensity = 12000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Spawning|Warning", meta=(EditCondition="bUseSpawnWarningLight", ClampMin="0.0"))
	float SpawnWarningLightRadius = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Spawning|Warning", meta=(EditCondition="bUseSpawnWarningLight"))
	FLinearColor SpawnWarningLightColor = FLinearColor(1.0f, 0.12f, 0.02f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Debug")
	bool bEnableWaveDebugLogs = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Debug")
	bool bEnableWaveDebugScreenMessages = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Debug", meta=(ClampMin="0.1"))
	float DebugScreenMessageDuration = 2.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="OneBulletSettings|Waves|Runtime")
	EOBWaveState WaveState = EOBWaveState::Waiting;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="OneBulletSettings|Waves|Runtime")
	int32 CurrentWaveNumber = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="OneBulletSettings|Waves|Runtime")
	int32 EnemiesRemainingToSpawn = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="OneBulletSettings|Waves|Runtime")
	int32 LivingEnemyCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="OneBulletSettings|Waves|Runtime")
	int32 SpawnedThisWave = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="OneBulletSettings|Waves|Runtime")
	int32 HeavyEnemiesSpawnedThisWave = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="OneBulletSettings|Waves|Runtime")
	int32 TotalEnemiesSpawned = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="OneBulletSettings|Debug")
	int32 SpawnWarningVFXSpawned = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="OneBulletSettings|Debug")
	int32 SpawnWarningLightsSpawned = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="OneBulletSettings|Waves|Runtime")
	float CurrentDifficultyMultiplier = 1.0f;

	UPROPERTY(BlueprintAssignable, Category="OneBulletSettings|Waves|Events")
	FOBWaveStateChangedSignature OnWaveStateChanged;

	UPROPERTY(BlueprintAssignable, Category="OneBulletSettings|Waves|Events")
	FOBWaveStartedSignature OnWaveStarted;

	UPROPERTY(BlueprintAssignable, Category="OneBulletSettings|Waves|Events")
	FOBWaveCompletedSignature OnWaveCompleted;

	UPROPERTY(BlueprintAssignable, Category="OneBulletSettings|Waves|Events")
	FOBWaveEnemySpawnedSignature OnEnemySpawned;

	UPROPERTY(BlueprintAssignable, Category="OneBulletSettings|Waves|Events")
	FOBWaveEnemyDiedSignature OnEnemyDied;

	UPROPERTY(BlueprintAssignable, Category="OneBulletSettings|Waves|Events")
	FOBWaveEnemyCountChangedSignature OnEnemyCountChanged;

	UPROPERTY(BlueprintAssignable, Category="OneBulletSettings|Waves|Events")
	FOBWaveSpawnWarningSignature OnSpawnWarning;

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Waves")
	void StartWaves();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Waves")
	void StartWave();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Waves")
	void RestartWaves();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Waves")
	void StopWave();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Waves")
	void StopWaves();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Waves")
	void CompleteCurrentWave();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Spawning")
	AOBEnemy* SpawnEnemy(EOBEnemyType Type);

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Spawning")
	void RefreshEnemySpawnPoints();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Spawning")
	void ClearSpawnedEnemies();

	UFUNCTION(BlueprintPure, Category="OneBulletSettings|Waves")
	float GetIntermissionTimeRemaining() const;

	UFUNCTION(BlueprintPure, Category="OneBulletSettings|Debug")
	int32 GetPendingSpawnCount() const { return PendingEnemySpawnTypes.Num(); }

	UFUNCTION(BlueprintPure, Category="OneBulletSettings|Debug")
	int32 GetTrackedEnemyCount() const { return SpawnedEnemies.Num(); }

private:
	struct FRuntimeWave
	{
		int32 FastCount = 0;
		int32 HeavyCount = 0;
		float SpawnInterval = 1.0f;
		int32 MaxLiveEnemies = 1;
		bool bFromScriptedDefinition = false;
		int32 DefinitionIndex = INDEX_NONE;
	};

	FTimerHandle SpawnTimerHandle;
	FTimerHandle StateTimerHandle;
	TSet<TWeakObjectPtr<AOBEnemy>> SpawnedEnemies;
	TArray<EOBEnemyType> PendingEnemySpawnTypes;
	int32 RemainingFastEnemies = 0;
	int32 RemainingHeavyEnemies = 0;
	int32 CurrentMaxLiveEnemies = 1;

	struct FEnemySpawnCandidate
	{
		TObjectPtr<AOBEnemySpawnPoint> SpawnPoint = nullptr;
		FVector SpawnLocation = FVector::ZeroVector;
		float DistanceToPlayer = 0.0f;
	};

	void SetWaveState(EOBWaveState NewState);
	void BeginNextWave();
	void SpawnEnemyTick();
	void CheckWaveCompletion();
	void EnterIntermission();
	bool TryChooseSpawnPoint(EOBEnemyType Type, FEnemySpawnCandidate& OutCandidate);
	bool TryResolveSpawnPointCandidate(EOBEnemyType Type, AOBEnemySpawnPoint* SpawnPoint, const FVector& PlayerLocation, bool bHasPlayer, bool bAllowTooClose, FEnemySpawnCandidate& OutCandidate, FString& OutFailureReason) const;
	bool TryResolveSafeSpawnLocation(EOBEnemyType Type, const FVector& MarkerLocation, FVector& OutSpawnLocation, FString& OutFailureReason) const;
	bool IsSpawnLocationSafe(EOBEnemyType Type, const FVector& NavLocation, FVector& OutSpawnLocation, FString& OutFailureReason) const;
	bool CanReachPlayerFromSpawn(const FVector& SpawnLocation) const;
	void GetSpawnCapsule(EOBEnemyType Type, float& OutRadius, float& OutHalfHeight) const;
	void ShowSpawnWarning(EOBEnemyType Type, const FVector& SpawnLocation);
	void RefreshLivingEnemyCount();
	FRuntimeWave BuildWave(int32 WaveNumber) const;
	float ResolveInitialWaitDuration() const;
	float ResolveIntermissionDuration() const;
	bool IsGameOver() const;
	void DebugWaveMessage(const FString& Message, const FColor& Color = FColor::White) const;

	UFUNCTION()
	void HandleEnemyDeathReported(AOBEnemy* Enemy);

	UFUNCTION()
	void HandleEnemyDestroyed(AActor* DestroyedActor);
};
