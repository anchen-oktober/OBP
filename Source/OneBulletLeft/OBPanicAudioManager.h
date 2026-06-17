#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OBPanicAudioManager.generated.h"

class USceneComponent;
class USoundBase;
class UAudioComponent;

enum class ECrowdRoarMode : uint8
{
	RandomSingle,
	LayeredPreset,
	CustomMix
};

enum class ECrowdLayeredRoarPreset : uint8
{
	Roar1AndRoar2,
	Roar1AndRoar3,
	Roar1Roar2Roar3
};

enum class ECrowdFootstepMode : uint8
{
	Run1Only,
	Run2Only,
	Run1AndRun2,
	CustomMix
};

UCLASS(PrioritizeCategories = "OneBulletSettings")
class ONEBULLETLEFT_API AOBPanicAudioManager : public AActor
{
	GENERATED_BODY()

public:
	AOBPanicAudioManager();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UAudioComponent> HeartbeatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UAudioComponent> LowDroneComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Audio Assets")
	TObjectPtr<USoundBase> ShotSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Audio Assets")
	TObjectPtr<USoundBase> BulletPickupSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Audio Assets")
	TObjectPtr<USoundBase> CrowdFootstepRun1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Audio Assets")
	TObjectPtr<USoundBase> CrowdFootstepRun2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Audio Assets")
	TObjectPtr<USoundBase> CrowdRoar1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Audio Assets")
	TObjectPtr<USoundBase> CrowdRoar2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Audio Assets")
	TObjectPtr<USoundBase> CrowdRoar3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Audio Assets")
	TObjectPtr<USoundBase> HeartbeatSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Audio Assets")
	TObjectPtr<USoundBase> WhisperAmbientSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Audio Assets")
	TObjectPtr<USoundBase> GhostAmbientSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Volume")
	float ShotVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Volume")
	float BulletPickupVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Volume")
	float HeartbeatVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Volume")
	float Roar1Volume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Volume")
	float Roar2Volume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Volume")
	float Roar3Volume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Volume")
	float FootstepRun1Volume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Volume")
	float FootstepRun2Volume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Volume")
	float WhisperVolume = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Volume")
	float GhostVolume = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Fade")
	float HeartbeatFadeIn = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Fade")
	float HeartbeatFadeOut = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Fade")
	float RoarFadeIn = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Fade")
	float RoarFadeOut = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Fade")
	float FootstepsFadeIn = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Fade")
	float FootstepsFadeOut = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Fade")
	float AmbientFadeIn = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Fade")
	float AmbientFadeOut = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Fade")
	float PickupReliefFadeOut = 0.65f;

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Panic Audio")
	void HandlePlayerShot(AActor* AudioFocus = nullptr);

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Panic Audio")
	void HandleBulletPickedUp(AActor* AudioFocus = nullptr);

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Panic Audio")
	void PlayShotSound(AActor* AudioFocus = nullptr);

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Panic Audio")
	void StartPanicAudio(AActor* AudioFocus = nullptr);

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Panic Audio")
	void StopPanicAudio(AActor* AudioFocus = nullptr, bool bPlayBulletPickupSound = true);

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Panic Audio")
	void StartAmbientHorrorLoop();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Panic Audio")
	void PlayRandomAmbientHorror();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Panic Audio")
	void ApplyReliefReactionToEnemies(AActor* PlayerActor = nullptr);

	UFUNCTION(BlueprintPure, Category="OneBulletSettings|Panic Audio")
	bool IsPanicAudioActive() const { return bPanicAudioActive; }

protected:
	static constexpr float PanicStartDelayMin = 0.15f;
	static constexpr float PanicStartDelayMax = 0.20f;
	static constexpr float RoarDelayAfterHeartbeat = 0.08f;
	static constexpr float FootstepsDelayAfterRoar = 0.12f;
	static constexpr float AmbientMinInterval = 20.0f;
	static constexpr float AmbientMaxInterval = 60.0f;
	static constexpr float AmbientPlayChance = 0.62f;
	static constexpr float ShotPitch = 1.0f;
	static constexpr float ReliefSlowDurationMin = 1.0f;
	static constexpr float ReliefSlowDurationMax = 1.5f;
	static constexpr float ReliefSpeedMultiplierMin = 0.6f;
	static constexpr float ReliefSpeedMultiplierMax = 0.7f;
	static constexpr float ReliefStepBackDistanceMin = 50.0f;
	static constexpr float ReliefStepBackDistanceMax = 100.0f;
	static constexpr float ReliefStepBackDurationMin = 0.3f;
	static constexpr float ReliefStepBackDurationMax = 0.5f;

	ECrowdRoarMode CrowdRoarMode = ECrowdRoarMode::LayeredPreset;
	ECrowdLayeredRoarPreset CrowdRoarPreset = ECrowdLayeredRoarPreset::Roar1Roar2Roar3;
	ECrowdFootstepMode CrowdFootstepMode = ECrowdFootstepMode::Run1AndRun2;
	bool bCustomUseRoar1 = true;
	bool bCustomUseRoar2 = true;
	bool bCustomUseRoar3 = true;
	bool bCustomUseFootstepRun1 = true;
	bool bCustomUseFootstepRun2 = true;

	FTimerHandle PanicAudioDelayTimerHandle;
	FTimerHandle RoarStartTimerHandle;
	FTimerHandle FootstepsStartTimerHandle;
	FTimerHandle AmbientHorrorTimerHandle;
	TWeakObjectPtr<AActor> PendingAudioFocus;
	bool bPanicAudioActive = false;
	bool bHeartbeatLayerActive = false;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UAudioComponent>> ActiveRoarComponents;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UAudioComponent>> ActiveFootstepComponents;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UAudioComponent>> ActiveAmbientComponents;

	void StartPanicAudioAfterDelay();
	void StartHeartbeatLayer();
	void StopHeartbeatLayer();
	void StartCrowdRoarLayer();
	void StopCrowdRoarLayer();
	void StartCrowdFootstepsLayer();
	void StopCrowdFootstepsLayer();
	void StopLowDroneLayer();
	void KeepLoopLayersAlive();
	void ScheduleNextAmbientHorror();
	void PlayAmbientSound(USoundBase* Sound, float Volume, const TCHAR* DebugName);
	void PlayRoarSound(USoundBase* Sound, float Volume, const TCHAR* DebugName);
	void PlayFootstepSound(USoundBase* Sound, float Volume, const TCHAR* DebugName);
	UAudioComponent* CreateAudioLayer(USoundBase* Sound, float Volume, float Pitch, const TCHAR* DebugName);
	void FadeOutAndForget(UAudioComponent* Component, float FadeOutDuration) const;
	void DebugMissingSound(const TCHAR* SoundName) const;
};
