#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "OBEnemy.h"
#include "OBGameMode.generated.h"

class AOBBulletPickup;
class AOBCharacter;

USTRUCT(BlueprintType)
struct FOBWaveDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Wave")
	int32 FastCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Wave")
	int32 HeavyCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Wave")
	float DelayBeforeWave = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Wave")
	float SpawnInterval = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Wave")
	int32 MaxLiveEnemies = 4;
};

UCLASS()
class ONEBULLETLEFT_API AOBGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AOBGameMode();

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Arena")
	bool bBuildGreyboxArena = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Window")
	bool bForceWindowedMode = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Window", meta=(EditCondition="bForceWindowedMode", ClampMin="320"))
	int32 WindowedResolutionX = 1280;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Window", meta=(EditCondition="bForceWindowedMode", ClampMin="240"))
	int32 WindowedResolutionY = 800;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Spawning")
	float SpawnInterval = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Spawning")
	int32 MaxLiveEnemies = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Waves")
	bool bUseScriptedWaves = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Waves")
	TArray<FOBWaveDefinition> WaveDefinitions;

	UPROPERTY(EditDefaultsOnly, Category="One Bullet|Spawning")
	TSubclassOf<AOBEnemy> EnemyClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="One Bullet|Spawning")
	TSubclassOf<AOBEnemy> FastEnemyClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="One Bullet|Spawning")
	TSubclassOf<AOBEnemy> HeavyEnemyClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="One Bullet|Bullet")
	TSubclassOf<AOBBulletPickup> BulletPickupClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Spawning|Visibility")
	bool bSpawnEnemiesOnlyInFrontOfPlayer = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Spawning|Visibility")
	float FrontSpawnMinDot = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Spawning|Visibility")
	bool bAllowAnySpawnIfNoFrontPoint = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Bullet")
	float BulletPickupDropHeight = 5.0f;

	UFUNCTION(BlueprintCallable, Category="One Bullet|Bullet")
	AOBBulletPickup* SpawnBulletPickup(const FVector& DropLocation);

	UFUNCTION(BlueprintCallable, Category="One Bullet|Flow")
	void RestartRun(AOBCharacter* Player);

protected:
	FTimerHandle SpawnTimerHandle;
	FTimerHandle WaveStartTimerHandle;
	FTimerHandle WindowModeTimerHandle;
	TArray<FVector> SpawnPoints;
	int32 CurrentWaveIndex = INDEX_NONE;
	int32 RemainingFastInWave = 0;
	int32 RemainingHeavyInWave = 0;

	void BuildGreyboxArena();
	void ApplyWindowMode();
	void SpawnEnemyWaveTick();
	void StartNextWave();
	void StartWave(int32 WaveIndex);
	bool SpawnEnemyOfType(EOBEnemyType Type);
	void RestartSpawning();
	void DestroyRunActors();
	bool FindRestartTransform(FVector& OutLocation, FRotator& OutRotation) const;
	bool TryChooseSpawnLocation(FVector& OutLocation) const;
	int32 CountLiveEnemies() const;
	void SpawnBlock(const FVector& Location, const FVector& Scale, const FName& Name);
};
