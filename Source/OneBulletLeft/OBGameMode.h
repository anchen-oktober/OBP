#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "OBWaveManager.h"
#include "OBGameMode.generated.h"

class AOBBulletPickup;
class AOBCharacter;
class AOBPanicAudioManager;

UCLASS(PrioritizeCategories = "OneBulletSettings")
class ONEBULLETLEFT_API AOBGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AOBGameMode();

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Window")
	bool bForceWindowedMode = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Window", meta=(EditCondition="bForceWindowedMode", ClampMin="320"))
	int32 WindowedResolutionX = 1280;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Window", meta=(EditCondition="bForceWindowedMode", ClampMin="240"))
	int32 WindowedResolutionY = 800;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="OneBulletSettings|Waves")
	TSubclassOf<AOBWaveManager> WaveManagerClass;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="OneBulletSettings|Waves")
	TObjectPtr<AOBWaveManager> WaveManager;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="OneBulletSettings|Bullet")
	TSubclassOf<AOBBulletPickup> BulletPickupClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Bullet")
	float BulletPickupDropHeight = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio")
	TSubclassOf<AOBPanicAudioManager> PanicAudioManagerClass;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="OneBulletSettings|Panic Audio")
	TObjectPtr<AOBPanicAudioManager> PanicAudioManager;

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Bullet")
	AOBBulletPickup* SpawnBulletPickup(const FVector& DropLocation);

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Bullet")
	void PlayBulletTrail(const FVector& TraceStart, const FVector& TraceEnd);

	void PlayBulletFlight(const FVector& TraceStart, const FVector& TraceEnd, AOBBulletPickup* DestinationPickup);

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Flow")
	void RestartRun(AOBCharacter* Player);

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Panic Audio")
	void StartPanicAudioAfterShot(AActor* AudioFocus);

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Panic Audio")
	void HandlePlayerShot(AActor* AudioFocus);

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Panic Audio")
	void HandleBulletPickedUp(AActor* AudioFocus);

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Panic Audio")
	void StopPanicAudio(AActor* AudioFocus = nullptr, bool bPlayBulletPickupSound = true);

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Panic Audio")
	void PlayShotSound(AActor* AudioFocus);

	UFUNCTION(BlueprintPure, Category="OneBulletSettings|Waves")
	AOBWaveManager* GetWaveManager() const { return WaveManager; }

protected:
	FTimerHandle WindowModeTimerHandle;

	void ApplyWindowMode();
	void InitializeWaveManager();
	void InitializePanicAudioManager();
	void DestroyRunActors();
	bool FindRestartTransform(FVector& OutLocation, FRotator& OutRotation) const;
};
