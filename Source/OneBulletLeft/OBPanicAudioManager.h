#pragma once

#include "CoreMinimal.h"
#include "Components/AudioComponent.h"
#include "GameFramework/Actor.h"
#include "OBPanicAudioManager.generated.h"

class USceneComponent;
class USoundBase;
class USoundClass;

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
	TObjectPtr<UAudioComponent> BackgroundMusicComponent;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> SelectedMusicSound;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> LastSelectedMusicSound;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Panic Audio | Music", meta=(ExposeOnSpawn="false", ToolTip="Add background music tracks here. One track will be selected randomly at runtime."))
	TArray<TObjectPtr<USoundBase>> BackgroundMusicPlaylist;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Panic Audio | Music", meta=(ExposeOnSpawn="false", ToolTip="Enable or disable the persistent background music layer."))
	bool bMusicEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Panic Audio | Music", meta=(ExposeOnSpawn="false", ToolTip="If enabled, one track from BackgroundMusicPlaylist is selected randomly on BeginPlay. If disabled, the first valid track is used."))
	bool bRandomizeMusic = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Panic Audio | Music", meta=(ExposeOnSpawn="false", ToolTip="When selecting music again during the same runtime, avoid choosing the same track twice in a row when another valid track exists."))
	bool bAvoidRepeatSameTrack = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Panic Audio | Sound Classes", meta=(ExposeOnSpawn="false", ToolTip="Optional Sound Class override for the persistent background music playlist. Leave empty to use the track asset's Sound Class."))
	TObjectPtr<USoundClass> MusicSoundClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Panic Audio | Sound Classes", meta=(ExposeOnSpawn="false", ToolTip="Optional Sound Class override for heartbeat, crowd roar, footsteps, and low drone panic layers. Leave empty to use each asset's Sound Class."))
	TObjectPtr<USoundClass> PanicSFXSoundClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Panic Audio | Sound Classes", meta=(ExposeOnSpawn="false", ToolTip="Optional Sound Class override for shot, pickup, and other short one-shot game sounds. Leave empty to use each asset's Sound Class."))
	TObjectPtr<USoundClass> SFXSoundClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Debug")
	bool bEnableAudioDebugLogs = false;

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
	float HeartbeatVolume = 1.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Volume")
	float LowDroneVolume = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Panic Audio | Music", meta=(ExposeOnSpawn="false", ClampMin="0.0", UIMin="0.0", UIMax="1.0", ToolTip="Background music volume during normal play. The playlist keeps playing through panic and pickup states."))
	float MusicVolume = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Panic Audio | Music", meta=(ExposeOnSpawn="false", ToolTip="Slightly duck background music during panic so heartbeat, crowd, footsteps, and low drone stay in front without stopping the playlist."))
	bool bEnableMusicDucking = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Panic Audio | Music", meta=(ExposeOnSpawn="false", ClampMin="0.0", UIMin="0.0", UIMax="1.0", ToolTip="Background music volume while panic state is active. The track keeps playing and only its volume changes."))
	float PanicMusicVolume = 0.38f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Panic Audio | Music", meta=(ExposeOnSpawn="false", ClampMin="0.0", UIMin="0.0", UIMax="2.0", ToolTip="Fade time for ducking music down when panic starts."))
	float MusicDuckFadeTime = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Panic Audio | Music", meta=(ExposeOnSpawn="false", ClampMin="0.0", UIMin="0.0", UIMax="2.0", ToolTip="Fade time for restoring music volume when panic ends."))
	float MusicRestoreFadeTime = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Volume")
	float Roar1Volume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Volume")
	float Roar2Volume = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Volume")
	float Roar3Volume = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Volume")
	float FootstepRun1Volume = 0.85f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Volume")
	float FootstepRun2Volume = 0.85f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Footsteps", meta=(ClampMin="0.0", UIMin="0.0", UIMax="2.0", ToolTip="Multiplier for the panic footsteps layer. Roar fade does not modify this value."))
	float PanicFootstepsVolume = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Volume")
	float WhisperVolume = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Volume")
	float GhostVolume = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Fade")
	float HeartbeatFadeInDuration = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Fade")
	float HeartbeatFadeOutDuration = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Fade")
	float LowDroneFadeIn = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Fade")
	float LowDroneFadeOut = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Panic Audio | Music", meta=(ExposeOnSpawn="false", ClampMin="0.0", UIMin="0.0", UIMax="5.0", ToolTip="Fade-in time for the selected background music track."))
	float MusicFadeIn = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Panic Audio | Music", meta=(ExposeOnSpawn="false", ClampMin="0.0", UIMin="0.0", UIMax="5.0", ToolTip="Fade-out time used when background music is explicitly stopped."))
	float MusicFadeOut = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Fade")
	float RoarFadeIn = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Fade")
	float RoarFadeOut = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Roar", meta=(ClampMin="0.0", UIMin="0.0", UIMax="5.0", ToolTip="Delay after the panic roar starts before the roar-only fade out begins. Does not affect heartbeat, footsteps, low drone, or music."))
	float PanicRoarFadeOutDelay = 1.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Roar", meta=(ClampMin="0.0", UIMin="0.0", UIMax="10.0", ToolTip="Duration for fading out only the panic roar layer after a shot."))
	float PanicRoarFadeOutDuration = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Roar", meta=(ClampMin="0.0", UIMin="0.0", UIMax="2.0", ToolTip="Multiplier applied to Roar1/2/3 volumes when a new panic roar burst starts."))
	float PanicRoarVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Roar", meta=(ClampMin="0.0", UIMin="0.0", UIMax="1.0", ToolTip="Target volume for the roar-only auto fade. Set to 0 to stop the roar completely after the fade."))
	float PanicRoarEndVolume = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Roar", meta=(ToolTip="Curve used by the roar-only automatic fade out. Does not affect heartbeat, footsteps, low drone, or music."))
	EAudioFaderCurve PanicRoarFadeOutCurve = EAudioFaderCurve::SCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Fade")
	float FootstepsFadeInDuration = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Fade")
	float FootstepsFadeOutDuration = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Fade")
	float AmbientFadeIn = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Fade")
	float AmbientFadeOut = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Fade")
	float PickupReliefFadeOut = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Debug", meta=(ExposeOnSpawn="false", ToolTip="If true, BeginPlay runs a quick audio balance pass: music, heartbeat, roar, footsteps, and low drone with one second between layers."))
	bool bAudioBalanceTestMode = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Bullet Pickup Fear", meta=(ClampMin="0.0", UIMin="0.5", UIMax="3.0"))
	float FearDuration = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Bullet Pickup Fear", meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.6", UIMax="0.7"))
	float FearSpeedMultiplier = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Bullet Pickup Fear", meta=(ClampMin="0.0", UIMin="100.0", UIMax="400.0"))
	float RetreatDistanceMin = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Bullet Pickup Fear", meta=(ClampMin="0.0", UIMin="100.0", UIMax="400.0"))
	float RetreatDistanceMax = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Bullet Pickup Fear", meta=(ClampMin="0.0", ClampMax="90.0", UIMin="0.0", UIMax="45.0"))
	float RetreatAngleVariation = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Bullet Pickup Fear", meta=(ClampMin="0.0", UIMin="100.0", UIMax="500.0"))
	float ExtraSafeDistance = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Bullet Pickup Fear", meta=(ClampMin="0.0", UIMin="0.05", UIMax="0.5"))
	float ReactionDelayMin = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Bullet Pickup Fear", meta=(ClampMin="0.0", UIMin="0.05", UIMax="0.5"))
	float ReactionDelayMax = 0.30f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Bullet Pickup Fear", meta=(ClampMin="0.0", UIMin="0.05", UIMax="0.5"))
	float RetreatDurationMin = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Bullet Pickup Fear", meta=(ClampMin="0.0", UIMin="0.05", UIMax="0.5"))
	float RetreatDurationMax = 0.30f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Bullet Pickup Fear", meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.1", UIMax="0.2"))
	float ReactionPauseSpeedMultiplierMin = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Panic Audio|Bullet Pickup Fear", meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.1", UIMax="0.2"))
	float ReactionPauseSpeedMultiplierMax = 0.20f;

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
	FTimerHandle PanicAudioDelayTimerHandle;
	FTimerHandle LowDroneStartTimerHandle;
	FTimerHandle RoarStartTimerHandle;
	FTimerHandle RoarAutoFadeOutTimerHandle;
	FTimerHandle RoarAutoFadeOutCleanupTimerHandle;
	FTimerHandle FootstepsStartTimerHandle;
	FTimerHandle AmbientHorrorTimerHandle;
	FTimerHandle AudioBalanceTestTimerHandle;
	TWeakObjectPtr<AActor> PendingAudioFocus;
	bool bPanicAudioActive = false;
	bool bHeartbeatLayerActive = false;
	bool bRoarLayerActive = false;
	bool bFootstepsLayerActive = false;
	int32 AudioBalanceTestStep = 0;

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
	void RestartCrowdRoarLayer();
	void StopRoarComponentsImmediately();
	void SchedulePanicRoarFadeOut();
	void BeginPanicRoarFadeOut();
	void CleanupStoppedRoarComponents();
	void StartCrowdFootstepsLayer();
	void StopCrowdFootstepsLayer();
	void StopLowDroneLayer();
	void KeepLoopLayersAlive();
	USoundBase* SelectBackgroundMusicTrack();
	float GetBaseMusicVolume() const;
	float GetPanicMusicVolume() const;
	void SetBackgroundMusicVolume(float TargetVolume, float FadeTime, const TCHAR* DebugReason);
	void DuckMusicForPanic();
	void RestoreMusicAfterPanic();
	bool IsLayerVolumeAudible(const TCHAR* LayerName, float Volume) const;
	void StartAudioBalanceTestMode();
	void PlayAudioBalanceTestStep();
	void PrintAudioDebug(const FString& Message) const;
	void ReportAudioWarning(const FString& Message) const;
	void LogAudioLayerStarted(const TCHAR* LayerName, USoundBase* Sound, float Volume) const;
	void PlayOneShotSound(USoundBase* Sound, float Volume, float Pitch, USoundClass* SoundClassOverride, const TCHAR* DebugName);
	void PlayAmbientSound(USoundBase* Sound, float Volume, const TCHAR* DebugName);
	void PlayRoarSound(USoundBase* Sound, float Volume, const TCHAR* DebugName);
	void PlayFootstepSound(USoundBase* Sound, float Volume, const TCHAR* DebugName);
	UAudioComponent* CreateAudioLayer(USoundBase* Sound, float Volume, float Pitch, const TCHAR* DebugName, USoundClass* SoundClassOverride = nullptr);
	UAudioComponent* FindReusableLayerComponent(TArray<TObjectPtr<UAudioComponent>>& Components, USoundBase* Sound) const;
	bool FadeInExistingLayer(UAudioComponent* Component, USoundBase* Sound, float Volume, float FadeInDuration, const TCHAR* LayerName, USoundClass* SoundClassOverride = nullptr);
	void FadeOutReusableLayer(UAudioComponent* Component, float FadeOutDuration) const;
	void FadeOutAndForget(UAudioComponent* Component, float FadeOutDuration) const;
	void DebugMissingSound(const TCHAR* SoundName) const;
};
