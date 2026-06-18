#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OBPanicAudioManager.generated.h"

class USceneComponent;
class USoundBase;
class UAudioComponent;

UENUM(BlueprintType)
enum class ECrowdRoarMode : uint8
{
	RandomSingle UMETA(DisplayName = "Random Single"),
	PresetCombination UMETA(DisplayName = "Preset Combination"),
	CustomMix UMETA(DisplayName = "Custom Mix")
};

UENUM(BlueprintType)
enum class ECrowdRoarPreset : uint8
{
	Roar1AndRoar2 UMETA(DisplayName = "Roar1 + Roar2"),
	Roar1AndRoar3 UMETA(DisplayName = "Roar1 + Roar3"),
	Roar2AndRoar3 UMETA(DisplayName = "Roar2 + Roar3"),
	Roar1AndRoar2AndRoar3 UMETA(DisplayName = "Roar1 + Roar2 + Roar3")
};

UENUM(BlueprintType)
enum class ECrowdFootstepMode : uint8
{
	Run1Only UMETA(DisplayName = "Run1 Only"),
	Run2Only UMETA(DisplayName = "Run2 Only"),
	Run1AndRun2 UMETA(DisplayName = "Run1 + Run2")
};

UCLASS(PrioritizeCategories = "OneBulletSettings", HideCategories = (Actor, Advanced, Collision, Cooking, DataLayers, HLOD, Input, Networking, Physics, Rendering, Replication, WorldPartition))
class ONEBULLETLEFT_API AOBPanicAudioManager : public AActor
{
	GENERATED_BODY()

public:
	AOBPanicAudioManager();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> HeartbeatComponent;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> LowDroneComponent;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> MusicComponent;

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
	TObjectPtr<USoundBase> LowDroneSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Audio Assets")
	TObjectPtr<USoundBase> MusicSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Audio Assets")
	TObjectPtr<USoundBase> WhisperAmbientSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Audio Assets")
	TObjectPtr<USoundBase> GhostAmbientSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Ambient Horror")
	bool bAmbientHorrorEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Ambient Horror", meta=(ClampMin="1.0", UIMin="1.0"))
	float MinAmbientInterval = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Ambient Horror", meta=(ClampMin="1.0", UIMin="1.0"))
	float MaxAmbientInterval = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Ambient Horror", meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
	float AmbientPlayChance = 0.62f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Music")
	bool bMusicEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Roar")
	ECrowdRoarMode RoarMode = ECrowdRoarMode::PresetCombination;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Roar")
	ECrowdRoarPreset RoarPreset = ECrowdRoarPreset::Roar1AndRoar2AndRoar3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Roar")
	bool bRoar1Enabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Roar")
	bool bRoar2Enabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Roar")
	bool bRoar3Enabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Footsteps")
	ECrowdFootstepMode FootstepsMode = ECrowdFootstepMode::Run1AndRun2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Volume")
	float ShotVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Volume")
	float BulletPickupVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Volume")
	float HeartbeatVolume = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Volume")
	float LowDroneVolume = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Volume")
	float MusicVolume = 0.75f;

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
	float HeartbeatFadeIn = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Fade")
	float HeartbeatFadeOut = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Fade")
	float LowDroneFadeIn = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Fade")
	float LowDroneFadeOut = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Fade")
	float MusicFadeIn = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Fade")
	float MusicFadeOut = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Fade")
	float RoarFadeIn = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Fade")
	float RoarFadeOut = 0.85f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Fade")
	float FootstepsFadeIn = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Fade")
	float FootstepsFadeOut = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Fade")
	float AmbientFadeIn = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Fade")
	float AmbientFadeOut = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Fade")
	float PickupReliefFadeOut = 1.0f;

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
	void StartPanicSequence(AActor* AudioFocus = nullptr);

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Panic Audio")
	void StopPanicSequence(AActor* AudioFocus = nullptr, bool bPlayBulletPickupSound = true);

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Panic Audio")
	void PlayRoar();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Panic Audio")
	void PlayFootsteps();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Panic Audio")
	void StartMusicLayer();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Panic Audio")
	void StopMusicLayer();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Panic Audio")
	void StartAmbientHorrorLoop();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Panic Audio")
	void StopAmbientHorrorLoop();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Panic Audio")
	void ScheduleNextAmbientHorror();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Panic Audio")
	void TryPlayAmbientHorror();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Panic Audio")
	void PlayRandomAmbientHorror();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Panic Audio")
	void ApplyReliefReactionToEnemies(AActor* PlayerActor = nullptr);

	UFUNCTION(BlueprintPure, Category="OneBulletSettings|Panic Audio")
	bool IsPanicAudioActive() const { return bPanicAudioActive; }

protected:
	static constexpr float PanicStartDelayMin = 0.15f;
	static constexpr float PanicStartDelayMax = 0.20f;
	static constexpr float LowDroneDelayAfterHeartbeat = 0.06f;
	static constexpr float RoarDelayAfterLowDrone = 0.08f;
	static constexpr float FootstepsDelayAfterRoar = 0.12f;
	static constexpr float ShotPitch = 1.0f;
	static constexpr float ReliefSlowDurationMin = 1.0f;
	static constexpr float ReliefSlowDurationMax = 1.5f;
	static constexpr float ReliefSpeedMultiplierMin = 0.6f;
	static constexpr float ReliefSpeedMultiplierMax = 0.7f;
	static constexpr float ReliefStepBackDistanceMin = 50.0f;
	static constexpr float ReliefStepBackDistanceMax = 100.0f;
	static constexpr float ReliefStepBackDurationMin = 0.3f;
	static constexpr float ReliefStepBackDurationMax = 0.5f;

	FTimerHandle PanicAudioDelayTimerHandle;
	FTimerHandle LowDroneStartTimerHandle;
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
	void StartLowDroneLayer();
	void StartCrowdRoarLayer();
	void StopCrowdRoarLayer();
	void StartCrowdFootstepsLayer();
	void StopCrowdFootstepsLayer();
	void StopLowDroneLayer();
	void KeepLoopLayersAlive();
	void PlayAmbientSound(USoundBase* Sound, float Volume, const TCHAR* DebugName);
	void PlayRoarSound(USoundBase* Sound, float Volume, const TCHAR* DebugName);
	void PlayFootstepSound(USoundBase* Sound, float Volume, const TCHAR* DebugName);
	UAudioComponent* CreateAudioLayer(USoundBase* Sound, float Volume, float Pitch, const TCHAR* DebugName);
	void FadeOutAndForget(UAudioComponent* Component, float FadeOutDuration) const;
	void DebugMissingSound(const TCHAR* SoundName) const;
};
