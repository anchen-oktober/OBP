#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "OBEnemy.h"
#include "OBGameMode.generated.h"

class AOBBulletPickup;

UCLASS()
class ONEBULLETLEFT_API AOBGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AOBGameMode();

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Arena")
	bool bBuildGreyboxArena = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Spawning")
	float SpawnInterval = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Spawning")
	int32 MaxLiveEnemies = 8;

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

protected:
	FTimerHandle SpawnTimerHandle;
	TArray<FVector> SpawnPoints;

	void BuildGreyboxArena();
	void SpawnEnemyWaveTick();
	bool TryChooseSpawnLocation(FVector& OutLocation) const;
	int32 CountLiveEnemies() const;
	void SpawnBlock(const FVector& Location, const FVector& Scale, const FName& Name);
};
